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

#include <sysutil/sysutil.h>
#include <sysutil/msg.h>

#include <sys/atomic.h>

#include "ripping.h"
#include "output_device.h" 
#include "exit_handler.h" 
#include "rsxutil.h"

static int dialog_action = 0;

static atomic_t partial_blocks_processed;
static atomic_t total_blocks_processed;
static uint32_t total_blocks;

static void dialog_handler(msgButton button, void *usrData)
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
 
void start_ripping(void) 
{
    msgType              dialog_type;

    atomic_set(&blocks_processed, 0);

    dialog_type = MSG_DIALOG_MUTE_ON | MSG_DIALOG_DOUBLE_PROGRESSBAR;

    msgDialogOpen2(dialog_type, "Hello World!", dialog_handler, NULL, NULL);

    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0, "Message 1..");
    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, "Message 2..");
    msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, (int) atomic_read(&total_blocks_processed) * 100 / total_blocks);
    msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX1, (int) atomic_read(&partial_blocks_processed) * 100 / total_blocks);

    dialog_action = 0;
    while (!user_requested_exit() && dialog_action == 0)
    {

        sysUtilCheckCallback();
        flip();
    }
    msgDialogAbort(); 
    
} 
 
#if 0

int get_device_info(uint64_t device_id) {
	uint8_t device_info[64];
	int ret = sys_storage_get_device_info(device_id, device_info);
	uint64_t total_sectors = *((uint64_t*) &device_info[40]);
	uint32_t sector_size = *((uint32_t*) &device_info[48]);
	//is_writeable = device_info[56];
	return (ret != CELL_OK || total_sectors == 0 || sector_size != 2048 ? -1 : 0);
}

typedef struct {
	int fd;
	uint64_t device_id;
	sys_ppu_thread_t thread_id;
	sys_mutex_t read_async_mutex_id;
	sys_event_queue_t queue;
	int io_buffer;
	int io_buffer_piece;
} sac_accessor_driver_t;

/*
   void open_storage(void) {
    int fail_count = 0;
    int res = storage_open();

    while (res == -1 && fail_count != 5) {
        sys_timer_usleep(5000000);
        res = storage_open();
   ++fail_count;
    }

    if (res == CELL_OK) {
        ret = get_device_info(driver->device_id);
    }
   }
 */
/*
   static inline int sys_storage_async_read(int fd, uint32_t start_sector, uint32_t sectors, uint8_t *bounce_buf, uint64_t *sectors_read) {
    system_call_7(SYS_STORAGE_READ, fd, 0, start_sector, sectors, (uint32_t) bounce_buf, (uint32_t) sectors_read, 0);
    return_to_user_prog(int);
   }
 */
static void accessor_driver_event_receive(uint64_t arg);

int storage_open(sac_accessor_driver_t *driver) {
	sys_event_queue_attribute_t queue_attr;
	sys_mutex_attribute_t mutex_attr;
	int ret;
	int tmp;

	ret = get_device_info(driver->device_id);
	if (ret != CELL_OK) {
		LOG_ERROR("sys_storage_get_device_info(%llx) failed ! ret = %x\n", driver->device_id, ret);
		return -1;
	}

	ret = sys_storage_open(driver->device_id, &driver->fd);
	if (ret != CELL_OK) {
		LOG_ERROR("sys_storage_open(%llx) failed ! ret = %x\n", driver->device_id, ret);
		return ret;
	}

	/* Create event */
	sys_event_queue_attribute_initialize(queue_attr);
	ret = sys_event_queue_create(&driver->queue, &queue_attr, SYS_EVENT_QUEUE_LOCAL, 5);
	if (ret != CELL_OK) {
		LOG_ERROR(__FILE__ ":%d:sys_event_queue_create failed\n", __LINE__);
		return ret;
	}


	ret = sys_io_buffer_create(&driver->io_buffer, 10);
	LOG_INFO("sys_io_buffer_create[%x]\n", ret);

	ret = sys_io_buffer_allocate(driver->io_buffer, &driver->io_buffer_piece);
	LOG_INFO("sys_io_buffer_allocate[%x] %x\n", ret, driver->io_buffer_piece);

	ret = sys_storage_async_configure(driver->fd, driver->io_buffer, driver->queue, &tmp);
	if (ret != CELL_OK) {
		LOG_ERROR("sys_storage_async_configure ret = %x\n", ret);
		return -3;
	}
	LOG_INFO("sys_storage_async_configure[%x]\n", ret);

	sys_mutex_attribute_initialize(mutex_attr);
	if (sys_mutex_create(&driver->read_async_mutex_id, &mutex_attr) != CELL_OK) {
		LOG_ERROR("create mmio_mutex failed.\n");
		return -3;
	}

	/* Initialze input thread */
	ret = sys_ppu_thread_create(&driver->thread_id,
	                            accessor_driver_event_receive,
	                            (uintptr_t) driver,
	                            1050,
	                            8192,
	                            SYS_PPU_THREAD_CREATE_JOINABLE,
	                            "accessor_driver_event_receive");
	if (ret != CELL_OK) {
		LOG_ERROR("sys_ppu_thread_create\n");
		return -3;
	}
	return ret;
}

