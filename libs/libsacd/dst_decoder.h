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

#ifndef __lv2ppu__
#error you need the psl1ght/lv2 ppu compatible compiler!
#endif

#include <ppu-types.h>

typedef struct dst_decoder_thread_s *dst_decoder_thread_t;

#define NUM_DST_DECODERS                5 /* The number of DST decoders (SPUs) */ 

typedef struct dst_decoder_t
{
    sys_event_queue_t               recv_queue;

    int                             event_count;

    int                             current_event;

    dst_decoder_thread_t            decoder[NUM_DST_DECODERS];
}
dst_decoder_t;

int create_dst_decoder(dst_decoder_t *);
int destroy_dst_decoder(dst_decoder_t *);
int decode_dst_frame(dst_decoder_t *, uint8_t *, size_t, int, int);
int prepare_dst_decoder(dst_decoder_t *);
int dst_decoder_wait(dst_decoder_t *, int);
int get_dsd_frame(dst_decoder_t *, uint8_t *, size_t *);

#endif /* __DST_DECODER_H__ */
