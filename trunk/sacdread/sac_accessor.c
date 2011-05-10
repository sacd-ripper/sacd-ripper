#include <sys/spu_initialize.h>
#include <sys/raw_spu.h>
#include <sys/spu_utility.h>
#include <sys/ppu_thread.h>
#include <cell/cell_fs.h>
#include <sys/synchronization.h>

#include "sac_accessor.h"
#include "debug.h"
#include "utils.h"

void handle_interrupt(uint64_t);
int file_alloc_load(const char *, uint8_t **, unsigned int *);
int exchange_data(int, uint8_t *, int, uint8_t *, int, usecond_t);

sac_accessor_t *sa = NULL;

int create_sac_accessor(void) {
    sys_cond_attribute_t cond_attr;
    sys_mutex_attribute_t mutex_attr;
#ifndef USE_ISOSELF
    uint32_t entry;
#endif
    int ret;

    if (sa != NULL) {
        return -1;
    }

    sa = malloc(sizeof (sac_accessor_t));
    if (sa == NULL) {
        LOG_ERROR("sac_accessor_t malloc failed\n");
        return -1;
    }
    memset(sa, 0, sizeof (sac_accessor_t));

    sa->buffer = (uint8_t *) memalign(128, DMA_BUFFER_SIZE);
    memset(sa->buffer, 0, DMA_BUFFER_SIZE);

    /*E
     * Initialize SPUs
     */
    LOG_INFO("Initializing SPUs\n");
    ret = sys_spu_initialize(MAX_PHYSICAL_SPU, MAX_RAW_SPU);
    if (ret != CELL_OK) {
        LOG_ERROR("sys_spu_initialize failed: %d\n", ret);
        return ret;
    }

#ifdef USE_ISOSELF
    ret = file_alloc_load(SAC_MODULE_LOCATION, &sa->module_buffer, &sa->module_size);
    if (ret != CELL_OK) {
        LOG_ERROR("cannot load file: " SAC_MODULE_LOCATION "0x%x\n", ret);
        return 0;
    }

    ret = sys_isoself_spu_create(&sa->id, sa->module_buffer);
    if (ret != CELL_OK) {
        LOG_ERROR("sys_isoself_spu_create : 0x%x\n", ret);
        return 0;
    }
#else
    /*E
     * Execute a series of system calls to load a program to a Raw SPU.
     */
    LOG_INFO("syscall sys_raw_spu_create...");
    ret = sys_raw_spu_create(&sa->id, NULL);
    if (ret) {
        LOG_ERROR("sys_raw_spu_create failed %d\n", ret);
        return ret;
    }
    LOG_INFO("succeeded. raw_spu number is %d\n", sa->id);

    /*E
     * Reset all pending interrupts before starting.
     * XXX: This is a workaround for SDK0.5.0. Interrupt Status Registers
     * should be reset when a new Raw SPU is created.
     */
    sys_raw_spu_set_int_stat(sa->id, 2, 0xfUL);
    sys_raw_spu_set_int_stat(sa->id, 0, 0xfUL);

    LOG_INFO("syscall sys_raw_spu_load...");
    ret = sys_raw_spu_load(sa->id, SAC_MODULE_LOCATION, &entry);
    if (ret) {
        LOG_ERROR("raw_spu_load failed %d\n", ret);
        return ret;
    }
    LOG_INFO("succeeded. entry %x\n", entry);
#endif

#ifdef USE_ISOSELF
    ret = sys_isoself_spu_set_int_mask(sa->id, SPU_INTR_CLASS_2, INTR_PPU_MB_MASK | INTR_STOP_MASK | INTR_HALT_MASK);
    if (ret != CELL_OK) {
        LOG_ERROR("sys_isoself_spu_set_int_mask : 0x%x\n", ret);
        return 0;
    }
#else
    ret = sys_raw_spu_set_int_mask(sa->id, SPU_INTR_CLASS_2, INTR_PPU_MB_MASK | INTR_STOP_MASK | INTR_HALT_MASK);
    if (ret != CELL_OK) {
        LOG_ERROR("sys_raw_spu_set_int_mask : 0x%x\n", ret);
        return ret;
    }
#endif

    sys_cond_attribute_initialize(cond_attr);
    sys_mutex_attribute_initialize(mutex_attr);

    if (sys_mutex_create(&sa->mmio_mutex, &mutex_attr) != CELL_OK) {
        LOG_ERROR("create mmio_mutex failed.\n");
        return -1;
    }

    if (sys_cond_create(&sa->mmio_cond, sa->mmio_mutex, &cond_attr) != CELL_OK) {
        LOG_ERROR("create mmio_cond failed.\n");
        return -1;
    }

    /**
     * Create an interrupt PPU thread. (flag = SYS_PPU_THREAD_CREATE_INTERRUPT)
     */
    LOG_INFO("Creating a PPU thread.\n");
    if ((ret = sys_ppu_thread_create(&sa->handler, handle_interrupt, 0, PRIMARY_PPU_THREAD_PRIO,
            PRIMARY_PPU_STACK_SIZE,
            SYS_PPU_THREAD_CREATE_INTERRUPT, (char *) "SEL Interrupt PPU Thread"))
            != CELL_OK) {
        LOG_ERROR("ppu_thread_create returned %d\n", ret);
        return ret;
    }

#ifdef USE_ISOSELF
    ret = sys_isoself_spu_create_interrupt_tag(sa->id, SPU_INTR_CLASS_2, SYS_HW_THREAD_ANY, &sa->intrtag);
    if (ret != CELL_OK) {
        LOG_ERROR("sys_isoself_spu_create_interrupt_tag : 0x%x\n", ret);
        return 0;
    }
#else
    ret = sys_raw_spu_create_interrupt_tag(sa->id, SPU_INTR_CLASS_2, SYS_HW_THREAD_ANY, &sa->intrtag);
    if (ret != CELL_OK) {
        LOG_ERROR("sys_raw_spu_create_interrupt_tag : 0x%x\n", ret);
        return ret;
    }
#endif

    /**
     * Establishing the interrupt tag on the interrupt PPU thread.
     */
    LOG_INFO("Establishing the interrupt tag on the interrupt PPU thread.\n");
    if ((ret = sys_interrupt_thread_establish(&sa->ih, sa->intrtag, sa->handler,
            sa->id)) != CELL_OK) {
        LOG_ERROR("sys_interrupt_thread_establish returned %d\n", ret);
        return ret;
    }

#ifdef USE_ISOSELF
    ret = sys_isoself_spu_set_int_mask(sa->id, SPU_INTR_CLASS_2, INTR_PPU_MB_MASK);
    if (ret != CELL_OK) {
        LOG_ERROR("sys_isoself_spu_set_int_mask : 0x%x\n", ret);
        return 0;
    }

    ret = sys_isoself_spu_start(sa->id);
    if (ret != CELL_OK) {
        LOG_ERROR("sys_isoself_spu_start : 0x%x\n", ret);
        return 0;
    }
#else
    ret = sys_raw_spu_set_int_mask(sa->id, SPU_INTR_CLASS_2, INTR_PPU_MB_MASK);
    if (ret != CELL_OK) {
        LOG_ERROR("sys_raw_spu_set_int_mask : 0x%x\n", ret);
        return ret;
    }

    /*E
     * Run the Raw SPU
     */
    sys_raw_spu_mmio_write(sa->id, SPU_NPC, entry);
    sys_raw_spu_mmio_write(sa->id, SPU_RunCntl, 0x1);
    EIEIO;
#endif

    sys_raw_spu_mmio_write(sa->id, SPU_In_MBox, (uint32_t) sa->buffer);
    EIEIO;

    return 0;
}

