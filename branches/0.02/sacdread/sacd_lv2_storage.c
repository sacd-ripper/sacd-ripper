#include <sys/timer.h>
#include <sys/event.h>
#include <sys/ppu_thread.h>

#include "sacd_lv2_storage.h"
#include "debug.h"

int ps3rom_lv2_get_configuration(int fd, uint8_t *buffer) {
	int res;
	struct lv2_atapi_cmnd_block atapi_cmnd;

	init_atapi_cmnd_block(&atapi_cmnd, 0x10, PIO_DATA_IN_PROTO, DIR_READ);
	atapi_cmnd.pkt[0] = GPCMD_GET_CONFIGURATION;
	atapi_cmnd.pkt[1] = 0;
	atapi_cmnd.pkt[2] = 0xff;
	atapi_cmnd.pkt[3] = 0x41;
	atapi_cmnd.pkt[8] = 0x10;

	res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);
	// if (buffer[10] & 1 == 0) return 0xF000FFFF
	// if (buffer[0] & 1 != 0) exec_mode_sense
	return res;
}

int ps3rom_lv2_mode_sense(int fd, uint8_t *buffer) {
	int res;
	struct lv2_atapi_cmnd_block atapi_cmnd;

	init_atapi_cmnd_block(&atapi_cmnd, 0x10, PIO_DATA_IN_PROTO, DIR_READ);

	atapi_cmnd.pkt[0] = GPCMD_MODE_SENSE_10;
	atapi_cmnd.pkt[1] = 0x08;
	atapi_cmnd.pkt[2] = 0x03;
	atapi_cmnd.pkt[8] = 0x10;

	res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);
	// if (buffer[11] == 2) exec_mode_select
	return res;
}

int ps3rom_lv2_mode_select(int fd) {
	int res;
	struct lv2_atapi_cmnd_block atapi_cmnd;
	static uint8_t buffer[256];
	memset(buffer, 0, sizeof (buffer));

	buffer[1] = 0x0e;
	buffer[7] = 8;
	buffer[8] = 3;
	buffer[9] = 6;
	buffer[11] = 3; // ? 3 == SACD
	buffer[255] = 0x10;

	init_atapi_cmnd_block(&atapi_cmnd, 0x10, PIO_DATA_OUT_PROTO, DIR_WRITE);

	atapi_cmnd.pkt[0] = GPCMD_MODE_SELECT_10;
	atapi_cmnd.pkt[1] = 0x10; /* PF */
	atapi_cmnd.pkt[8] = 0x10;

	res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);

	return res;
}

int ps3rom_lv2_enable_encryption(int fd, uint8_t *buffer, uint32_t lba) {
	int res;
	struct lv2_atapi_cmnd_block atapi_cmnd;

	init_atapi_cmnd_block(&atapi_cmnd, 0x0a, PIO_DATA_IN_PROTO, DIR_READ);

	atapi_cmnd.pkt[0] = GPCMD_READ_DVD_STRUCTURE;
	atapi_cmnd.pkt[1] = 2;

	atapi_cmnd.pkt[2] = lba >> 24;
	atapi_cmnd.pkt[3] = lba >> 16;
	atapi_cmnd.pkt[4] = lba >> 8;
	atapi_cmnd.pkt[5] = lba & 0xff;

	atapi_cmnd.pkt[9] = 0x0a;

	//LOG_DATA(&atapi_cmnd, 0x38);
	res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);

	return res;
}

int ps3rom_lv2_get_event_status_notification(int fd, uint8_t *buffer) {
	int res;
	struct lv2_atapi_cmnd_block atapi_cmnd;

	init_atapi_cmnd_block(&atapi_cmnd, 0x08, PIO_DATA_IN_PROTO, DIR_READ);

	atapi_cmnd.pkt[0] = GPCMD_GET_EVENT_STATUS_NOTIFICATION;
	atapi_cmnd.pkt[1] = 1; /* IMMED */
	atapi_cmnd.pkt[4] = 4; /* media event */
	atapi_cmnd.pkt[8] = 0x08;

	res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);

	return res;
}

int ps3rom_lv2_report_key_start(int fd, uint8_t *buffer) {
	int res;
	struct lv2_atapi_cmnd_block atapi_cmnd;

	init_atapi_cmnd_block(&atapi_cmnd, 0x08, PIO_DATA_IN_PROTO, DIR_READ);

	atapi_cmnd.pkt[0] = GPCMD_REPORT_KEY;

	atapi_cmnd.pkt[2] = 0;
	atapi_cmnd.pkt[3] = 0;
	atapi_cmnd.pkt[4] = 0;
	atapi_cmnd.pkt[5] = 0x08;

	atapi_cmnd.pkt[6] = 0;
	atapi_cmnd.pkt[7] = 0x10;

	//LOG_DATA(&atapi_cmnd, 0x38);
	res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);
	//LOG_DATA(buffer, 16);

	return res;
}

