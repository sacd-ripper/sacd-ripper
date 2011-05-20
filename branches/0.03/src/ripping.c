/**
 * SACD Ripper - http://code.google.com/p/sacd-ripper/
 *
 * Copyright (c) 2010-2011 by respective authors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <string.h>
#include <sysutil/sysutil.h>
#include <sysutil/msg.h>
#include <sys/thread.h>
#include <sys/systime.h>
#include <sys/event_queue.h>
#include <sys/file.h>
#include <sys/mutex.h>
#include <sys/cond.h>
#include <ppu-asm.h>

#include <sys/storage.h>
#include <sys/io_buffer.h>
#include <sys/atomic.h>
#include <utils.h>
#include <log.h>

#include <sacd_reader.h>
#include <scarletbook_read.h>
#include <sac_accessor.h>

#include "ripping.h"
#include "output_device.h"
#include "exit_handler.h"
#include "rsxutil.h"

/**
 * Note: most of this code (except GUI) need to be moved into libsacd,
 * but at this point I just want to get it done, let it be ugly..
 */

/*
   design comes down to: we try to do as much as possible at once but the pipeline is as fast as the slowest component!!

   so we can:
    - initiate a new read request once a previous write request has finished
    - do DST processing using multiple SPUs (should be no bottleneck..)
    - decrypt using multiple SPUs (should be no bottleneck..)

   - open sacd_accessor

    loop:
        - create, write, close DSDIFF/ISO/DSF files, write headers, folders, etc..
        - each read request contains "user_data", like: write destination, offset, conversion format, etc..

        - no outstanding read_request for read_buffer[0]?
            - create sacd_aio_packet_t
            - read read_buffer[0] async
        - no outstanding read_request for read_buffer[1]?
            - create sacd_aio_packet_t
            - read read_buffer[1] async

        - lock decryption mutex
        - wait for condition, x amount of seconds
            - on fail, stop!
        - unlock mutex

        - do we need to stop (user request)?

        - check which read_buffer was processed


    event thread:

        - decrypt data using sac_accessor

        - type of conversion? (none, DSD 3 in 14, DSD 3 in 16, DST)
            - write into write_buffer[x]

        - initiate write request for block [x]

    AIO write callback:

        - mark read_request for read_buffer[x] as done

        - destroy sacd_aio_packet_t
        - lock decryption mutex
        - signal condition
        - unlock decryption mutex

   - close sacd_accessor

 */

extern log_module_info_t * lm_main;

static int               dialog_action = 0;

// the following atomics are used for smooth progressbar indication
static atomic_t selected_progress_message;
static atomic_t partial_blocks_processed;
static atomic_t total_blocks_processed;
static atomic_t stop_processing;            // indicates if the thread needs to stop or has stopped
static atomic_t total_blocks;               // total amount of block to process
static char     progress_message_upper[2][64];
static char     progress_message_lower[2][64];

device_info_t          device_info;

#define ECANCELED                        (0x2f)
#define ESRCH                            (-17)
#define SYS_NO_TIMEOUT                   (0)
#define SYS_EVENT_QUEUE_DESTROY_FORCE    (1)

#define MAX_BLOCK_SIZE                   (512)
#define AIO_MAXIO                        (2)

typedef struct
{
    int                 offset;             // current read offset
    int                 blocks_read;        // amount of blocks read
    int                 conversion;         // conversion that needs to take place

    atomic_t            processing;         // is this packet being processed?
    sys_io_block_t      read_buffer;

    uint8_t            *write_buffer;
    sysFSAio            aio_write;

} sacd_aio_packet_t;

typedef struct sacd_accessor_t
{
    sys_ppu_thread_t        thread_id;
    sys_event_queue_t       queue;
    sys_io_buffer_t         io_buffer;

    sys_cond_t              processing_cond;
    sys_mutex_t             processing_mutex;

    sacd_aio_packet_t       aio_packet[AIO_MAXIO];
} sacd_accessor_t;

enum
{
    CONVERT_NOTHING       = 0
    , CONVERT_DST         = 1
    , CONVERT_DSD_3_IN_14 = 2
    , CONVERT_DSD_3_IN_16 = 3
} conversion_t;

