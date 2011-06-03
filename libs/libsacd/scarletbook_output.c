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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <inttypes.h>
#include <malloc.h>
#include <pthread.h>
#ifdef __lv2ppu__
#include <sys/file.h>
#elif defined(WIN32)
#include <io.h>
#endif

#include <utils.h>
#include <logging.h>

#include "scarletbook_output.h"
#include "sacd_reader.h"

// TODO: allocate dynamically
static scarletbook_output_t output;

extern scarletbook_format_handler_t const * dsdiff_format_fn(void);
extern scarletbook_format_handler_t const * dsf_format_fn(void);
extern scarletbook_format_handler_t const * iso_format_fn(void);

typedef const scarletbook_format_handler_t *(*sacd_output_format_fn_t)(void); 
static sacd_output_format_fn_t s_sacd_output_format_fns[] = 
{
    dsdiff_format_fn,
    dsf_format_fn,
    iso_format_fn,
    NULL
}; 

scarletbook_format_handler_t const * find_output_format(char const * name)
{
    int i;
    for (i = 0; s_sacd_output_format_fns[i]; ++i) 
    {
        scarletbook_format_handler_t const * handler = s_sacd_output_format_fns[i]();
        if (!strcasecmp(handler->name, name)) 
        {
            return handler;
        }
    }
    return NULL;
} 

static void destroy_ripping_queue()
{
    if (output.initialized)
    {
        struct list_head * node_ptr;
        scarletbook_output_format_t * output_format_ptr;

        while (!list_empty(&output.ripping_queue))
        {
            node_ptr = output.ripping_queue.next;
            output_format_ptr = list_entry(node_ptr, scarletbook_output_format_t, siblings);
            list_del(node_ptr);
            free(output_format_ptr);
        }
    }
}

int queue_track_to_rip(int area, int track, char *file_path, char *fmt, 
                                uint32_t start_lsn, uint32_t length_lsn, int dst_encoded)
{
    scarletbook_format_handler_t const * handler;
    scarletbook_output_format_t * output_format_ptr;

    if ((handler = find_output_format(fmt)))
    {
        output_format_ptr = calloc(sizeof(scarletbook_output_format_t), 1);
        output_format_ptr->area = area;
        output_format_ptr->track = track;
        output_format_ptr->handler = *handler;
        output_format_ptr->filename = strdup(file_path);
        output_format_ptr->start_lsn = start_lsn;
        output_format_ptr->length_lsn = length_lsn;
        output_format_ptr->dst_encoded = dst_encoded;
        list_add_tail(&output_format_ptr->siblings, &output.ripping_queue);

        return 0;
    }
    return -1;
}

static int create_output_file(scarletbook_output_format_t *ft)
{
    int result;

#ifdef __lv2ppu__
    if (sysFsOpen(ft->filename, SYS_O_RDWR | SYS_O_CREAT | SYS_O_TRUNC, &ft->fd, 0, 0) != 0)
    {
        fprintf(stderr, "error creating %s", ft->filename);
        goto error;
    }
#else
    ft->fd = open(ft->filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0666);
    if (ft->fd == -1)
    {
        fprintf(stderr, "error creating %s", ft->filename);
        goto error;
    }
#endif

    ft->priv = calloc(1, ft->handler.priv_size);

    result = ft->handler.startwrite ? (*ft->handler.startwrite)(ft) : 0;

    return result;

error:
    if (ft->fd)
#ifdef __lv2ppu__
        sysFsClose(ft->fd);
#else
        close(ft->fd);
#endif
    free(ft->priv);
    free(ft);
    return -1;
}

static inline int close_output_file(scarletbook_output_format_t * ft)
{
    int result;

    result = ft->handler.stopwrite ? (*ft->handler.stopwrite)(ft) : 0;

    if (ft->fd)
#ifdef __lv2ppu__
        sysFsClose(ft->fd);
#else
        close(ft->fd);
#endif
    return result;
}

static inline void destroy_output_format(scarletbook_output_format_t * ft)
{
    free(ft->filename);
    free(ft->priv);
    free(ft);
}

void init_stats(stats_callback_t cb)
{
    if (output.initialized)
    {
        struct list_head * node_ptr;
        scarletbook_output_format_t * output_format_ptr;

        output.stats_total_sectors = 0;
        output.stats_total_sectors_processed = 0;
        output.stats_current_file_total_sectors = 0;
        output.stats_current_file_sectors_processed = 0;
        output.stats_callback = cb;

        list_for_each(node_ptr, &output.ripping_queue)
        {
            output_format_ptr = list_entry(node_ptr, scarletbook_output_format_t, siblings);
            output.stats_total_sectors += output_format_ptr->length_lsn;
        }
    }
}

