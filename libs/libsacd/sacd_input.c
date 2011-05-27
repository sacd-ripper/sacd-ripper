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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#if defined(__lv2ppu__)
#include <sys/thread.h>
#include <sys/systime.h>
#include <sys/event_queue.h>
#include <sys/file.h>
#include <sys/mutex.h>
#include <sys/cond.h>
#include <sys/stat.h>
#include <ppu-asm.h>

#include <sys/storage.h>
#include <sys/io_buffer.h>
#include "ioctl.h"
#include "sac_accessor.h"
#elif defined(WIN32)
#include <io.h>
#endif

#include <utils.h>
#include <log.h>

extern log_module_info_t * lm_main;

#include "scarletbook.h"
#include "sacd_input.h"

#if defined(__lv2ppu__)
#define ECANCELED                        (0x2f)
#define ESRCH                            (-17)
#define SYS_NO_TIMEOUT                   (0)
#define SYS_EVENT_QUEUE_DESTROY_FORCE    (1)
#define AIO_MAXIO                        (4)
#endif

struct sacd_input_s
{
    int                 fd;
#if defined(__lv2ppu__)
    device_info_t       device_info;

    sys_ppu_thread_t    thread_id;
    sys_event_queue_t   queue;
    sys_io_buffer_t     io_buffer;
#else

    // synchronous buffer to emulate asynchronous calls
    uint8_t            *io_buffer;

#endif
};

#if defined(__lv2ppu__)
typedef struct
{
    int                 read_position;      // current read position
    int                 read_size;          // amount of blocks read (in LSN)
    sys_io_block_t      read_buffer;
    void               *user_data;    
    sacd_aio_callback_t cb;
} 
sacd_input_aio_packet_t;

static void sacd_input_handle_async_read(void *arg)
{
    sacd_input_t                dev = (sacd_input_t) arg;
    sacd_input_aio_packet_t    *packet = 0;
    sys_event_t                 event;
    int                         ret;

    while (1)
    {
        ret = sysEventQueueReceive(dev->queue, &event, SYS_NO_TIMEOUT);
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
                LOG(lm_main, LOG_ERROR, ("sacd_input_handle_async_read: sys_event_queue_receive failed (%#x)\n", ret));
                sysThreadExit(-1);
            }
        }

        // our packet is the first event argument
        {
            packet = (sacd_input_aio_packet_t *) event.data_1;
            sacd_aio_callback_t cb = (sacd_aio_callback_t) packet->cb;
            cb((uint8_t *) (uint64_t) packet->read_buffer, packet->read_position, packet->read_size, packet->user_data);
            ret = sys_io_buffer_free(dev->io_buffer, packet->read_buffer);
            free(packet);
        }
    }

    sysThreadExit(0);
}
#endif

int sacd_input_decrypt(sacd_input_t dev, uint8_t *buffer, int blocks)
{
#if defined(__lv2ppu__)
    int ret, block_number = 0;
    while(block_number < blocks)
    {
        // SacModule has an internal max of 3*2048 to process
        int block_size = min(blocks - block_number, 3);     
        ret = sac_exec_decrypt_data(buffer + block_number * SACD_LSN_SIZE, block_size * SACD_LSN_SIZE, buffer + (block_number * SACD_LSN_SIZE));
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("sac_exec_decrypt_data: (%#x)\n", ret));
            return -1;
        }

        block_number += block_size;
    }
    return 0;
#else
    return -2;
#endif
}

/**
 * initialize and open a SACD device or file.
 */
