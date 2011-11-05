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

#ifndef __DST_DECODER_H__
#define __DST_DECODER_H__

#include <pthread.h> 
#include <stdint.h>

#include "types.h"

enum slot_state_t {SLOT_EMPTY, SLOT_LOADED, SLOT_RUNNING, SLOT_READY};

typedef void (*frame_decoded_callback_t)(uint8_t* frame_data, size_t frame_size, void *userdata);

typedef struct buffer_slot_t
{
    uint8_t*                    buf;
    size_t                      size;
} 
buffer_slot_t;

typedef struct frame_slot_t
{
    int                         initialized;
    int                         frame_nr;
    ebunch                      D;
    buffer_slot_t              *dsd_data_slot;
    buffer_slot_t              *dst_data_slot;
    pthread_t                   thread;
    volatile int                state;
    pthread_mutex_t             get_mutex;
    pthread_mutex_t             put_mutex;
} 
frame_slot_t;

typedef struct dst_decoder_t
{
    frame_slot_t               *frame_slots;

    int                         frame_slot_count;
    int                         current_frame_slot;

    buffer_slot_t              *dsd_data_slots;
    buffer_slot_t              *dst_data_slots;

    int                         buffer_slot_count;
    int                         current_buffer_slot;

    int                         channel_count;
    uint32_t                    frame_nr;

    frame_decoded_callback_t    frame_decoded_callback;
    void                        *userdata;
} 
dst_decoder_t;

dst_decoder_t *dst_decoder_create(int channel_count, frame_decoded_callback_t frame_decoded_callback, void *userdata);
int dst_decoder_destroy(dst_decoder_t *dst_decoder);
int dst_decoder_decode(dst_decoder_t *dst_decoder, uint8_t *dst_data, size_t dst_size);

#endif // __DST_DECODER_H__
