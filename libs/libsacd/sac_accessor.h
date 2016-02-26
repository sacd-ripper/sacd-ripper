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

#ifndef __SAC_ACCESSOR_H__
#define __SAC_ACCESSOR_H__

#ifdef __lv2ppu__

#include <ppu-types.h>
#include <sys/spu.h>
#include <sys/interrupt.h>

//#define USE_ISOSELF 1

#ifdef USE_ISOSELF
#include <sys/isoself.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

#define INTR_PPU_MB_SHIFT              0
#define INTR_STOP_SHIFT                1
#define INTR_HALT_SHIFT                2
#define INTR_DMA_SHIFT                 3
#define INTR_SPU_MB_SHIFT              4
#define INTR_PPU_MB_MASK               (0x1 << INTR_PPU_MB_SHIFT)
#define INTR_STOP_MASK                 (0x1 << INTR_STOP_SHIFT)
#define INTR_HALT_MASK                 (0x1 << INTR_HALT_SHIFT)
#define INTR_DMA_MASK                  (0x1 << INTR_DMA_SHIFT)
#define INTR_SPU_MB_MASK               (0x1 << INTR_SPU_MB_SHIFT)

#define SPU_INTR_CLASS_2               (2)

#define EIEIO                          __asm__ volatile ("eieio")

/** configuration of sac accessor thread */
#define PRIMARY_PPU_THREAD_PRIO        (1001)
#define PRIMARY_PPU_STACK_SIZE         (0x2000)

#define DMA_BUFFER_SIZE                (4 * 2048)
#define EXIT_SAC_CMD                   (0xfefefeff)

#ifdef USE_ISOSELF
        #define SAC_MODULE_LOCATION    "/dev_flash/vsh/module/SacModule.spu.isoself"
#else
        #define SAC_MODULE_LOCATION    "/dev_hdd0/game/SACDRIP01/USRDIR/sac_module.spu.elf"
#endif

enum
{
      SAC_CMD_INITIALIZE     = 0
    , SAC_CMD_EXIT           = 1
    , SAC_CMD_GENERATE_KEY_1 = 2
    , SAC_CMD_VALIDATE_KEY_1 = 3
    , SAC_CMD_GENERATE_KEY_2 = 4
    , SAC_CMD_VALIDATE_KEY_2 = 5
    , SAC_CMD_VALIDATE_KEY_3 = 6
    , SAC_CMD_ENCRYPT        = 7
    , SAC_CMD_DECRYPT        = 8
} sac_command_t;

typedef struct
{
    uint8_t                       *buffer;

    uint8_t                       *module_buffer;
    unsigned int                  module_size;

    sys_ppu_thread_t              handler;

    sys_cond_t                    mmio_cond;
    sys_mutex_t                   mmio_mutex;

    sys_raw_spu_t                 id;
    sys_interrupt_thread_handle_t ih;
    sys_interrupt_tag_t           intrtag;

    uint32_t                      error_code;
    
    uint8_t                     * read_buffer;
    uint8_t                     * write_buffer;
} sac_accessor_t;

extern int create_sac_accessor(void);
extern int destroy_sac_accessor(void);
extern int sac_exec_initialize(void);
extern int sac_exec_key_exchange(int);
extern int sac_exec_decrypt_data(uint8_t *, uint32_t, uint8_t *);
extern int sac_exec_exit(void);

#ifdef __cplusplus
};
#endif

#endif

#endif /* __SAC_ACCESSOR_H__ */
