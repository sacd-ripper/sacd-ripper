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

#ifdef __lv2ppu__
#error this code is not intended for the ps3
#endif

#include <stdio.h>
#include <malloc.h>
#include <memory.h>

#include <logging.h>

#include "dst_decoder.h"
#include "dst_fram.h"
#include "dst_init.h"

#define DST_BUFFER_SIZE (MAX_DSDBITS_INFRAME / 8 * MAX_CHANNELS)
#define DSD_BUFFER_SIZE (MAX_DSDBITS_INFRAME / 8 * MAX_CHANNELS)

#define CACHE_ALIGNMENT 64

void *dst_decoder_thread(void *threadarg)
{
    frame_slot_t *frame_slot = (frame_slot_t *)threadarg;
    for (;;)
    {
        pthread_mutex_lock(&frame_slot->put_mutex);
        frame_slot->state = SLOT_RUNNING;
        DST_FramDSTDecode(frame_slot->dst_data_slot->buf, frame_slot->dsd_data_slot->buf, frame_slot->dst_data_slot->size, frame_slot->frame_nr, &frame_slot->D);
        frame_slot->dsd_data_slot->size = (size_t)(MAX_DSDBITS_INFRAME / 8 * frame_slot->D.FrameHdr.NrOfChannels);

        frame_slot->state = SLOT_READY;
        pthread_mutex_unlock(&frame_slot->get_mutex);
    }
    pthread_exit(NULL);
}

static int dst_decoder_init(dst_decoder_t *dst_decoder, int channel_count)
{
    int i;

    for (i = 0; i < dst_decoder->frame_slot_count; i++)
    {
        frame_slot_t *frame_slot = &dst_decoder->frame_slots[i];
        if (!frame_slot->initialized)
        {
            if (DST_InitDecoder(&frame_slot->D, channel_count, 64) == 0)
            {
                if (dst_decoder->frame_slot_count > 1)
                {
                    pthread_mutex_init(&frame_slot->get_mutex, NULL);
                    pthread_mutex_lock(&frame_slot->get_mutex);
                    pthread_mutex_init(&frame_slot->put_mutex, NULL);
                    pthread_mutex_lock(&frame_slot->put_mutex);
                    pthread_create(&frame_slot->thread, NULL, dst_decoder_thread, (void *)frame_slot);
                }
            }
            else
            {
                LOG(lm_main, LOG_ERROR, ("Could not initialize decoder slot"));
                return -1;
            }
            frame_slot->initialized = 1;
        }
    }
    dst_decoder->channel_count = channel_count;
    dst_decoder->frame_nr      = 0;
    return 0;
}

dst_decoder_t *dst_decoder_create(int channel_count, frame_decoded_callback_t frame_decoded_callback, void *userdata)
{
    dst_decoder_t *dst_decoder;
    int i;
    
    dst_decoder = (dst_decoder_t *)calloc(1, sizeof(dst_decoder_t));
    if (!dst_decoder)
        return 0;

    dst_decoder->frame_slot_count = pthread_num_processors_np();
    dst_decoder->buffer_slot_count = dst_decoder->frame_slot_count + 1;
    dst_decoder->frame_decoded_callback = frame_decoded_callback;
    dst_decoder->userdata = userdata;

    dst_decoder->frame_slots = (frame_slot_t *) calloc(dst_decoder->frame_slot_count, sizeof(frame_slot_t));
    dst_decoder->dsd_data_slots = (buffer_slot_t *) calloc(dst_decoder->buffer_slot_count, sizeof(buffer_slot_t));
    dst_decoder->dst_data_slots = (buffer_slot_t *) calloc(dst_decoder->buffer_slot_count, sizeof(buffer_slot_t));

    for (i = 0; i < dst_decoder->buffer_slot_count; i++)
    {
#ifdef _WIN32
        dst_decoder->dst_data_slots[i].buf = _aligned_malloc(DST_BUFFER_SIZE, CACHE_ALIGNMENT);
        dst_decoder->dsd_data_slots[i].buf = _aligned_malloc(DSD_BUFFER_SIZE, CACHE_ALIGNMENT);
#else
        dst_decoder->dst_data_slots[i].buf = memalign(CACHE_ALIGNMENT, DST_BUFFER_SIZE);
        dst_decoder->dsd_data_slots[i].buf = memalign(CACHE_ALIGNMENT, DSD_BUFFER_SIZE);
#endif 
    }

    dst_decoder->channel_count = 0;
    dst_decoder->current_frame_slot = 0;
    dst_decoder->current_buffer_slot = 0;

    dst_decoder_init(dst_decoder, channel_count);

    return dst_decoder;
}

