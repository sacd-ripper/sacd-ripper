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

#ifndef _WIN32
#error you need a compiler with a windows target to use this
#endif

#ifndef __DST_DECODER_H__
#define __DST_DECODER_H__

#include <pthread.h> 
#include "types.h"

enum slot_state_t {SLOT_EMPTY, SLOT_LOADED, SLOT_RUNNING, SLOT_READY};

typedef struct _frame_slot_t
{
    int             initialized;
    int             frame_nr;
    uint8_t*        dsd_data;
    uint8_t*        dst_data;
    size_t          dst_size;
    ebunch          D;
    pthread_t       thread;
    volatile int    state;
    pthread_mutex_t get_mutex;
    pthread_mutex_t put_mutex;
} frame_slot_t;

typedef struct _dst_decoder_t
{
    frame_slot_t *frame_slots;
    int          slot_nr;       
    int          channel_count;
    int          thread_count;
    uint32_t     frame_nr;
} dst_decoder_t;

int dst_decoder_create_mt(dst_decoder_t **dst_decoder, int thread_count);
int dst_decoder_destroy_mt(dst_decoder_t *dst_decoder);
int dst_decoder_init_mt(dst_decoder_t *dst_decoder, int channel_count);
int dst_decoder_decode_mt(dst_decoder_t *dst_decoder, uint8_t *dst_data, size_t dst_size, uint8_t **dsd_data, size_t *dsd_size);

int dst_decoder_create(dst_decoder_t **dst_decoder);
int dst_decoder_destroy(dst_decoder_t *dst_decoder);
int dst_decoder_init(dst_decoder_t *dst_decoder, int channel_count);
int dst_decoder_decode(dst_decoder_t *dst_decoder, uint8_t *dst_data, size_t dst_size, uint8_t *dsd_data, size_t *dsd_size);

#endif // __DST_DECODER_H__
