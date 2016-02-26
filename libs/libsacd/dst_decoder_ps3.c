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

#ifdef __lv2ppu__

// If you remove the following line, it will compile but the DST decoded output from SACD Ripper will be incorrect!!
//#error the PS3 SACD Ripper code is currently broken in the trunk. For now I advice you to use the SACD Daemon in combination with sacd_extract.

#include <sys/spu.h>
#include <sys/event_queue.h>

#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <scarletbook.h>
#include <logging.h>
#include "dst_decoder_ps3.h"

enum 
{
    EVENT_SEND_DST          = 0x100001,
    EVENT_RECEIVE_STARTED   = 0x100080,
    EVENT_RECEIVE_DSD       = 0x100081,
    EVENT_RECEIVE_ERROR     = 0x1000f0
};

#define EVENT_TERMINATE                 0xffffdeadUL

#define PRIORITY                        100

#define DST_DECODER_FILENAME            ("/dev_hdd0/game/SACDRIP01/USRDIR/decoder.spu.elf")
#define DST_QUEUE_NUMBER                10

#define DEFAULT_QUEUE_SIZE              127

/*
Potentially we could send multiple DST commands at once, statistics show it doesn't really matter
it's slow anyway:

command[1]: Ave: 8315.19 usec
command[2]: Ave: 16555.41 usec, 1 frame = 8277.70
command[3]: Ave: 24696.88 usec, 1 frame == 8232.29
*/
#define NUM_DST_COMMANDS                1

typedef struct dst_command_t
{
    uint32_t source_addr;
    uint32_t dest_addr;
    uint32_t source_size;
    uint32_t dest_size;

    // params
    uint32_t dst_encoded;
    uint32_t channel_count;
    uint32_t reserved[10];
} 
__attribute__ ((packed)) dst_command_t;

struct dst_decoder_thread_s
{
    sys_spu_group_t         spu_group;      /* SPU thread group ID */
    sys_spu_thread_t        spu_thread;     /* SPU thread IDs */
    
    sysSpuImage             spu_img;
    int                     spu_id;

    sys_event_port_t        event_port;
    sys_event_queue_t       send_queue;

    uint8_t                *dsd_channel_data;
    uint8_t                *dst_channel_data;
    dst_command_t          *command;
};

static int dst_decoder_thread_create(dst_decoder_thread_t decoder, int spu_id, sys_event_queue_t recv_queue)
{
    sysSpuThreadGroupAttribute  group_attr;
    sysSpuThreadAttribute       thread_attr;
    sysSpuThreadArgument        thread_args;
    sys_event_queue_attr_t      queue_attr;
    int ret;
    
    decoder->spu_id = spu_id;

    memset(&queue_attr, 0, sizeof(sys_event_queue_attr_t));
    queue_attr.attr_protocol = SYS_EVENT_QUEUE_PRIO;
    queue_attr.type = SYS_EVENT_QUEUE_SPU;
    ret = sysEventQueueCreate(&decoder->send_queue, &queue_attr, SYS_EVENT_QUEUE_KEY_LOCAL, DEFAULT_QUEUE_SIZE);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysEventQueueCreate failed: %#.8x, %d", ret, spu_id));
        return ret;
    }

    ret = sysEventPortCreate(&decoder->event_port, SYS_EVENT_PORT_LOCAL, SYS_EVENT_PORT_NO_NAME);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysEventPortCreate failed: %#.8x, %d", ret, spu_id));
        return ret;
    }

    ret = sysEventPortConnectLocal(decoder->event_port, decoder->send_queue);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysEventPortConnectLocal failed: %#.8x, %d", ret, spu_id));
        return ret;
    }
    
    memset(&group_attr, 0, sizeof(sysSpuThreadGroupAttribute));
    group_attr.nameSize = 6 + 1;
    group_attr.nameAddress = (uint64_t)((void*)("DSTGRP"));
    ret = sysSpuThreadGroupCreate(&decoder->spu_group, 
                                  1, // 1 SPU per group
                                  PRIORITY, 
                                  &group_attr);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysSpuThreadGroupCreate failed: %#.8x, %d", ret, spu_id));
        return ret;
    }
    
    ret = sysSpuImageOpen(&decoder->spu_img, DST_DECODER_FILENAME);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysSpuImageOpen failed: %#.8x, %d", ret, spu_id));
        return ret;
    }

    /*
     * Pass the SPU queue number to the SPU thread as an argument.
     * This is used by the decoder to identify the event queue.
     */
    memset(&thread_args, 0, sizeof(sysSpuThreadArgument));
    thread_args.arg0 = ((uint64_t)(DST_QUEUE_NUMBER)) << 32;

    memset(&thread_attr, 0, sizeof(sysSpuThreadAttribute));
    thread_attr.nameSize = 6 + 1;
    thread_attr.nameAddress = (uint64_t)((void*)("DSTTHR"));
    thread_attr.attribute = SPU_THREAD_ATTR_NONE;

    ret = sysSpuThreadInitialize(&decoder->spu_thread,
                                 decoder->spu_group,
                                 decoder->spu_id,
                                 &decoder->spu_img,
                                 &thread_attr,
                                 &thread_args);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysSpuThreadInitialize failed: %#.8x, %d", ret, spu_id));
        return ret;
    }

    ret = sysSpuThreadConnectEvent(decoder->spu_thread, recv_queue, 
                                   SPU_THREAD_EVENT_USER,
                                   DST_QUEUE_NUMBER);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysSpuThreadConnectEvent failed: %#.8x, %d", ret, spu_id));
        return ret;
    }

    ret = sysSpuThreadBindQueue(decoder->spu_thread, decoder->send_queue, DST_QUEUE_NUMBER);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysSpuThreadBindQueue failed: %#.8x, %d", ret, spu_id));
        return ret;
    }

    ret = sysSpuThreadGroupStart(decoder->spu_group);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysSpuThreadGroupStart failed: %#.8x, %d", ret, spu_id));
        return ret;
    }

    decoder->dsd_channel_data = (uint8_t *) memalign(128, FRAME_SIZE_64 * MAX_CHANNEL_COUNT);
    decoder->dst_channel_data = (uint8_t *) memalign(128, FRAME_SIZE_64 * MAX_CHANNEL_COUNT);
    decoder->command = (dst_command_t *) memalign(128, sizeof(dst_command_t) * NUM_DST_COMMANDS);

    return 0;
}