static void sacd_accessor_event_receive(void *arg)
{
    sacd_accessor_t   *accessor = (sacd_accessor_t *) arg;
    sacd_aio_packet_t *packet = 0;
    sys_event_t     event;
    int             ret, block_number;

    LOG(lm_main, LOG_DEBUG, ("sacd_accessor_event_receive %p", arg));

    while (1)
    {
        ret = sysEventQueueReceive(accessor->queue, &event, SYS_NO_TIMEOUT);
        LOG(lm_main, LOG_DEBUG, ("received event..\n"));

        if (ret != 0)
        {
            if (ret == ECANCELED || ret == ESRCH)
            {
                // An expected error when sysEventQueueDestroy is called
                break;
            }
            else
            {
                // An unexpected error
                LOG(lm_main, LOG_ERROR, ("sacd_accessor_event_receive: sys_event_queue_receive failed (%#x)\n", ret));
                sysThreadExit(-1);
                return;
            }
        }

        packet = (sacd_aio_packet_t *) event.data_1;
        if (!packet)
        {
            sysThreadExit(-1);
        }

        LOG(lm_main, LOG_DEBUG, ("sacd_accessor_event_receive: offset = %d, blocks = %d", packet->offset, packet->blocks_read));

    	block_number = 0;
    	while(block_number < MAX_BLOCK_SIZE)
    	{
    		int block_size = min(MAX_BLOCK_SIZE - block_number, 3);
            ret = sac_exec_decrypt_data((uint8_t *) (uint64_t) packet->read_buffer, block_size * SACD_LSN_SIZE, packet->write_buffer);
            if (ret != 0)
            {
                LOG(lm_main, LOG_ERROR, ("sac_exec_decrypt_data: (%#x)\n", ret));
                sysThreadExit(-1);
                return;
            }

    		block_number += block_size;
    	}

        atomic_add(packet->blocks_read, &total_blocks_processed);

        // we are done with processing this packet
        atomic_set(&packet->processing, 0);

        ret = sysMutexLock(accessor->processing_mutex, 0);
        if (ret != 0)
        {
            sysThreadExit(-1);
        }

        ret = sysCondSignal(accessor->processing_cond);
        if (ret != 0)
        {
            sysMutexUnlock(accessor->processing_mutex);
            sysThreadExit(-1);
        }

        ret = sysMutexUnlock(accessor->processing_mutex);
        if (ret != 0)
        {
            sysThreadExit(-1);
        }

    }

    LOG(lm_main, LOG_DEBUG, ("sacd_accessor_event_receive: Exit\n"));
    sysThreadExit(0);
}