static void allocate_round_robin_frame_buffer(void)
{
    audio_frame_t * frame_ptr;
    int i;

    INIT_LIST_HEAD(&output.frames);

    for (i = 0; i < PACKET_FRAME_BUFFER_COUNT; i++)
    {
        frame_ptr = calloc(sizeof(audio_frame_t), 1);
#ifdef __lv2ppu__
        frame_ptr->data = (uint8_t *) memalign(128, 0x4000);
#else
        frame_ptr->data = (uint8_t *) malloc(0x4000);
#endif
        list_add_tail(&frame_ptr->siblings, &output.frames);
    }

    output.frame = list_entry(output.frames.next, audio_frame_t, siblings);

    // HACK, creating a round-robin
    output.frame->siblings.prev = &frame_ptr->siblings;
    frame_ptr->siblings.next = &output.frame->siblings;

    output.current_frame_ptr = output.frame->data;
}

static void free_round_robin_frame_buffer(void)
{
    struct list_head * node_ptr;
    audio_frame_t * frame_ptr;

    // HACK, destroy the round-robin before deletion
    output.frames.next->prev = &output.frames;
    output.frames.prev->next = &output.frames;

    while (!list_empty(&output.frames))
    {
        node_ptr = output.frames.next;
        frame_ptr = list_entry(node_ptr, audio_frame_t, siblings);
        list_del(node_ptr);
        free(frame_ptr->data);
        free(frame_ptr);
    }
}

static inline size_t write_block(scarletbook_output_format_t * ft, const uint8_t *buf, size_t len, int last_frame)
{
    size_t actual = ft->handler.write? (*ft->handler.write)(ft, buf, len, last_frame) : 0;
    ft->write_length += actual;
    return actual;
}

static void process_blocks(scarletbook_output_format_t *ft, uint8_t *buffer, int pos, int blocks)
{
    int i;

    if (ft->handler.flags & OUTPUT_FLAG_DSD || ft->handler.flags & OUTPUT_FLAG_DST)
    {
        uint8_t *main_buffer_ptr = buffer;
        int last_frame = (pos + blocks == ft->start_lsn + ft->length_lsn);

        while(blocks--)
        {
            uint8_t *buffer_ptr = main_buffer_ptr;

            memcpy(&output.scarletbook_audio_sector.header, buffer_ptr, AUDIO_SECTOR_HEADER_SIZE);
            buffer_ptr += AUDIO_SECTOR_HEADER_SIZE;
#if defined(__BIG_ENDIAN__)
            memcpy(&output.scarletbook_audio_sector.packet, buffer_ptr, AUDIO_PACKET_INFO_SIZE * output.scarletbook_audio_sector.header.packet_info_count);
            buffer_ptr += AUDIO_PACKET_INFO_SIZE * output.scarletbook_audio_sector.header.packet_info_count;
#else
            // Little Endian systems cannot properly deal with audio_packet_info_t
            {
                for (i = 0; i < output.scarletbook_audio_sector.header.packet_info_count; i++)
                {
                    output.scarletbook_audio_sector.packet[i].frame_start = (buffer_ptr[0] >> 7) & 1;
                    output.scarletbook_audio_sector.packet[i].data_type = (buffer_ptr[0] >> 3) & 7;
                    output.scarletbook_audio_sector.packet[i].packet_length = (buffer_ptr[0] & 7) << 8 | buffer_ptr[1];
                    buffer_ptr += AUDIO_PACKET_INFO_SIZE;
                }
            }
#endif
            if (output.scarletbook_audio_sector.header.dst_encoded)
            {
                memcpy(&output.scarletbook_audio_sector.frame, buffer_ptr, AUDIO_FRAME_INFO_SIZE * output.scarletbook_audio_sector.header.frame_info_count);
                buffer_ptr += AUDIO_FRAME_INFO_SIZE * output.scarletbook_audio_sector.header.frame_info_count;
            }
            else
            {
                for (i = 0; i < output.scarletbook_audio_sector.header.frame_info_count; i++)
                {
                    memcpy(&output.scarletbook_audio_sector.frame[i], buffer_ptr, AUDIO_FRAME_INFO_SIZE - 1);
                    buffer_ptr += AUDIO_FRAME_INFO_SIZE - 1;
                }
            }
            for (i = 0; i < output.scarletbook_audio_sector.header.packet_info_count; i++)
            {
                audio_packet_info_t *packet = &output.scarletbook_audio_sector.packet[i];
                switch(packet->data_type)
                {
                case DATA_TYPE_AUDIO:
                    {
                        if (output.frame->size > 0 && packet->frame_start)
                        {
                            output.full_frame_count++;
                            output.frame->full = 1;

                            // advance one frame in our cache
                            output.frame = list_entry(output.frame->siblings.next, audio_frame_t, siblings);
                            output.current_frame_ptr = output.frame->data;

                            // is our cache full?
                            if (output.full_frame_count == PACKET_FRAME_BUFFER_COUNT - 1)
                            {
                                // always start with the full first frame
                                audio_frame_t * frame_ptr = list_entry(output.frame->siblings.next, audio_frame_t, siblings);

                                // loop until we reach the current frame
                                while (frame_ptr != output.frame)
                                {
                                    // when buffer is full we write to disk
                                    if (frame_ptr->full)
                                    {
                                        write_block(ft, frame_ptr->data, frame_ptr->size, 0);
                                        
                                        // mark frame as empty
                                        frame_ptr->full = 0;
                                        frame_ptr->size = 0;
                                    }
                                    
                                    // advance one frame
                                    frame_ptr = list_entry(frame_ptr->siblings.next, audio_frame_t, siblings);
                                }
                                // let's start over
                                output.full_frame_count = 0;
                            }
                        }

                        memcpy(output.current_frame_ptr, buffer_ptr, packet->packet_length);
                        output.frame->size += packet->packet_length;
                        output.current_frame_ptr += packet->packet_length;
                        buffer_ptr += packet->packet_length;
                    }
                    break;
                case DATA_TYPE_SUPPLEMENTARY:
                case DATA_TYPE_PADDING:
                    {
                        buffer_ptr += packet->packet_length;
                    }
                    break;
                default:
                    LOG(lm_main, LOG_ERROR, ("unknown type!"));
                    break;
                }
            }
            main_buffer_ptr += SACD_LSN_SIZE;
        }
        // are there any leftovers?
        if ((output.full_frame_count > 0 || output.frame->size > 0) && last_frame)
        {
            audio_frame_t * frame_ptr = output.frame;

            // process all frames
            do
            {
                // always start with the full first frame
                frame_ptr = list_entry(frame_ptr->siblings.next, audio_frame_t, siblings);

                if (frame_ptr->size > 0)
                {
                    write_block(ft, frame_ptr->data, frame_ptr->size, 0);
                    frame_ptr->full = 0;
                    frame_ptr->size = 0;
                }
        
            } while (frame_ptr != output.frame);

            output.full_frame_count = 0;
        }

        // decode DSD frames

    }
    else if (ft->handler.flags & OUTPUT_FLAG_RAW)
    {
        write_block(ft, buffer, blocks, 0);
    }
}

