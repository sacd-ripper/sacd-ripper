#ifndef __SACD_LV2_STORAGE_H__
#define __SACD_LV2_STORAGE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include <sys/syscall.h>

#include <sys/integertypes.h>
#include <sys/return_code.h>

#include <sys/interrupt.h>
#include <sys/fixed_addr.h>

#include <sys/raw_spu.h>

CDECL_BEGIN

/*********************************************************************
* Generic Packet commands, MMC commands, and such
*********************************************************************/

/* The generic packet command opcodes for CD/DVD Logical Units,
 * From Table 57 of the SFF8090 Ver. 3 (Mt. Fuji) draft standard. */
#define GPCMD_GET_CONFIGURATION             0x46
#define GPCMD_GET_EVENT_STATUS_NOTIFICATION 0x4a
#define GPCMD_MODE_SELECT_10                0x55
#define GPCMD_MODE_SENSE_10                 0x5a
#define GPCMD_READ_CD                       0xbe
#define GPCMD_READ_DVD_STRUCTURE            0xad
#define GPCMD_READ_TRACK_RZONE_INFO         0x52
#define GPCMD_READ_TOC_PMA_ATIP             0x43
#define GPCMD_REPORT_KEY                    0xa4
#define GPCMD_SEND_KEY                      0xa3

#define LV2_STORAGE_SEND_ATAPI_COMMAND  (1)

struct lv2_atapi_cmnd_block {
	uint8_t pkt[32]; /* packet command block           */
	uint32_t pktlen; /* should be 12 for ATAPI 8020    */
	uint32_t blocks;
	uint32_t block_size;
	uint32_t proto; /* transfer mode                  */
	uint32_t in_out; /* transfer direction             */
	uint32_t unknown;
} __attribute__((packed));

typedef struct {
	int fd;
	uint64_t device_id;
	sys_ppu_thread_t thread_id;
	sys_mutex_t read_async_mutex_id;
	sys_event_queue_t queue;
	int io_buffer;
	int io_buffer_piece;
} sac_accessor_driver_t;

enum lv2_atapi_proto {
	NON_DATA_PROTO = 0,
	PIO_DATA_IN_PROTO = 1,
	PIO_DATA_OUT_PROTO = 2,
	DMA_PROTO = 3
};

enum lv2_atapi_in_out {
	DIR_WRITE = 0, /* memory -> device */
	DIR_READ = 1 /* device -> memory */
};

static inline void init_atapi_cmnd_block(
        struct lv2_atapi_cmnd_block *atapi_cmnd
        , uint32_t block_size
        , uint32_t proto
        , uint32_t type) {
	memset(atapi_cmnd, 0, sizeof(struct lv2_atapi_cmnd_block));
	atapi_cmnd->pktlen = 12;
	atapi_cmnd->blocks = 1;
	atapi_cmnd->block_size = block_size; /* transfer size is block_size * blocks */
	atapi_cmnd->proto = proto;
	atapi_cmnd->in_out = type;
}

static inline int sys_storage_send_atapi_command(uint32_t fd, struct lv2_atapi_cmnd_block *atapi_cmnd, uint8_t *buffer) {
	uint64_t tag;
	system_call_7(SYS_STORAGE_EXECUTE_DEVICE_COMMAND
	              , fd
	              , LV2_STORAGE_SEND_ATAPI_COMMAND
	              , (uint32_t) atapi_cmnd
	              , sizeof (struct lv2_atapi_cmnd_block)
	              , (uint32_t) buffer
	              , atapi_cmnd->block_size
	              , (uint32_t) &tag);

	return_to_user_prog(int);
}

static inline int sys_storage_async_configure(uint32_t fd, int io_buffer, sys_event_queue_t equeue_id, int *unknown) {
	system_call_4(SYS_STORAGE_ASYNC_CONFIGURE
	              , fd
	              , io_buffer
	              , equeue_id
	              , (uint32_t) unknown);

	return_to_user_prog(int);
}