sacd_input_t sacd_input_open(const char *target)
{
    sacd_input_t dev;

    /* Allocate the library structure */
    dev = (sacd_input_t) calloc(sizeof(*dev), 1);
    if (dev == NULL)
    {
        fprintf(stderr, "libsacdread: Could not allocate memory.\n");
        return NULL;
    }

    /* Open the device */
#if defined(WIN32)
    dev->fd = open(target, O_RDONLY);

    dev->io_buffer = (uint8_t *) malloc(MAX_PROCESSING_BLOCK_SIZE * SACD_LSN_SIZE);
#elif defined(__lv2ppu__)
    {
        sys_cond_attr_t         cond_attr;
        sys_mutex_attr_t        mutex_attr;
        sys_event_queue_attr_t  queue_attr;
        uint8_t                 buffer[64];
        int                     ret, tmp;

        ret = sys_storage_get_device_info(BD_DEVICE, &dev->device_info);
        if (ret != 0)
        {
            goto error;
        }

        if (sys_storage_open(BD_DEVICE, &dev->fd) != 0)
        {
            goto error;
        }

        ioctl_get_configuration(dev->fd, buffer);
        if ((buffer[0] & 1) != 0)
        {
            ioctl_mode_sense(dev->fd, buffer);
            if (buffer[11] == 2)
            {
                ioctl_mode_select(dev->fd);
            }
        }
        sys_storage_get_device_info(BD_DEVICE, &dev->device_info);

        if (dev->device_info.sector_size != SACD_LSN_SIZE)
        {
            LOG(lm_main, LOG_ERROR, ("incorrect LSN size [%x]\n", dev->device_info.sector_size));
            goto error;
        }

        ret = create_sac_accessor();
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("create_sac_accessor (%#x)\n", ret));
            goto error;
        }

        ret = sac_exec_initialize();
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("sac_exec_initialize (%#x)\n", ret));
            goto error;
        }

        ret = sac_exec_key_exchange(dev->fd);
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("sac_exec_key_exchange (%#x)\n", ret));
            goto error;
        }

        // Create event
        queue_attr.attr_protocol = SYS_EVENT_QUEUE_PRIO;
        queue_attr.type          = SYS_EVENT_QUEUE_PPU;
        queue_attr.name[0]       = '\0';
        ret = sysEventQueueCreate(&dev->queue, &queue_attr, SYS_EVENT_QUEUE_KEY_LOCAL, 5);
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, (__FILE__ ":%d:sys_event_queue_create failed\n", __LINE__));
            goto error;
        }

        // allocate space for AIO_MAXIO * MAX_PROCESSING_BLOCK_SIZE * SACD_LSN_SIZE blocks
        ret = sys_io_buffer_create(AIO_MAXIO, MAX_PROCESSING_BLOCK_SIZE * SACD_LSN_SIZE, 1, 512, &dev->io_buffer);
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("sys_io_buffer_create[%x]\n", ret));
            goto error;
        }

        ret = sys_storage_async_configure(dev->fd, dev->io_buffer, dev->queue, &tmp);
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("sys_storage_async_configure ret = %x\n", ret));
            goto error;
        }
        LOG(lm_main, LOG_DEBUG, ("sys_storage_async_configure[%x]\n", ret));

        memset(&cond_attr, 0, sizeof(sys_cond_attr_t));
        cond_attr.attr_pshared = SYS_COND_ATTR_PSHARED;

        memset(&mutex_attr, 0, sizeof(sys_mutex_attr_t));
        mutex_attr.attr_protocol  = SYS_MUTEX_PROTOCOL_PRIO;
        mutex_attr.attr_recursive = SYS_MUTEX_ATTR_NOT_RECURSIVE;
        mutex_attr.attr_pshared   = SYS_MUTEX_ATTR_PSHARED;
        mutex_attr.attr_adaptive  = SYS_MUTEX_ATTR_NOT_ADAPTIVE;

        // Initialze input thread
        ret = sysThreadCreate(&dev->thread_id,
            sacd_input_handle_async_read,
            (void *) dev,
            1050,
            8192,
            THREAD_JOINABLE,
            "accessor_sacd_input_handle_async_read");
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("sys_ppu_thread_create"));
            goto error;
        }

    }
#else
    dev->fd = open(target, O_RDONLY | O_BINARY);
#endif
    if (dev->fd < 0)
    {
        goto error;
    }

    return dev;

error:

#if defined(WIN32)
    if (dev->io_buffer)
        free(dev->io_buffer);
#endif

    free(dev);

    return 0;
}

