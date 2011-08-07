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


#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <logging.h>
#include "DSTDecoder.h"
#include "dst_decoder_ref.h"

#define DST_BUFFER_SIZE (MAX_DSDBITS_INFRAME / 8 * MAX_CHANNELS)
#define DSD_BUFFER_SIZE (MAX_DSDBITS_INFRAME / 8 * MAX_CHANNELS)

static int g_numslots = 0;
static uint8_t *g_dstbuf = NULL;
static uint8_t *g_dsdbuf = NULL;

void *dst_decoder_thread(void *threadarg)
{
    frame_slot_t *frame_slot = (frame_slot_t *)threadarg;
    for (;;)
    {
        pthread_mutex_lock(&frame_slot->put_mutex);
        frame_slot->state = SLOT_RUNNING;
        Decode(&frame_slot->D, frame_slot->dst_data, frame_slot->dsd_data, frame_slot->frame_nr, &frame_slot->dst_size);
        frame_slot->state = SLOT_READY;
        pthread_mutex_unlock(&frame_slot->get_mutex);
    }
    pthread_exit(NULL);
}

int dst_decoder_create_mt(dst_decoder_t **dst_decoder, int thread_count)
{
    *dst_decoder = (dst_decoder_t *)calloc(1, sizeof(dst_decoder_t));
    if (*dst_decoder == NULL)
    {
        LOG(lm_main, LOG_ERROR, ("Could not create dst_decoder object"));
        return -1;
    }
    (*dst_decoder)->frame_slots = (frame_slot_t *)calloc(thread_count, sizeof(frame_slot_t));
    if ((*dst_decoder)->frame_slots == NULL)
    {
        free(*dst_decoder);
        *dst_decoder = NULL;
        LOG(lm_main, LOG_ERROR, ("Could not create decoder slot array"));
        return -2;
    }
    (*dst_decoder)->channel_count = 0;
    (*dst_decoder)->thread_count  = thread_count;
    (*dst_decoder)->slot_nr       = 0;
    (*dst_decoder)->buf_nr        = 0;
    return 0;
}

int dst_decoder_destroy_mt(dst_decoder_t *dst_decoder)
{
    int i;

    for (i = 0; i < dst_decoder->thread_count; i++)
    {
        uint8_t *dsd_data;
        size_t dsd_size;

        dst_decoder_decode_mt(dst_decoder, NULL, 0, &dsd_data, &dsd_size);
    }
    for (i = 0; i < dst_decoder->thread_count; i++)
    {
        frame_slot_t *frame_slot = &dst_decoder->frame_slots[i];
        if (frame_slot->initialized)
        {
            if (dst_decoder->thread_count > 1)
            {
                pthread_cancel(frame_slot->thread);
                pthread_mutex_destroy(&frame_slot->get_mutex);
                pthread_mutex_destroy(&frame_slot->put_mutex);
            }
            if (Close(&frame_slot->D) == 0)
            {
            }
            else
            {
                LOG(lm_main, LOG_ERROR, ("Could not close decoder slot"));
            }
            frame_slot->initialized = 0;
        }
    }
    free(dst_decoder->frame_slots);
    free(dst_decoder);
    return 0;
}   