int destroy_sac_accessor(void) {
    int ret = 0;

    if (sa == NULL) {
        return -1;
    }

    if (sa->module_buffer) {
        free(sa->module_buffer);
        sa->module_buffer = 0;
    }

    if (sa->id != 0) {
        ret = sys_raw_spu_mmio_read(sa->id, SPU_Status);
        EIEIO;

        if (ret & 0x1) {
            LOG_INFO("sys_raw_spu_mmio_write 0x%x\n", EXIT_SAC_CMD);
            sys_raw_spu_mmio_write(sa->id, SPU_In_MBox, EXIT_SAC_CMD);
            EIEIO;
        }
    }

    if (ret == 0 && sa->ih != 0) {
        if ((ret = sys_interrupt_thread_disestablish(sa->ih)) != CELL_OK) {
            LOG_ERROR("sys_interrupt_thread_disestablish returned %d\n", ret);
        } else {
            sa->ih = 0;
        }
    }

    if (sa->intrtag != 0) {
        if ((ret = sys_interrupt_tag_destroy(sa->intrtag)) != CELL_OK) {
            LOG_ERROR("sys_interrupt_tag_destroy returned %d\n", ret);
        } else {
            sa->intrtag = 0;
        }
    }

    if (sa->id != 0) {
#ifdef USE_ISOSELF
        if ((ret = sys_isoself_spu_destroy(sa->id)) != CELL_OK) {
            LOG_ERROR("sys_isoself_spu_destroy returned %d\n", ret);
        }
#else
        if ((ret = sys_raw_spu_destroy(sa->id)) != CELL_OK) {
            LOG_ERROR("sys_raw_spu_destroy returned %d\n", ret);
        }
#endif
    }

    if (sa->buffer) {
        free(sa->buffer);
        sa->buffer = 0;
    }

    if (sa->mmio_cond != 0) {
        if ((ret = sys_cond_destroy(sa->mmio_cond)) != CELL_OK) {
            LOG_ERROR("destroy mmio_cond failed.\n");
        } else {
            sa->mmio_cond = 0;
        }
    }

    if (sa->mmio_mutex != 0) {
        if ((ret = sys_mutex_destroy(sa->mmio_mutex)) != CELL_OK) {
            LOG_ERROR("destroy mmio_mutex failed.\n");
        } else {
            sa->mmio_mutex = 0;
        }
    }

    free(sa);
    sa = NULL;

    return ret;
}

