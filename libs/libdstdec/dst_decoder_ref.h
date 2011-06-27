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

#include "types.h"

typedef struct _dst_decoder_t
{
    uint32_t  frame_nr;
    int       channel_count;
    ebunch    D;
    //DSTData_t DSTData;
}
dst_decoder_t;

int dst_decoder_create(dst_decoder_t **dst_decoder);
int dst_decoder_destroy(dst_decoder_t *dst_decoder);
int dst_decoder_init(dst_decoder_t *dst_decoder, int channel_count);
int dst_decoder_decode(dst_decoder_t *dst_decoder, uint8_t *source_data, size_t source_size, uint8_t *dest_data, size_t *dsd_size);

#endif // __DST_DECODER_H__
