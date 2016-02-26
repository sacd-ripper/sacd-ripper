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

#ifndef DST_DECODER_H
#define DST_DECODER_H

#include <stdint.h>

typedef struct dst_decoder_s dst_decoder_t;
typedef void (*frame_decoded_callback_t)(uint8_t* frame_data, size_t frame_size, void *userdata);
typedef void (*frame_error_callback_t)(int frame_count, int frame_error_code, const char *frame_error_message, void *userdata);

dst_decoder_t* dst_decoder_create(int channel_count, frame_decoded_callback_t frame_decoded_callback, frame_error_callback_t frame_error_callback, void *userdata);
void dst_decoder_destroy(dst_decoder_t *dst_decoder);
void dst_decoder_decode(dst_decoder_t *dst_decoder, uint8_t* frame_data, size_t frame_size);


#endif /* DST_DECODER_H */