static inline int sys_storage_get_device_info(uint64_t device, uint8_t *buffer) {
	system_call_2(SYS_STORAGE_GET_DEVICE_INFO, device, (uint32_t) buffer);
	return_to_user_prog(int);
}

static inline int sys_storage_open(uint64_t id, int *fd) {
	system_call_4(SYS_STORAGE_OPEN, id, 0, (uint32_t) fd, 0);
	return_to_user_prog(int);
}

static inline int sys_storage_close(int fd) {
	system_call_1(SYS_STORAGE_CLOSE, fd);
	return_to_user_prog(int);
}

static inline int sys_storage_read(int fd, uint32_t start_sector, uint32_t sectors, uint8_t *bounce_buf, uint8_t *sectors_read) {
	system_call_7(SYS_STORAGE_READ, fd, 0, start_sector, sectors, (uint32_t) bounce_buf, (uint32_t) sectors_read, 0);
	return_to_user_prog(int);
}

static inline int sys_storage_async_read(int fd, uint32_t start_sector, uint32_t sectors, uint8_t *unknown1, uint64_t user_data) {
	system_call_7(SYS_STORAGE_ASYNC_READ, fd, 0, start_sector, sectors, (uint32_t) unknown1, user_data, 0);
	return_to_user_prog(int);
}

static inline int sys_io_buffer_create(int *io_buffer, int buffer_count) {
	system_call_5(SYS_IO_BUFFER_CREATE, buffer_count, 2, 64*2048, 512, (uint32_t) io_buffer);
	return_to_user_prog(int);
}

static inline int sys_io_buffer_destroy(int io_buffer) {
	system_call_1(SYS_IO_BUFFER_DESTROY, io_buffer);
	return_to_user_prog(int);
}

static inline int sys_io_buffer_allocate(int io_buffer, int *block) {
	system_call_2(SYS_IO_BUFFER_ALLOCATE, io_buffer, (uint32_t) block);
	return_to_user_prog(int);
}

static inline int sys_io_buffer_free(int io_buffer, int block) {
	system_call_2(SYS_IO_BUFFER_FREE, io_buffer, block);
	return_to_user_prog(int);
}

static inline void sys_storage_reset_bd(void) {
	system_call_2(864, 0x5004, 0x29);
}

static inline void sys_storage_authenticate_bd(void) {
	int func = 0x43;
	system_call_2(864, 0x5007, (uint32_t) &func);
}

extern int ps3rom_lv2_init(int fd, uint8_t *buffer);
extern int ps3rom_lv2_enable_encryption(int fd, uint8_t *buffer, uint32_t lba);
extern int ps3rom_lv2_get_configuration(int fd, uint8_t *buffer);
extern int ps3rom_lv2_mode_sense(int fd, uint8_t *buffer);
extern int ps3rom_lv2_mode_select(int fd);
extern int ps3rom_lv2_get_event_status_notification(int fd, uint8_t *buffer);
extern int ps3rom_lv2_read_toc_header(int fd, uint8_t *buffer);
extern int ps3rom_lv2_read_toc_entry(int fd, uint8_t *buffer);
extern int ps3rom_lv2_read_track(int fd, uint8_t *buffer, uint8_t track);
extern int ps3rom_lv2_read_block(int fd, uint8_t *buffer, int lba);
extern int get_device_info(uint64_t device_id);
extern void open_storage(void);
extern int storage_open(sac_accessor_driver_t *driver);
extern int storage_close(sac_accessor_driver_t *driver);

extern int ps3rom_lv2_report_key_start(int fd, uint8_t *buffer);
extern int ps3rom_lv2_send_key(int fd, uint8_t agid, uint32_t key_size, uint8_t *key, uint8_t sequence);
extern int ps3rom_lv2_report_key(int fd, uint8_t agid, uint32_t *key_size, uint8_t *key, uint8_t sequence);
extern int ps3rom_lv2_report_key_finish(int fd, uint8_t agid);


CDECL_END

#endif /* __SACD_LV2_STORAGE_H__ */