#ifdef __lv2ppu__
static void processing_thread(void *arg)
#else
static void *processing_thread(void *arg)
#endif
{
    scarletbook_handle_t *handle = (scarletbook_handle_t *) arg;
    struct list_head * node_ptr;
    scarletbook_output_format_t * output_format_ptr;

    if (output.initialized)
    {
        while (!list_empty(&output.ripping_queue))
        {
            node_ptr = output.ripping_queue.next;
            output_format_ptr = list_entry(node_ptr, scarletbook_output_format_t, siblings);
            list_del(node_ptr);

            output_format_ptr->sb_handle = handle;

            output.stats_current_file_total_sectors = output_format_ptr->length_lsn;
            output.stats_current_file_sectors_processed = 0;

            if (output.stats_callback)
            {
                output.stats_callback(output.stats_total_sectors, output.stats_total_sectors_processed, 
                    output.stats_current_file_total_sectors, output.stats_current_file_sectors_processed,
                    output_format_ptr->filename);
            }

            create_output_file(output_format_ptr);
            {
                scarletbook_output_format_t *ft = output_format_ptr;
                uint32_t block_size, end_lsn;
                uint32_t encrypted_start_1 = 0;
                uint32_t encrypted_start_2 = 0;
                uint32_t encrypted_end_1 = 0;
                uint32_t encrypted_end_2 = 0;
                int encrypted;

                if (handle->area[0].area_toc != 0)
                {
                    encrypted_start_1 = handle->area[0].area_toc->track_start;
                    encrypted_end_1 = handle->area[0].area_toc->track_end;
                }
                if (handle->area[1].area_toc != 0)
                {
                    encrypted_start_2 = handle->area[1].area_toc->track_start;
                    encrypted_end_2 = handle->area[1].area_toc->track_end;
                }

                ft->current_lsn = ft->start_lsn;
                end_lsn = ft->start_lsn + ft->length_lsn;

                atomic_set(&output.stop_processing, 0);

                while (atomic_read(&output.stop_processing) == 0)
                {
                    if (ft->current_lsn < end_lsn)
                    {
                        // check what block ranges are encrypted..
                        if (ft->current_lsn < encrypted_start_1)
                        {
                            block_size = min(encrypted_start_1 - ft->current_lsn, MAX_PROCESSING_BLOCK_SIZE);
                            encrypted = 0;
                        }
                        else if (ft->current_lsn >= encrypted_start_1 && ft->current_lsn <= encrypted_end_1)
                        {
                            block_size = min(encrypted_end_1 + 1 - ft->current_lsn, MAX_PROCESSING_BLOCK_SIZE);
                            encrypted = 1;
                        }
                        else if (ft->current_lsn > encrypted_end_1 && ft->current_lsn < encrypted_start_2)
                        {
                            block_size = min(encrypted_start_2 - ft->current_lsn, MAX_PROCESSING_BLOCK_SIZE);
                            encrypted = 0;
                        }
                        else if (ft->current_lsn >= encrypted_start_2 && ft->current_lsn <= encrypted_end_2)
                        {
                            block_size = min(encrypted_end_2 + 1 - ft->current_lsn, MAX_PROCESSING_BLOCK_SIZE);
                            encrypted = 1;
                        }
                        else
                        {
                            block_size = MAX_PROCESSING_BLOCK_SIZE;
                            encrypted = 0;
                        }
                        block_size = min(end_lsn - ft->current_lsn, block_size);

                        // read some blocks
                        sacd_read_block_raw(ft->sb_handle->sacd, ft->current_lsn, block_size, output.read_buffer);

                        // encrypted blocks need to be decrypted first
                        if (encrypted)
                        {
                            sacd_decrypt(ft->sb_handle->sacd, output.read_buffer, block_size);
                        }

                        // process blocks and write to disk
                        process_blocks(ft, output.read_buffer, ft->current_lsn, block_size);

                        output.stats_total_sectors_processed += block_size;
                        output.stats_current_file_sectors_processed += block_size;

                        if (output.stats_callback)
                        {
                            output.stats_callback(output.stats_total_sectors, output.stats_total_sectors_processed, 
                                output.stats_current_file_total_sectors, output.stats_current_file_sectors_processed,
                                0);
                        }

                        ft->current_lsn += block_size;
                    }
                    else
                    {
                        break;
                    }
                }

            }

            close_output_file(output_format_ptr);

            if (atomic_read(&output.stop_processing) == 1)
            {
                // remove the file being worked on
                remove(output_format_ptr->filename);
                destroy_output_format(output_format_ptr);
                destroy_ripping_queue();
#ifdef __lv2ppu__
                sysThreadExit(-1);
#else
                pthread_exit(0);
#endif
            }

            destroy_output_format(output_format_ptr);
        } 
        destroy_ripping_queue();
    }

#ifdef __lv2ppu__
    sysThreadExit(-1);
#else
    pthread_exit(0);

    return 0;
#endif
}