/**
 * return the last error message
 */
char *sacd_input_error(sacd_input_t dev)
{
    /* use strerror(errno)? */
    return (char *) "unknown error";
}

/**
 * read data asynchronously from the device.
 */
ssize_t sacd_input_async_read(sacd_input_t dev, int pos, int blocks, sacd_aio_callback_t cb, void *user_data)
{
    ssize_t ret;
#if defined(__lv2ppu__)
    sacd_input_aio_packet_t *packet = (sacd_input_aio_packet_t *) calloc(sizeof(sacd_input_aio_packet_t), 1);
    if (!packet)
        return -1;

    ret = sys_io_buffer_allocate(dev->io_buffer, &packet->read_buffer);
    if (ret != 0)
    {
        free(packet);
        return -1;
    }

    packet->read_position = pos;
    packet->read_size = blocks;
    packet->cb = cb;
    packet->user_data = user_data;

    return sys_storage_async_read(dev->fd, packet->read_position, packet->read_size, packet->read_buffer, (uint64_t) &packet);
#else
    ret = sacd_input_read(dev, pos, blocks, dev->io_buffer);
    cb(dev->io_buffer, pos, blocks, user_data);
    return ret;
#endif    
}

/**
 * read data from the device.
 */
ssize_t sacd_input_read(sacd_input_t dev, int pos, int blocks, void *buffer)
{
#if defined(__lv2ppu__)
    int      ret;
    uint32_t sectors_read;

    ret = sys_storage_read(dev->fd, pos, blocks, buffer, &sectors_read);

    return (ret != 0) ? 0 : sectors_read;

#else
    ssize_t ret, len;

    ret = lseek(dev->fd, (off_t) pos * (off_t) SACD_LSN_SIZE, SEEK_SET);
    if (ret < 0)
    {
        return 0;
    }

    len = (size_t) blocks * SACD_LSN_SIZE;

    while (len > 0)
    {
        ret = read(dev->fd, buffer, (unsigned int) len);

        if (ret < 0)
        {
            /* One of the reads failed, too bad.  We won't even bother
             * returning the reads that went OK, and as in the POSIX spec
             * the file position is left unspecified after a failure. */
            return ret;
        }

        if (ret == 0)
        {
            /* Nothing more to read.  Return all of the whole blocks, if any.
             * Adjust the file position back to the previous block boundary. */
            ssize_t bytes     = (ssize_t) blocks * SACD_LSN_SIZE - len;
            off_t   over_read = -(bytes % SACD_LSN_SIZE);
            /*off_t pos =*/ lseek(dev->fd, over_read, SEEK_CUR);
            /* should have pos % 2048 == 0 */
            return (int) (bytes / SACD_LSN_SIZE);
        }

        len -= ret;
    }

    return blocks;
#endif
}

/**
 * close the SACD device and clean up.
 */
int sacd_input_close(sacd_input_t dev)
{
    int ret;

#if defined(__lv2ppu__)
    uint64_t thr_exit_code;

    /*  clean event_queue for spu_printf */
    ret = sysEventQueueDestroy(dev->queue, SYS_EVENT_QUEUE_DESTROY_FORCE);
    if (ret)
    {
        LOG(lm_main, LOG_ERROR, ("sys_event_queue_destroy failed %x\n", ret));
        return ret;
    }

    ret = sys_io_buffer_destroy(dev->io_buffer);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_io_buffer_destroy (%#x) %x\n", ret, dev->io_buffer));
        return ret;
    }

    LOG(lm_main, LOG_DEBUG, ("Wait for the PPU thread %llu exits.\n", dev->thread_id));
    ret = sysThreadJoin(dev->thread_id, &thr_exit_code);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_ppu_thread_join failed (%#x)\n", ret));
        return ret;
    }

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

    ret = sys_storage_close(dev->fd);
#else
    if (dev->io_buffer)
        free(dev->io_buffer);

    ret = close(dev->fd);
#endif

    if (ret < 0)
        return ret;

    free(dev);

    return 0;
}