int dst_decoder_init_mt(dst_decoder_t *dst_decoder, int channel_count)
{
    int i;

    for (i = 0; i < dst_decoder->thread_count; i++)
    {
        frame_slot_t *frame_slot = &dst_decoder->frame_slots[i];
        if (!frame_slot->initialized)
        {
            if (Init(&frame_slot->D, channel_count, 64) == 0)
            {
                if (dst_decoder->thread_count > 1)
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

int dst_decoder_decode_mt(dst_decoder_t *dst_decoder, uint8_t *dst_data, size_t dst_size, uint8_t **dsd_data, size_t *dsd_size)
{
    frame_slot_t *frame_slot;

    /* Get current slot */
    frame_slot = &dst_decoder->frame_slots[dst_decoder->slot_nr];

    /* Allocate encoded frame into the slot */ 
    frame_slot->dsd_data = *dsd_data;
    frame_slot->dst_data = dst_data;
    frame_slot->dst_size = dst_size;
    frame_slot->frame_nr = dst_decoder->frame_nr;
    
    /* Release worker (decoding) thread on the loaded slot */
    if (dst_size > 0)
    {
        frame_slot->state = SLOT_LOADED;
        pthread_mutex_unlock(&frame_slot->put_mutex);
    }
    else
    {
        frame_slot->state = SLOT_EMPTY;
    }

    /* Advance to the next slot */
    dst_decoder->slot_nr = (dst_decoder->slot_nr + 1) % dst_decoder->thread_count;
    frame_slot = &dst_decoder->frame_slots[dst_decoder->slot_nr];

    /* Dump decoded frame */
    if (frame_slot->state != SLOT_EMPTY)
    {
        pthread_mutex_lock(&frame_slot->get_mutex);
        *dsd_data = frame_slot->dsd_data;
        *dsd_size = (size_t)(MAX_DSDBITS_INFRAME / 8 * dst_decoder->channel_count);
    }
    else
    {
        *dsd_data = frame_slot->dsd_data;
        *dsd_size = 0;
    }

    dst_decoder->frame_nr++;
    return 0;
}

int dst_decoder_create(dst_decoder_t **dst_decoder)
{
    g_numslots = pthread_num_processors_np();
    g_dstbuf = g_dsdbuf = NULL;

    // Allocate 1 more buffer than we have threads/slots
    if (g_numslots > 1)
    {
        g_dstbuf = (uint8_t*)malloc((g_numslots + 1) * DST_BUFFER_SIZE);
        g_dsdbuf = (uint8_t*)malloc((g_numslots + 1) * DSD_BUFFER_SIZE);
        if (g_dstbuf == NULL || g_dsdbuf == NULL)
        {
            LOG(lm_main, LOG_ERROR, ("Could not allocate memory for buffers"));
            return -1;
        }
    }
    return dst_decoder_create_mt(dst_decoder, g_numslots);
}

int dst_decoder_destroy(dst_decoder_t *dst_decoder)
{
    if (g_dstbuf)
    {
        free(g_dstbuf);
        g_dstbuf = NULL;
    }
    if (g_dsdbuf)
    {
        free(g_dsdbuf);
        g_dsdbuf = NULL;
    }
    return dst_decoder_destroy_mt(dst_decoder);
}

int dst_decoder_init(dst_decoder_t *dst_decoder, int channel_count)
{
    return dst_decoder_init_mt(dst_decoder, channel_count);
}

int dst_decoder_decode(dst_decoder_t *dst_decoder, uint8_t *dst_data, size_t dst_size, uint8_t *dsd_data, size_t *dsd_size)
{
    int rc;

    if (g_numslots > 1)
    {
        uint8_t *dst_ref, *dsd_ref;

        dst_ref = g_dstbuf + DST_BUFFER_SIZE * dst_decoder->buf_nr;
        dsd_ref = g_dsdbuf + DSD_BUFFER_SIZE * dst_decoder->buf_nr;
        dst_decoder->buf_nr = (++dst_decoder->buf_nr) % (g_numslots + 1);

        memcpy(dst_ref, dst_data, dst_size);
        rc = dst_decoder_decode_mt(dst_decoder, dst_ref, dst_size, &dsd_ref, dsd_size); 
        memcpy(dsd_data, dsd_ref, *dsd_size);
    }
    else if (dst_size > 0)
    {
        frame_slot_t *frame_slot = &dst_decoder->frame_slots[0];
        rc = Decode(&frame_slot->D, dst_data, dsd_data, frame_slot->frame_nr, &dst_size);
        *dsd_size = (size_t)(MAX_DSDBITS_INFRAME / 8 * dst_decoder->channel_count);
    }
    else
    {
        *dsd_size = 0;
        rc = 0;
    }

    return rc;
}