int sacd_accessor_open(sacd_accessor_t *accessor, sacd_reader_t *reader)
{
    sys_cond_attr_t         cond_attr;
    sys_mutex_attr_t        mutex_attr;
    sys_event_queue_attr_t  queue_attr;
    int                     ret, tmp, i;

    memset(accessor, 0, sizeof(sacd_accessor_t));

    ret = sys_storage_get_device_info(BD_DEVICE, &device_info);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_storage_get_device_info[%x]\n", ret));
        return ret;
    }

    if (device_info.sector_size != SACD_LSN_SIZE)
    {
        LOG(lm_main, LOG_ERROR, ("incorrect LSN size [%x]\n", device_info.sector_size));
        return -1;
    }

	ret = create_sac_accessor();
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("create_sac_accessor (%#x)\n", ret));
        return ret;
    }
  
    ret = sac_exec_initialize();
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sac_exec_initialize (%#x)\n", ret));
        return ret;
    }

    ret = sac_exec_key_exchange(sacd_get_fd(reader));
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sac_exec_key_exchange (%#x)\n", ret));
        return ret;
    }

    // Create event
    queue_attr.attr_protocol = SYS_EVENT_QUEUE_PRIO;
    queue_attr.type          = SYS_EVENT_QUEUE_PPU;
    queue_attr.name[0]       = '\0';
    ret = sysEventQueueCreate(&accessor->queue, &queue_attr, SYS_EVENT_QUEUE_KEY_LOCAL, 5);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, (__FILE__ ":%d:sys_event_queue_create failed\n", __LINE__));
        return ret;
    }

    // allocate space for AIO_MAXIO * MAX_BLOCK_SIZE * SACD_LSN_SIZE blocks
    ret = sys_io_buffer_create(AIO_MAXIO, MAX_BLOCK_SIZE * SACD_LSN_SIZE, 1, 512, &accessor->io_buffer);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_io_buffer_create[%x]\n", ret));
        return ret;
    }

    for (i = 0; i < AIO_MAXIO; i++)
    {
        accessor->aio_packet[i].write_buffer = (uint8_t *) malloc(2 * MAX_BLOCK_SIZE * SACD_LSN_SIZE);
        ret = sys_io_buffer_allocate(accessor->io_buffer, &accessor->aio_packet[i].read_buffer);
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("sys_io_buffer_allocate[%x] %d %x\n", ret, i, accessor->aio_packet[i].read_buffer));
            return ret;
        }
    }

    ret = sys_storage_async_configure(sacd_get_fd(reader), accessor->io_buffer, accessor->queue, &tmp);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_storage_async_configure ret = %x\n", ret));
        return ret;
    }
    LOG(lm_main, LOG_DEBUG, ("sys_storage_async_configure[%x]\n", ret));

    memset(&cond_attr, 0, sizeof(sys_cond_attr_t));
    cond_attr.attr_pshared = SYS_COND_ATTR_PSHARED;

    memset(&mutex_attr, 0, sizeof(sys_mutex_attr_t));
    mutex_attr.attr_protocol  = SYS_MUTEX_PROTOCOL_PRIO;
    mutex_attr.attr_recursive = SYS_MUTEX_ATTR_NOT_RECURSIVE;
    mutex_attr.attr_pshared   = SYS_MUTEX_ATTR_PSHARED;
    mutex_attr.attr_adaptive  = SYS_MUTEX_ATTR_NOT_ADAPTIVE;

    if (sysMutexCreate(&accessor->processing_mutex, &mutex_attr) != 0)
    {
        //LOG_ERROR("create processing_mutex failed.\n");
        return -1;
    }

    if (sysCondCreate(&accessor->processing_cond, accessor->processing_mutex, &cond_attr) != 0)
    {
        //LOG_ERROR("create processing_cond failed.\n");
        return -1;
    }

    // Initialze input thread
    ret = sysThreadCreate(&accessor->thread_id,
                          sacd_accessor_event_receive,
                          (void *) accessor,
                          1050,
                          8192,
                          THREAD_JOINABLE,
                          "accessor_sacd_accessor_event_receive");
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_ppu_thread_create\n"));
        return ret;
    }
    return 0;
}

int sacd_accessor_close(sacd_accessor_t *accessor)
{
    uint64_t thr_exit_code;
    int      ret, i;

    /*	clean event_queue for spu_printf */
    ret = sysEventQueueDestroy(accessor->queue, SYS_EVENT_QUEUE_DESTROY_FORCE);
    if (ret)
    {
        LOG(lm_main, LOG_ERROR, ("sys_event_queue_destroy failed %x\n", ret));
        return ret;
    }

    for (i = 0; i < AIO_MAXIO; i++)
    {
        free(accessor->aio_packet[i].write_buffer);
        ret = sys_io_buffer_free(accessor->io_buffer, accessor->aio_packet[i].read_buffer);
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("sys_io_buffer_free (%#x) %d %x\n", ret, i, accessor->aio_packet[i].read_buffer));
            return ret;
        }
    }

    ret = sys_io_buffer_destroy(accessor->io_buffer);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_io_buffer_destroy (%#x) %x\n", ret, accessor->io_buffer));
        return ret;
    }

    LOG(lm_main, LOG_DEBUG, ("Wait for the PPU thread %llu exits.\n", accessor->thread_id));
    ret = sysThreadJoin(accessor->thread_id, &thr_exit_code);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_ppu_thread_join failed (%#x)\n", ret));
        return ret;
    }

    if (accessor->processing_cond != 0)
    {
        if ((ret = sysCondDestroy(accessor->processing_cond)) != 0)
        {
            //LOG_ERROR("destroy processing_cond failed.\n");
        }
        else
        {
            accessor->processing_cond = 0;
        }
    }

    if (accessor->processing_mutex != 0)
    {
        if ((ret = sysMutexDestroy(accessor->processing_mutex)) != 0)
        {
            //LOG_ERROR("destroy processing_mutex failed.\n");
        }
        else
        {
            accessor->processing_mutex = 0;
        }
    }

    memset(accessor, 0, sizeof(sacd_accessor_t));

    ret = sac_exec_exit();
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sac_exec_exit (%#x)\n", ret));
    }

    ret = destroy_sac_accessor();
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("destroy_sac_accessor (%#x)\n", ret));
    }
    
    return 0;
}