static int dst_decoder_thread_destroy(dst_decoder_thread_t decoder)
{
    uint32_t cause, status;
    int ret, spu_id;

    spu_id = decoder->spu_id;

    ret = sysSpuThreadGroupJoin(decoder->spu_group, &cause, &status);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysSpuThreadGroupJoin failed: %#.8x, %d", ret, spu_id));
    }

    ret = sysSpuThreadDisconnectEvent(decoder->spu_thread,
                                      SPU_THREAD_EVENT_USER,
                                      DST_QUEUE_NUMBER);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysSpuThreadDisconnectEvent() failed: %#.8x, %d", ret, spu_id));
    }

    ret = sysSpuThreadUnbindQueue(decoder->spu_thread, DST_QUEUE_NUMBER);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysSpuThreadUnbindQueue(%u, %u) failed: %#.8x, %d",
                decoder->spu_thread, DST_QUEUE_NUMBER, ret, spu_id));
    }
    
    ret = sysSpuThreadGroupDestroy(decoder->spu_group);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysSpuThreadGroupDestroy() failed: %#.8x, %d", ret, spu_id));
    }

    ret = sysEventPortDisconnect(decoder->event_port);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysEventPortDisconnect() failed: %#.8x, %d", ret, spu_id));
    }

    ret = sysEventPortDestroy(decoder->event_port);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysEventPortDestroy() failed: %#.8x, %d", ret, spu_id));
    }

    ret = sysEventQueueDestroy(decoder->send_queue, 0);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysEventQueueDestroy failed: %#.8x, %d", ret, spu_id));
    }

    ret = sysSpuImageClose(&decoder->spu_img);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysSpuImageClose failed: %.8x, %d", ret, spu_id));
    }

    if (decoder->dsd_channel_data)
        free(decoder->dsd_channel_data);

    if (decoder->dst_channel_data)
        free(decoder->dst_channel_data);

    if (decoder->command)
        free(decoder->command);

    return 0;
}   

static int dst_decoder_wait(dst_decoder_t *dst_decoder, int timeout)
{
    sys_event_t event;
    int ret;
    int errors = 0;
    int event_count = dst_decoder->event_count;

    while (event_count > 0)
    {
        ret = sysEventQueueReceive(dst_decoder->recv_queue, &event, timeout);
        if (ret != 0) 
        {
            LOG(lm_main, LOG_ERROR, ("sysEventQueueReceive failed: %#.8x", ret));
            errors++;
        } 
        else 
        {
            switch(event.data_2 & 0x00000000FFFFFFFF)
            {
            case EVENT_RECEIVE_STARTED:
            case EVENT_RECEIVE_DSD:
                break;
            case EVENT_RECEIVE_ERROR:
                errors++;
                break;
            }
        }
        event_count--;
    }
    return errors;
}

static void process_dst_frames(dst_decoder_t *dst_decoder)
{
    if (dst_decoder->event_count > 0)
    {
        int current_event = 0;
    
        // wait for all frames to be decoded (decoding takes around 0.03 seconds per frame)
        dst_decoder_wait(dst_decoder, 500000);
    
        while (current_event < dst_decoder->event_count)
        {
            int channel, i;
            uint8_t *dsd_data;
            dst_decoder_thread_t decoder = dst_decoder->decoder[current_event];
            dst_command_t *command = decoder->command;
    
            dsd_data = dst_decoder->dsd_data;
    
            // DSD data is stored sequential per channel, now we need to interleave it..
            for (i = 0; i < FRAME_SIZE_64; i++)
            {
                for (channel = 0; channel < command->channel_count; channel++)
                {
                    *dsd_data = *(decoder->dsd_channel_data + i + channel * FRAME_SIZE_64);
                    ++dsd_data;
                }
            }
    
            dst_decoder->frame_decoded_callback(dst_decoder->dsd_data, command->dest_size, dst_decoder->userdata);
    
            current_event++;
        }
    
        dst_decoder->event_count = 0;
    }
}

