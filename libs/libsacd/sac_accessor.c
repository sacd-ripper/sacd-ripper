/**
 * SACD Ripper - https://github.com/sacd-ripper/
 *
 * Copyright (c) 2010-2015 by respective authors.
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

#ifdef __lv2ppu__

#include <ppu-lv2.h>
#include <sys/spu.h>
#include <sys/mutex.h>
#include <sys/cond.h>
#include <sys/thread.h>
#include <sys/file.h>
#include <sys/interrupt.h>
#include <string.h>
#include <malloc.h>

#include <utils.h>
#include <logging.h>
#include "sac_accessor.h"
#include "ioctl.h"

#ifdef USE_ISOSELF
int file_alloc_load(const char *, uint8_t **, unsigned int *);
#endif

void handle_interrupt(void *);
int sac_exec_generate_key_1(uint8_t *, uint32_t, uint32_t *);
int sac_exec_validate_key_1(uint8_t *, uint32_t);
int sac_exec_generate_key_2(uint8_t *, uint32_t, uint32_t *);
int sac_exec_validate_key_2(uint8_t *, uint32_t);
int sac_exec_validate_key_3(uint8_t *, uint32_t);
int exchange_data(int, uint8_t *, int, uint8_t *, int, uint32_t);

static sac_accessor_t *sa = NULL;

int create_sac_accessor(void)
{
    sys_cond_attr_t  cond_attr;
    sys_mutex_attr_t mutex_attr;
#ifndef USE_ISOSELF
    uint32_t         entry;
#endif
    int              ret;

    if (sa != NULL)
    {
        return -1;
    }

    sa = calloc(sizeof(sac_accessor_t), 1);
    if (sa == NULL)
    {
        LOG(lm_main, LOG_ERROR, ("sac_accessor_t malloc failed\n"));
        return -1;
    }
    sa->id = -1;

    sa->buffer = (uint8_t *) memalign(128, DMA_BUFFER_SIZE);
    memset(sa->buffer, 0, DMA_BUFFER_SIZE);

    sa->read_buffer = (uint8_t *) malloc(DMA_BUFFER_SIZE);
    sa->write_buffer = (uint8_t *) malloc(DMA_BUFFER_SIZE);

#ifdef USE_ISOSELF
    ret = file_alloc_load(SAC_MODULE_LOCATION, &sa->module_buffer, &sa->module_size);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("cannot load file: " SAC_MODULE_LOCATION "0x%x\n", ret));
        return 0;
    }

    ret = sys_isoself_spu_create(&sa->id, sa->module_buffer);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_isoself_spu_create : 0x%x\n", ret));
        return 0;
    }
#else
    ret = sysSpuRawCreate(&sa->id, NULL);
    if (ret)
    {
        LOG(lm_main, LOG_ERROR, ("sysSpuRawCreate failed %d\n", ret));
        return ret;
    }
    LOG(lm_main, LOG_DEBUG, ("succeeded. raw_spu number is %d\n", sa->id));

    // Reset all pending interrupts before starting.
    sysSpuRawSetIntStat(sa->id, 2, 0xfUL);
    sysSpuRawSetIntStat(sa->id, 0, 0xfUL);

    ret = sysSpuRawLoad(sa->id, SAC_MODULE_LOCATION, &entry);
    if (ret)
    {
        LOG(lm_main, LOG_ERROR, ("sysSpuRawLoad failed [" SAC_MODULE_LOCATION "]%d\n", ret));
        return ret;
    }
    LOG(lm_main, LOG_DEBUG, ("succeeded. entry %x\n", entry));
#endif

#ifdef USE_ISOSELF
    ret = sys_isoself_spu_set_int_mask(sa->id, SPU_INTR_CLASS_2, INTR_PPU_MB_MASK | INTR_STOP_MASK | INTR_HALT_MASK);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_isoself_spu_set_int_mask : 0x%x\n", ret));
        return 0;
    }
#else
    ret = sysSpuRawSetIntMask(sa->id, SPU_INTR_CLASS_2, INTR_PPU_MB_MASK | INTR_STOP_MASK | INTR_HALT_MASK);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_raw_spu_set_int_mask : 0x%x\n", ret));
        return ret;
    }
#endif

    memset(&cond_attr, 0, sizeof(sys_cond_attr_t));
    cond_attr.attr_pshared = SYS_COND_ATTR_PSHARED;

    memset(&mutex_attr, 0, sizeof(sys_mutex_attr_t));
    mutex_attr.attr_protocol  = SYS_MUTEX_PROTOCOL_PRIO;
    mutex_attr.attr_recursive = SYS_MUTEX_ATTR_NOT_RECURSIVE;
    mutex_attr.attr_pshared   = SYS_MUTEX_ATTR_PSHARED;
    mutex_attr.attr_adaptive  = SYS_MUTEX_ATTR_NOT_ADAPTIVE;

    if (sysMutexCreate(&sa->mmio_mutex, &mutex_attr) != 0)
    {
        LOG(lm_main, LOG_ERROR, ("create mmio_mutex failed.\n"));
        return -1;
    }

    if (sysCondCreate(&sa->mmio_cond, sa->mmio_mutex, &cond_attr) != 0)
    {
        LOG(lm_main, LOG_ERROR, ("create mmio_cond failed.\n"));
        return -1;
    }

    if ((ret = sysThreadCreate(&sa->handler, handle_interrupt, 0, PRIMARY_PPU_THREAD_PRIO,
                               PRIMARY_PPU_STACK_SIZE,
                               THREAD_INTERRUPT, (char *) "SEL Interrupt PPU Thread"))
        != 0)
    {
        LOG(lm_main, LOG_ERROR, ("ppu_thread_create returned %d\n", ret));
        return ret;
    }

#ifdef USE_ISOSELF
    ret = sys_isoself_spu_create_interrupt_tag(sa->id, SPU_INTR_CLASS_2, SYS_HW_THREAD_ANY, &sa->intrtag);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_isoself_spu_create_interrupt_tag : 0x%x\n", ret));
        return 0;
    }
#else
    ret = sysSpuRawCreateInterrupTag(sa->id, SPU_INTR_CLASS_2, SYS_HW_THREAD_ANY, &sa->intrtag);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_raw_spu_create_interrupt_tag : 0x%x\n", ret));
        return ret;
    }
#endif

    // Establishing the interrupt tag on the interrupt PPU thread.
    LOG(lm_main, LOG_DEBUG, ("Establishing the interrupt tag on the interrupt PPU thread.\n"));
    if ((ret = sysInterruptThreadEstablish(&sa->ih, sa->intrtag, sa->handler,
                                           sa->id)) != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_interrupt_thread_establish returned %d\n", ret));
        return ret;
    }

#ifdef USE_ISOSELF
    ret = sys_isoself_spu_set_int_mask(sa->id, SPU_INTR_CLASS_2, INTR_PPU_MB_MASK);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_isoself_spu_set_int_mask : 0x%x\n", ret));
        return 0;
    }

    ret = sys_isoself_spu_start(sa->id);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_isoself_spu_start : 0x%x\n", ret));
        return 0;
    }
#else
    ret = sysSpuRawSetIntMask(sa->id, SPU_INTR_CLASS_2, INTR_PPU_MB_MASK);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_raw_spu_set_int_mask : 0x%x\n", ret));
        return ret;
    }

    // Run the Raw SPU
    sysSpuRawWriteProblemStorage(sa->id, SPU_NextPC, entry);
    sysSpuRawWriteProblemStorage(sa->id, SPU_RunCtrl, 0x1);
    EIEIO;
#endif

    sysSpuRawWriteProblemStorage(sa->id, SPU_In_MBox, (uint64_t) sa->buffer);
    EIEIO;

    return 0;
}

int destroy_sac_accessor(void)
{
    int ret = 0;

    if (sa == NULL)
    {
        return 0;
    }

    if (sa->module_buffer)
    {
        free(sa->module_buffer);
        sa->module_buffer = 0;
    }

    if (sa->id != -1)
    {
        ret = sysSpuRawReadProblemStorage(sa->id, SPU_Status);
        EIEIO;

        if (ret & 0x1)
        {
            sysSpuRawWriteProblemStorage(sa->id, SPU_In_MBox, EXIT_SAC_CMD);
            EIEIO;

            sysSpuRawWriteProblemStorage(sa->id, SPU_In_MBox, 0);
            EIEIO;
        }
    }

    if ((ret = sysInterruptThreadDisestablish(sa->ih)) != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sysInterruptThreadDisestablish returned %d\n", ret));
    }

    if ((ret = sysInterruptTagDestroy(sa->intrtag)) != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_interrupt_tag_destroy returned %d\n", ret));
    }

    if (sa->id != -1)
    {
#ifdef USE_ISOSELF
        if ((ret = sys_isoself_spu_destroy(sa->id)) != 0)
        {
            LOG(lm_main, LOG_ERROR, ("sys_isoself_spu_destroy returned %d\n", ret));
        }
#else
        if ((ret = sysSpuRawDestroy(sa->id)) != 0)
        {
            LOG(lm_main, LOG_ERROR, ("sys_raw_spu_destroy returned %d\n", ret));
        }
#endif
    }

    if (sa->buffer)
    {
        free(sa->buffer);
        sa->buffer = 0;
    }

    if (sa->read_buffer)
    {   
        free(sa->read_buffer);
        sa->read_buffer = 0;
    }
    
    if (sa->write_buffer)
    {
        free(sa->write_buffer);
        sa->write_buffer = 0;
    }

    if ((ret = sysCondDestroy(sa->mmio_cond)) != 0)
    {
        LOG(lm_main, LOG_ERROR, ("destroy mmio_cond failed.\n"));
    }

    if ((ret = sysMutexDestroy(sa->mmio_mutex)) != 0)
    {
        LOG(lm_main, LOG_ERROR, ("destroy mmio_mutex failed.\n"));
    }

    free(sa);
    sa = NULL;

    return ret;
}

void handle_interrupt(void *arg)
{
    uint64_t stat;
    int      ret;

    // Create a tag to handle class 2 interrupt, because PPU Interrupt MB is
    // handled by class 2.
#ifdef USE_ISOSELF
    ret = sys_isoself_spu_get_int_stat(sa->id, SPU_INTR_CLASS_2, &stat);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_isoself_spu_get_int_stat failed %d\n", ret));
        sysInterruptThreadEOI();
    }
#else
    ret = sysSpuRawGetIntStat(sa->id, SPU_INTR_CLASS_2, &stat);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sys_raw_spu_get_int_stat failed %d\n", ret));
        sysInterruptThreadEOI();
    }
#endif

    // If the caught class 2 interrupt includes mailbox interrupt, handle it.
    if ((stat & INTR_PPU_MB_MASK) == INTR_PPU_MB_MASK)
    {
#ifdef USE_ISOSELF
        ret = sys_isoself_spu_read_puint_mb(sa->id, &sa->error_code);
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("sys_isoself_spu_read_puint_mb failed %d\n", ret));
            sysMutexUnlock(sa->mmio_mutex);
            sysInterruptThreadEOI();
        }

        // Reset the PPU_INTR_MB interrupt status bit.
        ret = sys_isoself_spu_set_int_stat(sa->id, SPU_INTR_CLASS_2, INTR_PPU_MB_MASK);
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("sys_isoself_spu_set_int_stat failed %d\n", ret));
            sysMutexUnlock(sa->mmio_mutex);
            sysInterruptThreadEOI();
        }
#else
        ret = sysSpuRawReadPuintMb(sa->id, &sa->error_code);
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("sys_raw_spu_read_puint_mb failed %d\n", ret));
            sysMutexUnlock(sa->mmio_mutex);
            sysInterruptThreadEOI();
        }

        // Reset the PPU_INTR_MB interrupt status bit.
        ret = sysSpuRawSetIntStat(sa->id, SPU_INTR_CLASS_2, INTR_PPU_MB_MASK);
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("sys_raw_spu_set_int_stat failed %d\n", ret));
            sysMutexUnlock(sa->mmio_mutex);
            sysInterruptThreadEOI();
        }
#endif

        ret = sysMutexLock(sa->mmio_mutex, 0);
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("sysMutexLock() failed. (%d)\n", ret));
            sysInterruptThreadEOI();
        }

        ret = sysCondSignal(sa->mmio_cond);
        if (ret != 0)
        {
            sysMutexUnlock(sa->mmio_mutex);
            sysInterruptThreadEOI();
        }

        ret = sysMutexUnlock(sa->mmio_mutex);
        if (ret != 0)
        {
            sysInterruptThreadEOI();
        }
    }
    sysInterruptThreadEOI();
}

int exchange_data(int func_nr
                  , uint8_t *write_buffer, int write_count
                  , uint8_t *read_buffer, int read_count
                  , uint32_t timeout)
{
    int ret;
    
    if (sa == NULL)
    {
        return 0;
    }

    sa->error_code = 0xfffffff1;

    if (write_count > 0)
    {
        memcpy(sa->buffer, write_buffer, min(write_count, DMA_BUFFER_SIZE));
    }

    ret = sysMutexLock(sa->mmio_mutex, 0);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sysMutexLock() failed. (%d)\n", ret));
        return ret;
    }

    sysSpuRawWriteProblemStorage(sa->id, SPU_In_MBox, func_nr);
    sysSpuRawWriteProblemStorage(sa->id, SPU_In_MBox, read_count);

    ret = sysCondWait(sa->mmio_cond, timeout);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("exchange_data: [sysCondWait timeout]\n"));
        sysMutexUnlock(sa->mmio_mutex);
        return ret;
    }

    ret = sysMutexUnlock(sa->mmio_mutex);
    if (ret != 0)
    {
        return ret;
    }

    if (sa->error_code == 0xfffffff1)
    {
        return -1;
    }

    if (read_buffer != NULL && read_count > 0)
    {
        memcpy(read_buffer, sa->buffer, min(read_count, DMA_BUFFER_SIZE));
    }

    return sa->error_code;
}

int sac_exec_initialize(void)
{
    uint8_t buffer[1] = { 0 };
    return exchange_data(0, buffer, 1, 0, 0, 5000000);
}

int sac_exec_exit(void)
{
    return exchange_data(1, 0, 0, 0, 0, 5000000);
}

int sac_exec_generate_key_1(uint8_t *key, uint32_t expected_size, uint32_t *key_size)
{
    uint32_t new_key_size;
    uint8_t  buffer[0xd0];
    int      ret;

    memset(buffer, 0, 0xd0);

    ret = exchange_data(SAC_CMD_GENERATE_KEY_1, 0, 0, buffer, 0xd0, 5000000);

    new_key_size = buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
    if (new_key_size <= expected_size)
    {
        memcpy(key, buffer + 4, new_key_size);
        *key_size = new_key_size;

        return ret;
    }
    return -1;
}

int sac_exec_validate_key_1(uint8_t *key, uint32_t expected_size)
{
    int     ret;
    uint8_t buffer[0xd8];

    memset(buffer, 0, 0xd8);

    memcpy(buffer, &expected_size, 4);

    buffer[8]  = 0;
    buffer[9]  = 0;
    buffer[10] = 0;
    buffer[11] = 0;

    buffer[12] = 0;
    buffer[13] = 0;
    buffer[14] = 0;
    buffer[15] = 0;

    memcpy(buffer + 16, key, expected_size);

    ret = exchange_data(SAC_CMD_VALIDATE_KEY_1, buffer, 0xd8, 0, 0, 0);

    return ret;
}

int sac_exec_generate_key_2(uint8_t *key, uint32_t expected_size, uint32_t *key_size)
{
    uint32_t new_key_size;
    uint8_t  buffer[0xb4];
    int      ret;

    memset(buffer, 0, 0xb4);

    ret = exchange_data(SAC_CMD_GENERATE_KEY_2, 0, 0, buffer, 0xb4, 5000000);

    new_key_size = buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
    if (new_key_size <= expected_size)
    {
        memcpy(key, buffer + 4, new_key_size);
        *key_size = new_key_size;

        return ret;
    }
    return -1;
}

int sac_exec_validate_key_2(uint8_t *key, uint32_t expected_size)
{
    int     ret;
    uint8_t buffer[0xb4];

    memset(buffer, 0, 0xb4);
    memcpy(buffer, &expected_size, 4);
    memcpy(buffer + 4, key, expected_size);

    ret = exchange_data(SAC_CMD_VALIDATE_KEY_2, buffer, 0xb4, 0, 0, 5000000);

    return ret;
}

int sac_exec_validate_key_3(uint8_t *key, uint32_t expected_size)
{
    int     ret;
    uint8_t buffer[0x34];

    memset(buffer, 0, 0x34);
    memcpy(buffer, &expected_size, 4);
    memcpy(buffer + 4, key, expected_size);

    ret = exchange_data(SAC_CMD_VALIDATE_KEY_3, buffer, 0x34, 0, 0, 5000000);

    return ret;
}

int sac_exec_decrypt_data(uint8_t *encrypted_buffer, uint32_t expected_size, uint8_t *decrypted_buffer)
{
    int     ret;

    memset(sa->read_buffer, 0, 0x10);
    memcpy(sa->read_buffer, &expected_size, 4);
    memcpy(sa->read_buffer + 0x10, encrypted_buffer, expected_size);

    ret = exchange_data(SAC_CMD_DECRYPT, sa->read_buffer, expected_size + 0x10, sa->write_buffer, expected_size + 4, 5000000);

    memcpy(decrypted_buffer, sa->write_buffer + 4, expected_size);

    return ret;
}

int sac_exec_key_exchange(int fd)
{
    int      ret;
    uint8_t  agid;
    uint32_t buffer_size;
    uint8_t  buffer[256];
    memset(buffer, 0, 256);

    ret = ioctl_report_key_start(fd, buffer);
    LOG(lm_main, LOG_DEBUG, ("ioctl_report_key1[%x] %x\n", fd, ret));
    if (ret != 0)
    {
        return ret;
    }

    agid = buffer[7];

    ret = sac_exec_generate_key_1(buffer, 0xcc, &buffer_size);
    LOG(lm_main, LOG_DEBUG, ("sac_exec_generate_key 0 %x %x\n", buffer_size, ret));
    if (ret != 0)
    {
        return ret;
    }

    ret = ioctl_send_key(fd, agid, buffer_size, buffer, 2);
    LOG(lm_main, LOG_DEBUG, ("ioctl_send_key[2] %x %x\n", buffer_size, ret));
    if (ret != 0)
    {
        return ret;
    }

    buffer_size = 0xcc;
    ret         = ioctl_report_key(fd, agid, &buffer_size, buffer, 2);
    LOG(lm_main, LOG_DEBUG, ("ioctl_report_key[2] %x %x\n", buffer_size, ret));
    if (ret != 0)
    {
        return ret;
    }

    ret = sac_exec_validate_key_1(buffer, 0xc5);
    LOG(lm_main, LOG_DEBUG, ("sac_exec_validate_key_1[%x]\n", ret));
    if (ret != 0)
    {
        return ret;
    }

    // start from scratch
    memset(buffer, 0, 256);

    ret = sac_exec_generate_key_2(buffer, 0xb0, &buffer_size);
    LOG(lm_main, LOG_DEBUG, ("sac_exec_generate_key_2[%x] %x\n", buffer_size, ret));
    if (ret != 0)
    {
        return ret;
    }

    ret = ioctl_send_key(fd, agid, buffer_size, buffer, 3);
    LOG(lm_main, LOG_DEBUG, ("ioctl_send_key[3] %x %x\n", buffer_size, ret));
    if (ret != 0)
    {
        return ret;
    }

    ret = ioctl_report_key(fd, agid, &buffer_size, buffer, 3);
    LOG(lm_main, LOG_DEBUG, ("ioctl_report_key[3] %x %x\n", buffer_size, ret));
    if (ret != 0)
    {
        return ret;
    }

    ret = sac_exec_validate_key_2(buffer, 0xae);
    LOG(lm_main, LOG_DEBUG, ("sac_exec_validate_key_2[%x]\n", ret));
    if (ret != 0)
    {
        return ret;
    }

    // start from scratch
    memset(buffer, 0, 256);

    buffer_size = 0x30;
    ret         = ioctl_report_key(fd, agid, &buffer_size, buffer, 4);
    LOG(lm_main, LOG_DEBUG, ("ioctl_report_key[4] %x %x\n", buffer_size, ret));
    if (ret != 0)
    {
        return ret;
    }

    ret = sac_exec_validate_key_3(buffer, 0x30);
    LOG(lm_main, LOG_DEBUG, ("sac_exec_validate_key_3[%x]\n", ret));
    if (ret != 0)
    {
        return ret;
    }

    ret = ioctl_report_key_finish(fd, agid);
    LOG(lm_main, LOG_DEBUG, ("ioctl_report_finish [0xff] %x\n", ret));

    return ret;
}

#ifdef USE_ISOSELF

int file_alloc_load(const char *file_path, uint8_t **buf, unsigned int *size)
{
    int       ret, fd;
    sysFSStat status;
    uint64_t  read_length;

    ret = sysFsOpen(file_path, SYS_O_RDONLY, &fd, 0, 0);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("file %s open error : 0x%x\n", file_path, ret));
        return -1;
    }

    ret = sysFsFStat(fd, &status);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("file %s get stat error : 0x%x\n", file_path, ret));
        sysFsClose(fd);
        return -1;
    }

    *buf = malloc(status.st_size);
    if (*buf == NULL)
    {
        LOG(lm_main, LOG_ERROR, ("alloc failed\n"));
        sysFsClose(fd);
        return -1;
    }

    ret = sysFsRead(fd, *buf, status.st_size, &read_length);
    if (ret != 0 || status.st_size != read_length)
    {
        LOG(lm_main, LOG_ERROR, ("file %s read error : 0x%x\n", file_path, ret));
        sysFsClose(fd);
        free(*buf);
        *buf = NULL;
        return -1;
    }

    ret = sysFsClose(fd);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("file %s close error : 0x%x\n", file_path, ret));
        free(*buf);
        *buf = NULL;
        return -1;
    }

    *size = status.st_size;
    return 0;
}

#endif

#endif