void handle_interrupt(uint64_t arg) {
    uint64_t stat;
    int ret;

    LOG_INFO("handle_interrupt [%x]\n", (uintptr_t) arg);

    /**
     * Create a tag to handle class 2 interrupt, because PPU Interrupt MB is
     * handled by class 2.
     */
#ifdef USE_ISOSELF
    ret = sys_isoself_spu_get_int_stat(sa->id, SPU_INTR_CLASS_2, &stat);
    if (ret != CELL_OK) {
        LOG_ERROR("sys_isoself_spu_get_int_stat failed %d\n", ret);
        sys_interrupt_thread_eoi();
    }
#else
    LOG_INFO("sys_raw_spu_get_int_stat\n");
    ret = sys_raw_spu_get_int_stat(sa->id, SPU_INTR_CLASS_2, &stat);
    if (ret != CELL_OK) {
        LOG_ERROR("sys_raw_spu_get_int_stat failed %d\n", ret);
        sys_interrupt_thread_eoi();
    }

    LOG_INFO("sys_raw_spu_get_int_stat returned [%016llX]\n", stat);
#endif

    /**
     * If the caught class 2 interrupt includes mailbox interrupt, handle it.
     */
    if ((stat & INTR_PPU_MB_MASK) == INTR_PPU_MB_MASK) {

#ifdef USE_ISOSELF
        ret = sys_isoself_spu_read_puint_mb(sa->id, &sa->error_code);
        if (ret != CELL_OK) {
            LOG_ERROR("sys_isoself_spu_read_puint_mb failed %d\n", ret);
            sys_mutex_unlock(sa->mmio_mutex);
            sys_interrupt_thread_eoi();
        }

        /**
         * Reset the PPU_INTR_MB interrupt status bit.
         */
        ret = sys_isoself_spu_set_int_stat(sa->id, SPU_INTR_CLASS_2, INTR_PPU_MB_MASK);
        if (ret != CELL_OK) {
            LOG_ERROR("sys_isoself_spu_set_int_stat failed %d\n", ret);
            sys_mutex_unlock(sa->mmio_mutex);
            sys_interrupt_thread_eoi();
        }        
#else 
        LOG_INFO("sys_raw_spu_read_puint_mb\n");
        ret = sys_raw_spu_read_puint_mb(sa->id, &sa->error_code);
        if (ret != CELL_OK) {
            LOG_ERROR("sys_raw_spu_read_puint_mb failed %d\n", ret);
            sys_mutex_unlock(sa->mmio_mutex);
            sys_interrupt_thread_eoi();
        }
        LOG_INFO("sys_raw_spu_read_puint_mb returned [%x]\n", sa->error_code);

        /**
         * Reset the PPU_INTR_MB interrupt status bit.
         */
        LOG_INFO("sys_raw_spu_set_int_stat\n");
        ret = sys_raw_spu_set_int_stat(sa->id, SPU_INTR_CLASS_2, INTR_PPU_MB_MASK);
        if (ret != CELL_OK) {
            LOG_ERROR("sys_raw_spu_set_int_stat failed %d\n", ret);
            sys_mutex_unlock(sa->mmio_mutex);
            sys_interrupt_thread_eoi();
        }
#endif

        LOG_INFO("sys_mutex_lock\n");
        ret = sys_mutex_lock(sa->mmio_mutex, 0);
        if (ret != CELL_OK) {
            LOG_ERROR("sys_mutex_lock() failed. (%d)\n", ret);
            sys_interrupt_thread_eoi();
        }

        LOG_INFO("sys_cond_signal\n");
        ret = sys_cond_signal(sa->mmio_cond);
        if (ret != CELL_OK) {
            sys_mutex_unlock(sa->mmio_mutex);
            sys_interrupt_thread_eoi();
        }

        LOG_INFO("sys_mutex_unlock\n");
        ret = sys_mutex_unlock(sa->mmio_mutex);
        if (ret != CELL_OK) {
            sys_interrupt_thread_eoi();
        }
    }
    LOG_INFO("sys_interrupt_thread_eoi\n");
    sys_interrupt_thread_eoi();
}

