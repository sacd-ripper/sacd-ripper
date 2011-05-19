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

#include <sys/storage.h>
#include <sys/io_buffer.h>
#include <sys/atomic.h>
#include <log.h>

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

        - no outstanding read_request for io_block[0]?
            - create sacd_aio_packet_t
            - read io_block[0] async
        - no outstanding read_request for io_block[1]?
            - create sacd_aio_packet_t
            - read io_block[1] async

        - lock decryption mutex
        - wait for condition, x amount of seconds
            - on fail, stop!
        - unlock mutex

        - do we need to stop (user request)?

        - check which io_block was processed


    event thread:

        - decrypt data using sac_accessor

        - type of conversion? (none, DSD 3 in 14, DSD 3 in 16, DST)
            - write into write_buffer[x]

        - initiate write request for block [x]

    AIO write callback:

        - mark read_request for io_block[x] as done

        - destroy sacd_aio_packet_t
        - lock decryption mutex
        - signal condition
        - unlock decryption mutex

   - close sacd_accessor

 */

extern log_module_info_t * lm_main;

static int               dialog_action = 0;

// the following atomic are only used for smooth progressbar indication
static atomic_t selected_progress_message;
static atomic_t partial_blocks_processed;
static atomic_t total_blocks_processed;
static atomic_t stop_processing;            // indicates if the thread needs to stop or has stopped
static uint32_t total_blocks;               // total amount of block to process
static char     progress_message_upper[2][64];
static char     progress_message_lower[2][64];

#define ECANCELED                        (0x2f)
#define ESRCH                            (-17)
#define SYS_NO_TIMEOUT                   (0)
#define SYS_EVENT_QUEUE_DESTROY_FORCE    (1)

typedef struct
{
    int               fd;

    sys_ppu_thread_t  thread_id;
    sys_event_queue_t queue;
    sys_io_buffer_t   io_buffer;
    sys_io_block_t    io_block[2];

    uint8_t           *write_buffer[2];
} sacd_accessor_t;

enum
{
    CONVERT_NOTHING       = 0
    , CONVERT_DST         = 1
    , CONVERT_DSD_3_IN_14 = 2
    , CONVERT_DSD_3_IN_16 = 3
} conversion_t;

typedef struct
{
    int      id;
    int      offset;
    int      blocks_read;
    int      conversion;
    sysFSAio aio_write;
} sacd_aio_packet_t;

static void sacd_accessor_event_receive(void *arg)
{
    sacd_accessor_t *accessor = (sacd_accessor_t *) arg;
    sys_event_t     event;
    int             ret;

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
        LOG(lm_main, LOG_DEBUG, ("sacd_accessor_event_receive: source = %llx, data1 = %llx, data2 = %llx, data3 = %llx\n",
                                 event.source, event.data_1, event.data_2, event.data_3));
    }

    LOG(lm_main, LOG_DEBUG, ("sacd_accessor_event_receive: Exit\n"));
    sysThreadExit(0);
}

int sacd_accessor_open(sacd_accessor_t *accessor)
{
    sys_event_queue_attr_t queue_attr;
    uint8_t                device_info[64];
    uint64_t               total_sectors = 0;
    uint32_t               sector_size   = 0;
    int                    ret, tmp;

    ret = sys_storage_get_device_info(BD_DEVICE, device_info);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_storage_get_device_info[%x]\n", ret));
        return ret;
    }
    total_sectors = *((uint64_t *) &device_info[40]);
    sector_size   = *((uint32_t *) &device_info[48]);

    // Create event
    queue_attr.attr_protocol = SYS_EVENT_QUEUE_PRIO;
    queue_attr.type          = SYS_EVENT_QUEUE_PPU;
    queue_attr.name[0]       = '\0';
    ret                      = sysEventQueueCreate(&accessor->queue, &queue_attr, SYS_EVENT_QUEUE_KEY_LOCAL, 5);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, (__FILE__ ":%d:sys_event_queue_create failed\n", __LINE__));
        return ret;
    }

    // allocate space for two 1MB blocks
    ret = sys_io_buffer_create(2, 1024 * 1024, 1, 512, &accessor->io_buffer);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_io_buffer_create[%x]\n", ret));
        return ret;
    }

    ret = sys_io_buffer_allocate(accessor->io_buffer, &accessor->io_block[0]);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_io_buffer_allocate[%x] %x\n", ret, accessor->io_block[0]));
        return ret;
    }

    ret = sys_io_buffer_allocate(accessor->io_buffer, &accessor->io_block[1]);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_io_buffer_allocate[%x] %x\n", ret, accessor->io_block[1]));
        return ret;
    }

    ret = sys_storage_async_configure(accessor->fd, accessor->io_buffer, accessor->queue, &tmp);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_storage_async_configure ret = %x\n", ret));
        return -3;
    }
    LOG(lm_main, LOG_DEBUG, ("sys_storage_async_configure[%x]\n", ret));

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
        return -3;
    }
    return ret;
}