int dst_decoder_destroy(dst_decoder_t *dst_decoder)
{
    int i;

    /* empty the filled slots */
    for (i = 0; i < dst_decoder->frame_slot_count; i++)
    {
        dst_decoder_decode(dst_decoder, NULL, 0);
    }
    for (i = 0; i < dst_decoder->frame_slot_count; i++)
    {
        frame_slot_t *frame_slot = &dst_decoder->frame_slots[i];
        if (frame_slot->initialized)
        {
            if (dst_decoder->frame_slot_count > 1)
            {
                pthread_cancel(frame_slot->thread);
                pthread_mutex_destroy(&frame_slot->get_mutex);
                pthread_mutex_destroy(&frame_slot->put_mutex);
            }
            if (DST_CloseDecoder(&frame_slot->D) != 0)
            {
                LOG(lm_main, LOG_ERROR, ("Could not close decoder slot"));
            }
            frame_slot->initialized = 0;
        }
    }
    for (i = 0; i < dst_decoder->buffer_slot_count; i++)
    {
#ifdef _WIN32
        _aligned_free(dst_decoder->dst_data_slots[i].buf);
        _aligned_free(dst_decoder->dsd_data_slots[i].buf);
#else
        free(dst_decoder->dst_data_slots[i].buf);
        free(dst_decoder->dsd_data_slots[i].buf);
#endif 
    }

    free(dst_decoder->dst_data_slots);
    free(dst_decoder->dsd_data_slots);
    free(dst_decoder->frame_slots);
    free(dst_decoder);

    return 0;
}   

int dst_decoder_decode(dst_decoder_t *dst_decoder, uint8_t *dst_data, size_t dst_size)
{
    frame_slot_t *frame_slot;

    /* Get current slot */
    frame_slot = &dst_decoder->frame_slots[dst_decoder->current_frame_slot];
 
    /* Release worker (decoding) thread on the loaded slot */
    if (dst_size > 0)
    {
        frame_slot->state = SLOT_LOADED;

        /* advance buffer slot */
        dst_decoder->current_buffer_slot = (++dst_decoder->current_buffer_slot) % dst_decoder->buffer_slot_count;

        /* current frame_slot gets new dsd/dst buffer */
        frame_slot->dsd_data_slot = &dst_decoder->dsd_data_slots[dst_decoder->current_buffer_slot];
        frame_slot->dst_data_slot = &dst_decoder->dst_data_slots[dst_decoder->current_buffer_slot];

        // TODO, this extra copy could be avoided..
        memcpy(frame_slot->dst_data_slot->buf, dst_data, dst_size);
        frame_slot->dst_data_slot->size = dst_size;

        pthread_mutex_unlock(&frame_slot->put_mutex);
    }
    else
    {
        frame_slot->state = SLOT_EMPTY;
    }

    /* Advance to the next slot */
    dst_decoder->current_frame_slot = (dst_decoder->current_frame_slot + 1) % dst_decoder->frame_slot_count;
    frame_slot = &dst_decoder->frame_slots[dst_decoder->current_frame_slot];

    /* Dump decoded frame */
    if (frame_slot->state != SLOT_EMPTY)
    {
        pthread_mutex_lock(&frame_slot->get_mutex);

        dst_decoder->frame_decoded_callback(frame_slot->dsd_data_slot->buf, frame_slot->dsd_data_slot->size, dst_decoder->userdata);
    }

    dst_decoder->frame_nr++;
    return 0;
}