// TODO: - sac_accessor speed test, vs read speed test

static void processing_thread(void *arg)
{
    int ret, i;
	sacd_reader_t *sacd_reader = 0;
	scarletbook_handle_t *sb_handle = 0;
    sacd_accessor_t sacd_accessor;
    uint32_t current_position = 0;
    int block_size;

	sacd_reader = sacd_open("/dev_bdvd");
	if (!sacd_reader)
	{
        LOG(lm_main, LOG_ERROR, ("could not open device %llx, error: %#x\n", BD_DEVICE, (uint32_t) (uint64_t) sacd_reader));
        goto close_thread;
	}

	sb_handle = scarletbook_open(sacd_reader, 0);
	if (!sb_handle)
	{
        LOG(lm_main, LOG_ERROR, ("could not open scarletbook (%#x)\n", (uint32_t) (uint64_t) sb_handle));
        goto close_thread;
	}

    ret = sacd_accessor_open(&sacd_accessor, sacd_reader);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("could not open accessor (%#x)\n", ret));
        goto close_thread;
    }

    // set total amount of blocks to process
    atomic_set(&total_blocks, (uint32_t) device_info.total_sectors);

    LOG(lm_main, LOG_NOTICE, ("total sectors: %d", (uint32_t) device_info.total_sectors));

    while (atomic_read(&stop_processing) == 0 && current_position < device_info.total_sectors)
    {
        for (i = 0; i < AIO_MAXIO; i++)
        {
            block_size = min(device_info.total_sectors - current_position, MAX_BLOCK_SIZE);

            if (atomic_cmpxchg(&sacd_accessor.aio_packet[i].processing, 0, 1) == 0)
            {
                sacd_accessor.aio_packet[i].offset = current_position;
                sacd_accessor.aio_packet[i].blocks_read = block_size;

                LOG(lm_main, LOG_NOTICE, ("triggering async_read %x pos: %d, size: %d", (uint32_t) (uint64_t) &sacd_accessor.aio_packet[i], current_position, block_size));

                ret = sacd_async_read_block_raw(sacd_reader, current_position, block_size,
                                          sacd_accessor.aio_packet[i].read_buffer, (uint64_t) &sacd_accessor.aio_packet[i]);

                current_position += block_size;
            }
        }

        ret = sysMutexLock(sacd_accessor.processing_mutex, 0);
        if (ret != 0)
        {
            LOG(lm_main, LOG_NOTICE, ("error sysMutexLock"));
            goto close_thread;
        }

        LOG(lm_main, LOG_NOTICE, ("waiting for async read result (10 sec)"));
        ret = sysCondWait(sacd_accessor.processing_cond, 10000000);
        if (ret != 0)
        {
            LOG(lm_main, LOG_NOTICE, ("error sysCondWait"));
            sysMutexUnlock(sacd_accessor.processing_mutex);
            goto close_thread;
        }

        ret = sysMutexUnlock(sacd_accessor.processing_mutex);
        if (ret != 0)
        {
            LOG(lm_main, LOG_NOTICE, ("error sysMutexUnlock"));
            goto close_thread;
        }

        //current_block = atomic_add_return(1, &partial_blocks_processed);
        //
        //if (current_block % 10)
        //{
        //    int message_target = (atomic_read(&selected_progress_message) == 0 ? 1 : 0);
        //
        //    snprintf(progress_message_upper[message_target], 64, "Upper Status %d..", current_block);
        //    snprintf(progress_message_lower[message_target], 64, "Lower status %d..", current_block);
        //
        //    atomic_set(&selected_progress_message, message_target);
        //}
        //
        //if (atomic_add_return(1, &total_blocks_processed) == total_blocks)
        //    break;
        //
        //sysUsleep(100000);
    }