int sacd_accessor_close(sacd_accessor_t *accessor)
{
    uint64_t thr_exit_code;
    int      ret;

    ret = sys_storage_close(accessor->fd);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_storage_close failed: %x\n", ret));
        return ret;
    }

    /*	clean event_queue for spu_printf */
    ret = sysEventQueueDestroy(accessor->queue, SYS_EVENT_QUEUE_DESTROY_FORCE);
    if (ret)
    {
        LOG(lm_main, LOG_ERROR, ("sys_event_queue_destroy failed %x\n", ret));
        return ret;
    }

    ret = sys_io_buffer_free(accessor->io_buffer, accessor->io_block[0]);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_io_buffer_free (%#x) %x\n", ret, accessor->io_block[0]));
        return ret;
    }

    ret = sys_io_buffer_free(accessor->io_buffer, accessor->io_block[1]);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_io_buffer_free (%#x) %x\n", ret, accessor->io_block[1]));
        return ret;
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

    memset(accessor, 0, sizeof(sacd_accessor_t));

    return ret;
}

static void processing_thread(void *arg)
{
    uint32_t current_block;

    // TODO: - sac_accessor speed test, vs read speed test

    while (atomic_read(&stop_processing) == 0)
    {
        current_block = atomic_add_return(1, &partial_blocks_processed);

        if (current_block % 10)
        {
            int message_target = (atomic_read(&selected_progress_message) == 0 ? 1 : 0);

            snprintf(progress_message_upper[message_target], 64, "Upper Status %d..", current_block);
            snprintf(progress_message_lower[message_target], 64, "Lower status %d..", current_block);

            atomic_set(&selected_progress_message, message_target);
        }

        if (atomic_add_return(1, &total_blocks_processed) == total_blocks)
            break;

        sysUsleep(100000);
    }

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
    uint32_t         prev_total_blocks_processed   = 0;
    uint32_t         prev_partial_blocks_processed = 0;

    memset(progress_message_upper, 0, sizeof(progress_message_upper));
    memset(progress_message_lower, 0, sizeof(progress_message_lower));
    atomic_set(&total_blocks_processed, 0);
    atomic_set(&partial_blocks_processed, 0);
    atomic_set(&stop_processing, 0);
    atomic_set(&selected_progress_message, 0);
    total_blocks = 0;

    ret = sysThreadCreate(&thread_id, processing_thread, NULL, 1500, 4096, THREAD_JOINABLE, "processing_thread");
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_ppu_thread_join failed (%#x)\n", ret));
        return ret;
    }

    total_blocks = 100;

    dialog_action = 0;
    dialog_type   = MSG_DIALOG_MUTE_ON | MSG_DIALOG_DOUBLE_PROGRESSBAR;
    msgDialogOpen2(dialog_type, "Copying to:...", dialog_handler, NULL, NULL);
    while (!user_requested_exit() && dialog_action == 0 && atomic_read(&stop_processing) == 0)
    {
        msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, (atomic_read(&total_blocks_processed) * 100 / total_blocks) - prev_total_blocks_processed);
        msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX1, (atomic_read(&partial_blocks_processed) * 100 / total_blocks) - prev_partial_blocks_processed);

        msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0, progress_message_upper[atomic_read(&selected_progress_message)]);
        msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, progress_message_lower[atomic_read(&selected_progress_message)]);

        prev_total_blocks_processed   = atomic_read(&total_blocks_processed);
        prev_partial_blocks_processed = atomic_read(&partial_blocks_processed);

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
