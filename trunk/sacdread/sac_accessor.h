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

#ifndef __SAC_ACCESSOR_H__
#define __SAC_ACCESSOR_H__

#include <stdint.h>

#include <pthread.h>
#include <libspe2.h> 

/** configuration of sac accessor thread */
#define EIEIO __asm__ volatile("eieio")
#define SAC_MODULE_LOCATION "sac_module.spu.elf"
#define EXIT_SAC_CMD ( 0xFEFEFEFF )
#define DMA_SIZE (1024 * 4)

enum {
    SAC_INITIALIZE = 0
    , SAC_EXIT = 1
    , SAC_GENERATE_KEY_1 = 2
    , SAC_VALIDATE_KEY_1 = 3
    , SAC_GENERATE_KEY_2 = 4
    , SAC_VALIDATE_KEY_2 = 5
    , SAC_VALIDATE_KEY_3 = 6

} encoding_t;

typedef struct {
    uint8_t *buffer;

    pthread_t ppe_tid;

    pthread_cond_t mmio_cond;
    pthread_mutex_t mmio_mutex;

    spe_context_ptr_t spe;
    pthread_t spe_tid;

    spe_event_handler_ptr_t evhandler;
    spe_event_unit_t event;

    uint32_t read_count;
} sac_accessor_t;

extern sac_accessor_t *create_sac_accessor(void);
extern int destroy_sac_accessor(sac_accessor_t *);
extern int exchange_data(sac_accessor_t *, int, uint8_t *, int, uint8_t *, int, int);

#endif /* __SAC_ACCESSOR_H__ */