int exchange_data(int func_nr
        , uint8_t *write_buffer, int write_count
        , uint8_t *read_buffer, int read_count
        , usecond_t timeout) {

    int ret;

    sa->error_code = 0xfffffff1;

    if (write_count > 0) {
        memcpy(sa->buffer, write_buffer, min(write_count, DMA_BUFFER_SIZE));
    }

    LOG_INFO("exchange_data: [sys_mutex_lock]\n");
    ret = sys_mutex_lock(sa->mmio_mutex, 0);
    if (ret != CELL_OK) {
        LOG_ERROR("sys_mutex_lock() failed. (%d)\n", ret);
        return ret;
    }

    LOG_INFO("exchange_data: [writing func %d for amount %d..]\n", func_nr, read_count);
    sys_raw_spu_mmio_write(sa->id, SPU_In_MBox, func_nr);
    sys_raw_spu_mmio_write(sa->id, SPU_In_MBox, read_count);

    LOG_INFO("exchange_data: [sys_cond_wait]\n");
    ret = sys_cond_wait(sa->mmio_cond, timeout);
    if (ret != CELL_OK) {
        LOG_ERROR("exchange_data: [sys_cond_wait timeout]\n");
        sys_mutex_unlock(sa->mmio_mutex);
        return ret;
    }

    LOG_INFO("exchange_data: [sys_mutex_unlock]\n");
    ret = sys_mutex_unlock(sa->mmio_mutex);
    if (ret != CELL_OK) {
        return ret;
    }

    if (sa->error_code == 0xfffffff1) {
        return -1;
    }

    if (read_buffer != NULL && read_count > 0) {
        memcpy(read_buffer, sa->buffer, min(read_count, DMA_BUFFER_SIZE));
    }

    return sa->error_code;
}

int sac_initialize(void) {
 		uint8_t buffer[1] = {0};
		return exchange_data(0, buffer, 1, 0, 0, 5000000);
}

int sac_exit(void) {
		return exchange_data(1, 0, 0, 0, 0, 5000000);
}

int sac_generate_key_1(uint8_t *key, uint32_t expected_size, uint32_t *key_size) {
  uint32_t new_key_size;
  uint8_t buffer[0xd0];
  int ret;
  
  memset(buffer, 0, 0xd0);

	ret = exchange_data(SAC_CMD_GENERATE_KEY_1, 0, 0, buffer, 0xd0, 5000000);

  new_key_size = buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
  if (new_key_size <= expected_size) {

    memcpy(key, buffer + 4, new_key_size);
    *key_size = new_key_size;

    return ret;
  }
  return -1;
}