close_thread:

    scarletbook_close(sb_handle);
    sacd_close(sacd_reader);
    sacd_accessor_close(&sacd_accessor); // closing accessor comes after closing device

    atomic_set(&stop_processing, 1);

    sysThreadExit(0);
}

static void dialog_handler(msgButton button, void *user_data)
{
    switch (button)
    {
    case MSG_DIALOG_BTN_OK:
        dialog_action = 1;
        break;
    case MSG_DIALOG_BTN_NO:
    case MSG_DIALOG_BTN_ESCAPE:
        dialog_action = 2;
        break;
    case MSG_DIALOG_BTN_NONE:
        dialog_action = -1;
        break;
    default:
        break;
    }
}

int start_ripping(void)
{
    msgType          dialog_type;
    sys_ppu_thread_t thread_id;
    int              ret;
    uint64_t         retval;

	uint32_t prev_upper_progress = 0;
	uint32_t prev_lower_progress = 0;
	uint32_t delta;

    uint32_t prev_total_blocks_processed = 0;
    uint64_t tb_start, tb_freq;

    memset(progress_message_upper, 0, sizeof(progress_message_upper));
    memset(progress_message_lower, 0, sizeof(progress_message_lower));
    atomic_set(&total_blocks_processed, 0);
    atomic_set(&partial_blocks_processed, 0);
    atomic_set(&stop_processing, 0);
    atomic_set(&selected_progress_message, 0);
    atomic_set(&total_blocks, 0);

    ret = sysThreadCreate(&thread_id, processing_thread, NULL, 1500, 4096, THREAD_JOINABLE, "processing_thread");
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_ppu_thread_join failed (%#x)\n", ret));
        return ret;
    }

    tb_freq = sysGetTimebaseFrequency();
    tb_start = __gettime(); 

    dialog_action = 0;
    dialog_type   = MSG_DIALOG_MUTE_ON | MSG_DIALOG_DOUBLE_PROGRESSBAR;
    msgDialogOpen2(dialog_type, "Copying to:...", dialog_handler, NULL, NULL);
    while (!user_requested_exit() && dialog_action == 0 && atomic_read(&stop_processing) == 0)
    {
        uint32_t tmp_total_blocks_processed = atomic_read(&total_blocks_processed);
        uint32_t tmp_total_blocks = atomic_read(&total_blocks);
        if (tmp_total_blocks != 0 && prev_total_blocks_processed != tmp_total_blocks_processed)
        {
            //msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, delta);

    		delta = (tmp_total_blocks_processed + (tmp_total_blocks_processed - prev_total_blocks_processed)) * 100 / tmp_total_blocks - prev_lower_progress;
    		prev_lower_progress += delta;
            msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX1, delta);

            snprintf(progress_message_lower[0], 64, "transfer rate: (%8.3f MB/sec)", (float)((float) tmp_total_blocks_processed * SACD_LSN_SIZE / 1048576.00) / (float)((__gettime() - tb_start) / (float)(tb_freq)));
            
            //msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0, progress_message_upper[atomic_read(&selected_progress_message)]);
            msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, progress_message_lower[0]);
            
            prev_total_blocks_processed = tmp_total_blocks_processed;
        }

        sysUtilCheckCallback();
        flip();
    }
    msgDialogAbort();

    // in case of user intervention we tell our processing thread to stop
    atomic_set(&stop_processing, 1);

    // wait for our thread to close
    ret = sysThreadJoin(thread_id, &retval);

    // did we complete?
    if (1)
    {
        dialog_type = (MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
        msgDialogOpen2(dialog_type, "Ripping was successful!", dialog_handler, NULL, NULL);

        dialog_action = 0;
        while (!dialog_action && !user_requested_exit())
        {
            sysUtilCheckCallback();
            flip();
        }
        msgDialogAbort();
    }

    return 0;
}