dst_decoder_t* dst_decoder_create(int channel_count, frame_decoded_callback_t frame_decoded_callback, frame_error_callback_t frame_error_callback, void *userdata)
{
    sys_event_queue_attr_t queue_attr;
    dst_decoder_t *dst_decoder;
    int i, ret;

    dst_decoder = (dst_decoder_t *) calloc(sizeof(dst_decoder_t), 1);
    dst_decoder->channel_count = channel_count;
    dst_decoder->frame_decoded_callback = frame_decoded_callback;
    dst_decoder->frame_error_callback = frame_error_callback;
    dst_decoder->userdata = userdata;
    dst_decoder->dsd_data = (uint8_t *) memalign(128, FRAME_SIZE_64 * MAX_CHANNEL_COUNT);

    memset(&queue_attr, 0, sizeof(sys_event_queue_attr_t));
    queue_attr.attr_protocol = SYS_EVENT_QUEUE_PRIO;
    queue_attr.type = SYS_EVENT_QUEUE_PPU;
    ret = sysEventQueueCreate(&dst_decoder->recv_queue, &queue_attr, SYS_EVENT_QUEUE_KEY_LOCAL, DEFAULT_QUEUE_SIZE);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysEventQueueCreate failed: %#.8x", ret));
        return 0;
    }

    dst_decoder->event_count = 0;
    for (i = 0; i < NUM_DST_DECODERS; i++)
    {
        dst_decoder->decoder[i] = calloc(sizeof(struct dst_decoder_thread_s), 1);
        ret = dst_decoder_thread_create(dst_decoder->decoder[i], i, dst_decoder->recv_queue);
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("dst_decoder_thread_create failed: %#.8x", ret));
            return 0;
        }

        // we are expecting a start event
        dst_decoder->event_count++;
    }

    // prefetch all the start events..
    ret = dst_decoder_wait(dst_decoder, 500000);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("dst_decoder_wait failed: %#.8x", ret));
        return 0;
    }

    // clear event count, they have been handled..
    dst_decoder->event_count = 0;

    return dst_decoder;
}

int dst_decoder_destroy(dst_decoder_t *dst_decoder)
{
    int ret, i;

    if (!dst_decoder)
    {
        return -1;
    }

    process_dst_frames(dst_decoder);
    
    for (i = 0; i < NUM_DST_DECODERS; i++)
    {
        ret = sysEventPortSend(dst_decoder->decoder[i]->event_port, EVENT_TERMINATE, 0, 0);
        if (ret != 0) 
        {
            LOG(lm_main, LOG_ERROR, ("sysEventPortSend failed: %#.8x", ret));
        }

        ret = dst_decoder_thread_destroy(dst_decoder->decoder[i]);
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("dst_decoder_thread_destroy failed: %#.8x", ret));
        }
        free(dst_decoder->decoder[i]);
    }

    ret = sysEventQueueDestroy(dst_decoder->recv_queue, 0);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysEventQueueDestroy failed: %#.8x", ret));
    }

    free(dst_decoder->dsd_data);
    free(dst_decoder);

    return 0;
}   

int dst_decoder_decode(dst_decoder_t *dst_decoder, uint8_t *dst_data, size_t dst_size)
{
    int ret;
    dst_decoder_thread_t decoder;

    if (dst_decoder->event_count == NUM_DST_DECODERS)
    {
        process_dst_frames(dst_decoder);
    }

    decoder = dst_decoder->decoder[dst_decoder->event_count];
    
    memcpy(decoder->dst_channel_data, dst_data, dst_size);

    memset(decoder->command, 0, sizeof(dst_command_t));
    decoder->command->source_addr = (uint32_t) (uint64_t) decoder->dst_channel_data;
    decoder->command->dest_addr = (uint32_t) (uint64_t) decoder->dsd_channel_data;
    decoder->command->source_size = dst_size;
    decoder->command->dest_size = 0;
    decoder->command->dst_encoded = 1;
    decoder->command->channel_count = dst_decoder->channel_count;

    // send the DST conversion command
    ret = sysEventPortSend(decoder->event_port, EVENT_SEND_DST, (uint64_t) decoder->command, NUM_DST_COMMANDS);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysEventPortSend failed: %#.8x", ret));
    }

    // event count should be smaller or equal to number of decoders
    dst_decoder->event_count++;
    if (dst_decoder->event_count > NUM_DST_DECODERS)
    {
        LOG(lm_main, LOG_ERROR, ("dst_decoder->event_count > NUM_DST_DECODERS"));
        dst_decoder->event_count = NUM_DST_DECODERS - 1;
        return -1;
    }

    return ret;
}

#endif