int ps3rom_lv2_send_key(int fd, uint8_t agid, uint32_t key_size, uint8_t *key, uint8_t sequence) {
	int res;
	struct lv2_atapi_cmnd_block atapi_cmnd;
	uint32_t buffer_size;
	uint8_t buffer[256];
	uint8_t buffer_align = 0;

	if ((key_size & 3) != 0) {
		buffer_align = ~(key_size & 3) + 4 + 1;
	}
	buffer_size = key_size + 4 + buffer_align;

	init_atapi_cmnd_block(&atapi_cmnd, buffer_size, PIO_DATA_OUT_PROTO, DIR_WRITE);

	atapi_cmnd.pkt[0] = GPCMD_SEND_KEY;

	atapi_cmnd.pkt[2] = buffer_size >> 24;
	atapi_cmnd.pkt[3] = buffer_size >> 16;
	atapi_cmnd.pkt[4] = buffer_size >> 8;
	atapi_cmnd.pkt[5] = buffer_size & 0xff;

	atapi_cmnd.pkt[6] = sequence;
	atapi_cmnd.pkt[7] = 0x10;
	atapi_cmnd.pkt[10] = agid;

	memset(buffer, 0, sizeof(buffer));

	buffer[0] = key_size >> 24;
	buffer[1] = key_size >> 16;
	buffer[2] = key_size >> 8;
	buffer[3] = key_size & 0xff;
	memcpy(buffer + 4, key, key_size);

	//if (buffer_align != 0) {
	//	memset(buffer + key_size + 4, 0, buffer_align);
	//}
	//
	//buffer[0xfc] = buffer_size >> 24;
	//buffer[0xfd] = buffer_size >> 16;
	//buffer[0xfe] = buffer_size >> 8;
	//buffer[0xff] = buffer_size & 0xff;
	//LOG_DATA(&atapi_cmnd, 0x38);
	//LOG_DATA(buffer, 256);

	res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);

	return res;
}

int ps3rom_lv2_report_key(int fd, uint8_t agid, uint32_t *key_size, uint8_t *key, uint8_t sequence) {
	int res;
	struct lv2_atapi_cmnd_block atapi_cmnd;
	uint32_t buffer_size;
	uint8_t buffer[256];
	uint8_t buffer_align = 0;
	uint32_t new_key_size, old_key_size = *key_size;

	memset(buffer, 0, sizeof(buffer));

	if ((old_key_size & 3) != 0) {
		buffer_align = ~(old_key_size & 3) + 4 + 1;
	}
	buffer_size = old_key_size + 4 + buffer_align;

	init_atapi_cmnd_block(&atapi_cmnd, buffer_size, PIO_DATA_IN_PROTO, DIR_READ);

	atapi_cmnd.pkt[0] = GPCMD_REPORT_KEY;

	atapi_cmnd.pkt[2] = buffer_size >> 24;
	atapi_cmnd.pkt[3] = buffer_size >> 16;
	atapi_cmnd.pkt[4] = buffer_size >> 8;
	atapi_cmnd.pkt[5] = buffer_size & 0xff;

	atapi_cmnd.pkt[6] = sequence;
	atapi_cmnd.pkt[7] = 0x10;
	atapi_cmnd.pkt[10] = agid;

	//LOG_DATA(&atapi_cmnd, 0x38);

	res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);

	new_key_size = buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
	*key_size = new_key_size;

	memcpy(key, buffer + 4, (old_key_size > new_key_size ? new_key_size : old_key_size));

	//LOG_DATA(buffer, 256);

	return res;
}

int ps3rom_lv2_report_key_finish(int fd, uint8_t agid) {
	int res;
	struct lv2_atapi_cmnd_block atapi_cmnd;

	init_atapi_cmnd_block(&atapi_cmnd, 0, NON_DATA_PROTO, DIR_READ);

	atapi_cmnd.pkt[0] = GPCMD_REPORT_KEY;

	atapi_cmnd.pkt[6] = 0xff;
	atapi_cmnd.pkt[7] = 0x10;
	atapi_cmnd.pkt[10] = agid;

	//LOG_DATA(&atapi_cmnd, 0x38);
	res = sys_storage_send_atapi_command(fd, &atapi_cmnd, 0);

	return res;
}


int ps3rom_lv2_read_toc_header(int fd, uint8_t *buffer) {
	int res;
	struct lv2_atapi_cmnd_block atapi_cmnd;

	init_atapi_cmnd_block(&atapi_cmnd, 12, PIO_DATA_IN_PROTO, DIR_READ);

	atapi_cmnd.pkt[0] = GPCMD_READ_TOC_PMA_ATIP;
	atapi_cmnd.pkt[6] = 0;
	atapi_cmnd.pkt[8] = 12; /* LSB of length */

	res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);

	return res;
}

int ps3rom_lv2_read_toc_entry(int fd, uint8_t *buffer) {
	int res;
	struct lv2_atapi_cmnd_block atapi_cmnd;

	init_atapi_cmnd_block(&atapi_cmnd, 12, PIO_DATA_IN_PROTO, DIR_READ);

	atapi_cmnd.pkt[0] = GPCMD_READ_TOC_PMA_ATIP;
	atapi_cmnd.pkt[6] = 0x01;
	atapi_cmnd.pkt[8] = 12; /* LSB of length */

	res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);

	return res;
}

int ps3rom_lv2_read_track(int fd, uint8_t *buffer, uint8_t track) {
	int res;
	struct lv2_atapi_cmnd_block atapi_cmnd;

	init_atapi_cmnd_block(&atapi_cmnd, 48, PIO_DATA_IN_PROTO, DIR_READ);

	atapi_cmnd.pkt[0] = GPCMD_READ_TRACK_RZONE_INFO;
	atapi_cmnd.pkt[1] = 1;
	atapi_cmnd.pkt[5] = track; /* track */
	atapi_cmnd.pkt[8] = 48; /* LSB of length */

	res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);

	return res;
}

int get_device_info(uint64_t device_id) {
	uint8_t device_info[64];
	int ret = sys_storage_get_device_info(device_id, device_info);
	uint64_t total_sectors = *((uint64_t*) &device_info[40]);
	uint32_t sector_size = *((uint32_t*) &device_info[48]);
	//is_writeable = device_info[56];
	return (ret != CELL_OK || total_sectors == 0 || sector_size != 2048 ? -1 : 0);
}


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
