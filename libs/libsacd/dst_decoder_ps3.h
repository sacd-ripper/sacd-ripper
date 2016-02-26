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

#ifndef __DST_DECODER_PS3_H__
#define __DST_DECODER_PS3_H__

#ifdef __lv2ppu__

#include <ppu-types.h>

typedef void (*frame_decoded_callback_t)(uint8_t* frame_data, size_t frame_size, void *userdata);
typedef void (*frame_error_callback_t)(int frame_count, int frame_error_code, const char *frame_error_message, void *userdata);
typedef struct dst_decoder_thread_s *dst_decoder_thread_t;

#define NUM_DST_DECODERS                5 /* The number of DST decoders (SPUs) */ 

typedef struct dst_decoder_t
{
    sys_event_queue_t               recv_queue;

    int                             event_count;

    int                             channel_count;

    dst_decoder_thread_t            decoder[NUM_DST_DECODERS];

    uint8_t                        *dsd_data;

    frame_decoded_callback_t        frame_decoded_callback;
    frame_error_callback_t          frame_error_callback;
    void                           *userdata;
}
dst_decoder_t;

dst_decoder_t* dst_decoder_create(int channel_count, frame_decoded_callback_t frame_decoded_callback, frame_error_callback_t frame_error_callback, void *userdata);
int dst_decoder_destroy(dst_decoder_t *dst_decoder);
int dst_decoder_decode(dst_decoder_t *dst_decoder, uint8_t* frame_data, size_t frame_size);

#endif

#endif /* __DST_DECODER_H__ */