int sac_validate_key_1(uint8_t *key, uint32_t expected_size, uint8_t *key2) {
  int ret;
  uint8_t buffer[0xd8];

  memset(buffer, 0, 0xd8);

  memcpy(buffer, &expected_size, 4);
  
  buffer[8] = 0;
  buffer[9] = 0;
  buffer[10] = 0;
  buffer[11] = 0;

  memcpy(buffer + 12, key2, 4);
  buffer[12] = 0;
  buffer[13] = 0;
  buffer[14] = 0;
  buffer[15] = 0;
  
  memcpy(buffer + 16, key, expected_size);
  
	ret = exchange_data(SAC_CMD_VALIDATE_KEY_1, buffer, 0xd8, 0, 0, 0);

  return ret;
}

int sac_generate_key_2(uint8_t *key, uint32_t expected_size, uint32_t *key_size) {
  uint32_t new_key_size;
  uint8_t buffer[0xb4];
  int ret;
  
  memset(buffer, 0, 0xb4);

	ret = exchange_data(SAC_CMD_GENERATE_KEY_2, 0, 0, buffer, 0xb4, 5000000);

  new_key_size = buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
  if (new_key_size <= expected_size) {

    memcpy(key, buffer + 4, new_key_size);
    *key_size = new_key_size;

    return ret;
  }
  return -1;
}

int sac_validate_key_2(uint8_t *key, uint32_t expected_size) {
  int ret;
  uint8_t buffer[0xb4];

  memset(buffer, 0, 0xb4);
  memcpy(buffer, &expected_size, 4);
  memcpy(buffer + 4, key, expected_size);

	ret = exchange_data(SAC_CMD_VALIDATE_KEY_2, buffer, 0xb4, 0, 0, 5000000);

  return ret;
}

int sac_validate_key_3(uint8_t *key, uint32_t expected_size) {
  int ret;
  uint8_t buffer[0x34];

  memset(buffer, 0, 0x34);
  memcpy(buffer, &expected_size, 4);
  memcpy(buffer + 4, key, expected_size);

	ret = exchange_data(SAC_CMD_VALIDATE_KEY_3, buffer, 0x34, 0, 0, 5000000);

  return ret;
}

int sac_decrypt_data(uint8_t *buffer1, uint32_t expected_size, uint8_t *buffer2) {
  int ret;
  uint8_t read_buffer[0x1810];
  uint8_t write_buffer[0x1804];
  memset(read_buffer, 0, 0x1810);
  memcpy(read_buffer, &expected_size, 4);
  memcpy(read_buffer + 0x10, buffer1, 4);
	ret = exchange_data(SAC_CMD_DECRYPT, read_buffer, 0x1810, write_buffer, 0x1804, 5000000);

  memcpy(buffer2, write_buffer + 4, 0x1800);

  return ret;
}

int file_alloc_load(const char *file_path, uint8_t **buf, unsigned int *size) {
    int ret, fd;
    CellFsStat status;
    uint64_t read_length;

    ret = cellFsOpen(file_path, CELL_FS_O_RDONLY, &fd, NULL, 0);
    if (ret != CELL_FS_SUCCEEDED) {
        LOG_ERROR("file %s open error : 0x%x\n", file_path, ret);
        return -1;
    }

    ret = cellFsFstat(fd, &status);
    if (ret != CELL_FS_SUCCEEDED) {
        LOG_ERROR("file %s get stat error : 0x%x\n", file_path, ret);
        cellFsClose(fd);
        return -1;
    }

    *buf = malloc(status.st_size);
    if (*buf == NULL) {
        LOG_ERROR("alloc failed\n");
        cellFsClose(fd);
        return -1;
    }

    ret = cellFsRead(fd, *buf, status.st_size, &read_length);
    if (ret != CELL_FS_SUCCEEDED || status.st_size != read_length) {
        LOG_ERROR("file %s read error : 0x%x\n", file_path, ret);
        cellFsClose(fd);
        free(*buf);
        *buf = NULL;
        return -1;
    }

    ret = cellFsClose(fd);
    if (ret != CELL_FS_SUCCEEDED) {
        LOG_ERROR("file %s close error : 0x%x\n", file_path, ret);
        free(*buf);
        *buf = NULL;
        return -1;
    }

    *size = status.st_size;
    return 0;
}