void initialize_ripping(void)
{
    memset(&output, 0, sizeof(scarletbook_output_t));

    INIT_LIST_HEAD(&output.ripping_queue);
    output.read_buffer = (uint8_t *) malloc(MAX_PROCESSING_BLOCK_SIZE * SACD_LSN_SIZE);
    output.initialized = 1;

    allocate_round_robin_frame_buffer();
}

int is_ripping(void)
{
    return atomic_read(&output.stop_processing);
}

int start_ripping(scarletbook_handle_t *handle)
{
    int ret = 0;

#ifdef __lv2ppu__
    ret = sysThreadCreate(&output.processing_thread_id,
                          processing_thread,
                          (void *) handle,
                          1050,
                          8192,
                          THREAD_JOINABLE,
                          "processing_thread");
#else
    ret = pthread_create(&output.processing_thread_id, NULL, processing_thread, (void *) handle);
#endif
    if (ret)
    {
        LOG(lm_main, LOG_ERROR, ("return code from processing thread creation is %d\n", ret));
    }

    return ret;
}

void interrupt_ripping(void)
{
    atomic_set(&output.stop_processing, 1);
}

int stop_ripping(scarletbook_handle_t *handle)
{
    int     ret = 0;

#ifdef __lv2ppu__
    {
        uint64_t thr_exit_code;
        ret = sysThreadJoin(output.processing_thread_id, &thr_exit_code);
    }
#else
    {
        void *thr_exit_code;
        ret = pthread_join(output.processing_thread_id, &thr_exit_code);
    }
#endif    
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("processing thread didn't close properly..."));
    }

    output.initialized = 0;
    free_round_robin_frame_buffer();
    free(output.read_buffer);

    return ret;
}
