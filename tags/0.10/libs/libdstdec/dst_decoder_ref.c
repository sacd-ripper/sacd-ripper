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


#include <malloc.h>
#include <stdio.h>
#include <logging.h>
#include "DSTDecoder.h"
#include "dst_decoder_ref.h"

int dst_decoder_create(dst_decoder_t **dst_decoder)
{
    *dst_decoder = (dst_decoder_t *)malloc(sizeof(dst_decoder_t));
    if (*dst_decoder == NULL)
    {
        LOG(lm_main, LOG_ERROR, ("Could not create dst_decoder_t object"));
        return -1;
    }
    return 0;
}

int dst_decoder_destroy(dst_decoder_t *dst_decoder)
{
    if (Close(&dst_decoder->D))
    {
        free(dst_decoder);
        return 0;
    }
    LOG(lm_main, LOG_ERROR, ("Could not close dst_decoder"));
    return -1;
}   

int dst_decoder_init(dst_decoder_t *dst_decoder, int channel_count)
{
    if (Init(&dst_decoder->D, channel_count, 64))
    {
        dst_decoder->frame_nr      = 0;
        dst_decoder->channel_count = channel_count;
        return 0;
    }
    LOG(lm_main, LOG_ERROR, ("Could not init dst_decoder"));
    return -1;
}

int dst_decoder_decode(dst_decoder_t *dst_decoder, uint8_t *source_data, size_t source_size, uint8_t *dest_data, size_t *dsd_size)
{
    if (Decode(&dst_decoder->D, source_data, dest_data, dst_decoder->frame_nr, &source_size))
    {
        *dsd_size = (size_t)((MAX_DSDBITS_INFRAME / 8) * dst_decoder->channel_count);
        dst_decoder->frame_nr++;
        return 0;
    }
    *dsd_size = (size_t)0;
    LOG(lm_main, LOG_ERROR, ("Could not decode frame %d", dst_decoder->frame_nr));
    return -1;
}
