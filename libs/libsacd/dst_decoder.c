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

#ifndef __lv2ppu__
#error you need the psl1ght/lv2 ppu compatible compiler!
#endif

#include <sys/spu.h>
#include <sys/event_queue.h>

#include <stdlib.h>
#include <string.h>

#include <logging.h>
#include "dst_decoder.h"

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

typedef struct dst_command_t
{
    uint32_t src_addr;
    uint32_t dst_addr;
    uint32_t src_size;
    uint32_t dst_size;
    uint32_t dst_encoded;
    uint32_t channel_count;
    uint32_t reserved[2];

} 
dst_command_t;

struct dst_decoder_thread_s
{
    sys_spu_group_t         spu_group;      /* SPU thread group ID */
    sys_spu_thread_t        spu_thread;     /* SPU thread IDs */
    
    sysSpuImage             spu_img;
    int                     spu_id;

    sys_event_port_t        event_port;
    sys_event_queue_t       send_queue;

    uint8_t                 dst_buffer[0x7000]__attribute__ ((aligned(128)));

    dst_command_t           command __attribute__ ((aligned(128)));
};

static int create_dst_decoder_thread(dst_decoder_thread_t decoder, int spu_id, sys_event_queue_t recv_queue)
{
    sysSpuThreadGroupAttribute  group_attr;
    sysSpuThreadAttribute       thread_attr;
    sysSpuThreadArgument        thread_args;
    sys_event_queue_attr_t      queue_attr;
    int ret;
    
    decoder->spu_id = spu_id;

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
    
    group_attr.nameSize = 3 + 1;
    group_attr.nameAddress = (uint64_t)((void*)("DST"));
    group_attr.memContainer = 0;
    group_attr.groupType = 0;
    ret = sysSpuThreadGroupCreate(&decoder->spu_group, 
                                  1,
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
    thread_args.arg1 = ((uint64_t)(DST_QUEUE_NUMBER)) << 32;

    thread_attr.nameSize = 3 + 1;
    thread_attr.nameAddress = (uint64_t)((void*)("DST"));
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

    return 0;
}

static int destroy_dst_decoder_thread(dst_decoder_thread_t decoder)
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

    return 0;
}   

int create_dst_decoder(dst_decoder_t *dst_decoder)
{
    sys_event_queue_attr_t queue_attr;
    int ret, i;

    memset(&queue_attr, 0, sizeof(sys_event_queue_attr_t));
    queue_attr.attr_protocol = SYS_EVENT_QUEUE_PRIO;
    queue_attr.type = SYS_EVENT_QUEUE_PPU;
    ret = sysEventQueueCreate(&dst_decoder->recv_queue, &queue_attr, SYS_EVENT_QUEUE_KEY_LOCAL, DEFAULT_QUEUE_SIZE);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysEventQueueCreate failed: %#.8x", ret));
        return ret;
    }

    for (i = 0; i < NUM_DST_DECODERS; i++)
    {
        dst_decoder->decoder[i] = calloc(sizeof(dst_decoder_thread_t), 1);
        ret = create_dst_decoder_thread(dst_decoder->decoder[i], i, dst_decoder->recv_queue);
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("create_dst_decoder_thread failed: %#.8x", ret));
            return ret;
        }

        return 0;

        // we are expecting a start event
        dst_decoder->event_count++;
    }

    ret = dst_decoder_wait(dst_decoder, 500000);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("dst_decoder_wait failed: %#.8x", ret));
        return ret;
    }

    return 0;
}

int destroy_dst_decoder(dst_decoder_t *dst_decoder)
{
    int ret, i;
    
    for (i = 0; i < NUM_DST_DECODERS; i++)
    {
        ret = sysEventPortSend(dst_decoder->decoder[i]->event_port, EVENT_TERMINATE, 0, 0);
        if (ret != 0) 
        {
            LOG(lm_main, LOG_ERROR, ("sysEventPortSend failed: %#.8x", ret));
        }

        ret = destroy_dst_decoder_thread(dst_decoder->decoder[i]);
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("destroy_dst_decoder_thread failed: %#.8x", ret));
        }
        free(dst_decoder->decoder[i]);
    }

    ret = sysEventQueueDestroy(dst_decoder->recv_queue, 0);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysEventQueueDestroy failed: %#.8x", ret));
    }

    return 0;
}   

int prepare_dst_decoder(dst_decoder_t *dst_decoder)
{
    dst_decoder->event_count = 0;

    return 0;
}

int decode_dst_frame(dst_decoder_t *dst_decoder, uint8_t *src_data, size_t src_size, int dst_encoded, int channel_count)
{
    int ret;
    dst_decoder_thread_t decoder = dst_decoder->decoder[dst_decoder->event_count];

    decoder->command.src_addr = (uint32_t) (uint64_t) src_data;
    decoder->command.dst_addr = (uint32_t) (uint64_t) decoder->dst_buffer;
    decoder->command.src_size = src_size;
    decoder->command.dst_size = 0;
    decoder->command.dst_encoded = dst_encoded;
    decoder->command.channel_count = channel_count;

    ret = sysEventPortSend(decoder->event_port, EVENT_SEND_DST, (uint64_t) &decoder->command, 2);
    if (ret != 0) 
    {
        LOG(lm_main, LOG_ERROR, ("sysEventPortSend failed: %#.8x", ret));
    }
    dst_decoder->event_count++;
    if (dst_decoder->event_count > NUM_DST_DECODERS)
    {
        LOG(lm_main, LOG_ERROR, ("dst_decoder->event_count > NUM_DST_DECODERS"));
        dst_decoder->event_count = NUM_DST_DECODERS - 1;
        return -1;
    }

    return ret;
}

int dst_decoder_wait(dst_decoder_t *dst_decoder, int timeout)
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
            LOG(lm_main, LOG_ERROR, ("sysEventQueueReceive ok!: %#.8x", (event.data_2 & 0x00000000FFFFFFFF)));
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

int get_dsd_frame(dst_decoder_t *dst_decoder, int idx, uint8_t **dsd_data, size_t *dsd_size)
{
    if (idx < dst_decoder->event_count)
    {
        *dsd_data = (uint8_t *) (uint64_t) dst_decoder->decoder[idx]->command.dst_addr;
        *dsd_size = dst_decoder->decoder[idx]->command.dst_size;
        return 0;
    }

    return -1;
}