int storage_close(sac_accessor_driver_t *driver) {
	uint64_t thr_exit_code;
	int ret;

	ret = sys_storage_close(driver->fd);
	if (ret != CELL_OK) {
		LOG_ERROR("sys_storage_close failed: %x\n", ret);
		return ret;
	}

	/*	clean event_queue for spu_printf */
	ret = sys_event_queue_destroy(driver->queue, SYS_EVENT_QUEUE_DESTROY_FORCE);
	if (ret) {
		LOG_ERROR("sys_event_queue_destroy failed %x\n", ret);
		return ret;
	}

	ret = sys_io_buffer_free(driver->io_buffer, driver->io_buffer_piece);
	if (ret != CELL_OK) {
		LOG_ERROR("sys_io_buffer_free (%#x) %x\n", ret, driver->io_buffer_piece);
		return ret;
	}
	LOG_INFO("sys_io_buffer_free[%x]\n", ret);

	ret = sys_io_buffer_destroy(driver->io_buffer);
	if (ret != CELL_OK) {
		LOG_ERROR("sys_io_buffer_destroy (%#x) %x\n", ret, driver->io_buffer);
		return ret;
	}
	LOG_INFO("sys_io_buffer_destroy[%x]\n", ret);

	/*E
	 * Wait until pu_thr exits
	 * If sys_ppu_thread_join() succeeds, output the exit status.
	 */
	LOG_INFO("Wait for the PPU thread %llu exits.\n", driver->thread_id);
	ret = sys_ppu_thread_join(driver->thread_id, &thr_exit_code);
	if (ret != CELL_OK) {
		LOG_ERROR("sys_ppu_thread_join failed (%#x)\n", ret);
		return ret;
	}

	ret = sys_mutex_destroy(driver->read_async_mutex_id);
	if (ret != CELL_OK) {
		LOG_ERROR("sys_mutex_destroy(au_mutex) failed ! ret = %x\n", ret);
	}

	memset(driver, 0, sizeof(sac_accessor_driver_t));

	return ret;
}

/*E
 * The event queue ID is passed as an argument
 */
static void accessor_driver_event_receive(uint64_t arg) {
	sac_accessor_driver_t *driver = (sac_accessor_driver_t *) (uintptr_t) arg;
	sys_event_t event;
	int ret;

	LOG_INFO("starting thread..\n");

	/*E
	 * Receive events
	 */
	while (1) {
		ret = sys_event_queue_receive(driver->queue, &event, SYS_NO_TIMEOUT);
		LOG_INFO("received event..\n");

		if (ret != CELL_OK) {
			if (ret == ECANCELED || ret == ESRCH) {
				/*E An expected error when sys_event_queue_cancel is called */
				break;
			} else {
				/*E An unexpected error */
				LOG_ERROR("accessor_driver_event_receive: sys_event_queue_receive failed (%#x)\n", ret);
				sys_ppu_thread_exit(-1);
				return;
			}
		}
		LOG_INFO("accessor_driver_event_receive: source = %llx, data1 = %llx, data2 = %llx, data3 = %llx\n",
		         event.source, event.data1, event.data2, event.data3);

		LOG_DATA((uint8_t *) ((uint32_t) driver->io_buffer_piece), 8*2048);
	}

	LOG_INFO("accessor_driver_event_receive: Exit\n");
	sys_ppu_thread_exit(0);
}
#endif