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
#ifdef __lv2ppu__
#include <sys/file.h>
#elif defined(WIN32)
#include <io.h>
#endif

#include <utils.h>
#include <logging.h>

#include "scarletbook_output.h"
#include "sacd_reader.h"

static struct list_head ripping_queue;
static int initialized_ripping_queue = 0;
static int stop_processing = 0;

// TODO, move to context
audio_frame_t audio_sector;
int current_audio_frame_size = 0;
uint8_t current_audio_frame[SACD_LSN_SIZE * 40];
uint8_t *current_audio_frame_ptr = current_audio_frame;

extern scarletbook_format_handler_t const * dsdiff_format_fn(void);

typedef const scarletbook_format_handler_t *(*sacd_output_format_fn_t)(void); 
static sacd_output_format_fn_t s_sacd_output_format_fns[] = 
{
    dsdiff_format_fn,
    NULL
}; 

scarletbook_format_handler_t const * sacd_find_output_format(char const * name)
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

static inline void initialize_ripping_queue()
{
    if (!initialized_ripping_queue)
    {
        INIT_LIST_HEAD(&ripping_queue);
        initialized_ripping_queue = 1;
    }
}

static void destroy_ripping_queue()
{
    if (initialized_ripping_queue)
    {
        struct list_head * node_ptr;
        scarletbook_output_format_t * output_format_ptr;

        while (!list_empty(&ripping_queue))
        {
            node_ptr = ripping_queue.next;
            output_format_ptr = list_entry(node_ptr, scarletbook_output_format_t, siblings);
            list_del(node_ptr);
            free(output_format_ptr);
        }
        initialized_ripping_queue = 0;
    }
}

int queue_track_to_rip(int area, int track, char *file_path, char *fmt, 
                                uint32_t start_lsn, uint32_t length_lsn, int dst_encoded)
{
    scarletbook_format_handler_t const * handler;
    scarletbook_output_format_t * output_format_ptr;

    initialize_ripping_queue();

    if ((handler = sacd_find_output_format(fmt)))
    {
        output_format_ptr = calloc(sizeof(scarletbook_output_format_t), 1);
        output_format_ptr->area = area;
        output_format_ptr->track = track;
        output_format_ptr->handler = *handler;
        output_format_ptr->filename = strdup(file_path);
        output_format_ptr->start_lsn = start_lsn;
        output_format_ptr->length_lsn = length_lsn;
        output_format_ptr->dst_encoded = dst_encoded;
        list_add_tail(&output_format_ptr->siblings, &ripping_queue);

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

size_t write_frame(scarletbook_output_format_t * ft, const uint8_t *buf, size_t len)
{
    size_t actual = ft->handler.write? (*ft->handler.write)(ft, buf, len) : 0;
    ft->write_length += actual;
    return actual;
}

int close_output_file(scarletbook_output_format_t * ft)
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

void destroy_output_format(scarletbook_output_format_t * ft)
{
    free(ft->filename);
    free(ft->priv);
    free(ft);
}

static void scarletbook_process_frames_callback(uint8_t *buffer, int pos, int blocks, void *user_data)
{
    int ret, i;
    scarletbook_output_format_t *ft = (scarletbook_output_format_t *) user_data;

    if (ft->encrypted)
    {
        ret = sacd_decrypt(ft->sb_handle->sacd, buffer, blocks);
        if (ret != 0)
            ft->error_nr = -1;
    }

    if (ft->handler.preprocess_audio_frames)
    {
        uint8_t *main_buffer_ptr = buffer;
        int last_frame = (pos + blocks == ft->start_lsn + ft->length_lsn);

        while(blocks--)
        {
            uint8_t *buffer_ptr = main_buffer_ptr;

            memcpy(&audio_sector.header, buffer_ptr, AUDIO_SECTOR_HEADER_SIZE);
            buffer_ptr += AUDIO_SECTOR_HEADER_SIZE;
#if defined(__BIG_ENDIAN__)
            memcpy(&audio_sector.packet, buffer_ptr, AUDIO_PACKET_INFO_SIZE * audio_sector.header.packet_info_count);
            buffer_ptr += AUDIO_PACKET_INFO_SIZE * audio_sector.header.packet_info_count;
#else
            // Little Endian systems cannot properly deal with audio_packet_info_t
            {
                for (i = 0; i < audio_sector.header.packet_info_count; i++)
                {
                    audio_sector.packet[i].frame_start = (buffer_ptr[0] >> 7) & 1;
                    audio_sector.packet[i].data_type = (buffer_ptr[0] >> 3) & 7;
                    audio_sector.packet[i].packet_length = (buffer_ptr[0] & 7) << 8 | buffer_ptr[1];
                    buffer_ptr += AUDIO_PACKET_INFO_SIZE;
                }
            }
#endif
            if (audio_sector.header.dst_encoded)
            {
                memcpy(&audio_sector.frame, buffer_ptr, AUDIO_FRAME_INFO_SIZE * audio_sector.header.frame_info_count);
                buffer_ptr += AUDIO_FRAME_INFO_SIZE * audio_sector.header.frame_info_count;
            }
            else
            {
                for (i = 0; i < audio_sector.header.frame_info_count; i++)
                {
                    memcpy(&audio_sector.frame[i], buffer_ptr, AUDIO_FRAME_INFO_SIZE - 1);
                    buffer_ptr += AUDIO_FRAME_INFO_SIZE - 1;
                }
            }
            for (i = 0; i < audio_sector.header.packet_info_count; i++)
            {
                audio_packet_info_t *packet = &audio_sector.packet[i];
                switch(packet->data_type)
                {
                case DATA_TYPE_AUDIO:
                    {
                        if (current_audio_frame_size > 0 && packet->frame_start)
                        {
                            // TODO: add DST decoding here..

                            write_frame(ft, current_audio_frame, current_audio_frame_size);
                            
                            current_audio_frame_ptr = current_audio_frame;
                            current_audio_frame_size = 0;
                        }

                        memcpy(current_audio_frame_ptr, buffer_ptr, packet->packet_length);
                        current_audio_frame_size += packet->packet_length;
                        current_audio_frame_ptr += packet->packet_length;
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
        if (current_audio_frame_size > 0 && last_frame)
        {
            write_frame(ft, current_audio_frame, current_audio_frame_size);

            current_audio_frame_ptr = current_audio_frame;
            current_audio_frame_size = 0;
        }
    }
    else
    {
        write_frame(ft, buffer, blocks);
    }
}

#ifdef __lv2ppu__
static int process_frames(scarletbook_output_format_t * ft)
{
    uint32_t block_size, end_lsn;
    scarletbook_handle_t *handle = ft->sb_handle;
    uint32_t encrypted_start_1 = 0;
    uint32_t encrypted_start_2 = 0;
    uint32_t encrypted_end_1;
    uint32_t encrypted_end_2;

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

    while (atomic_read(&stop_processing) == 0)
    {
        if (ft->current_lsn < end_lsn)
        {
            // check what parts are encrypted..
            if (encrypted_start_1
                && (is_between_inclusive(ft->current_lsn + MAX_PROCESSING_BLOCK_SIZE, encrypted_start_1, encrypted_end_1)
                || is_between_exclusive(ft->current_lsn, encrypted_start_1, encrypted_end_1))
                )
            {
                if (ft->current_lsn < encrypted_start_1)
                {
                    block_size = encrypted_start_1 - ft->current_lsn;
                    ft->encrypted = 0;
                }
                else
                {
                    block_size = min(encrypted_end_1 - ft->current_lsn + 1, MAX_PROCESSING_BLOCK_SIZE);
                    ft->encrypted = 1;
                }
            }
            else if (encrypted_start_2
                && (is_between_inclusive(ft->current_lsn + MAX_PROCESSING_BLOCK_SIZE, encrypted_start_2, encrypted_end_2)
                || is_between_exclusive(ft->current_lsn, encrypted_start_2, encrypted_end_2))
                )
            {
                if (ft->current_lsn < encrypted_start_2)
                {
                    block_size = encrypted_start_2 - ft->current_lsn;
                    ft->encrypted = 0;
                }
                else
                {
                    block_size = min(encrypted_end_2 - ft->current_lsn + 1, MAX_PROCESSING_BLOCK_SIZE);
                    ft->encrypted = 1;
                }
            }
            else 
            {
                block_size = min(end_lsn - ft->current_lsn, MAX_PROCESSING_BLOCK_SIZE);
                ft->encrypted = 0;
            }

            sacd_read_async_block_raw(ft->sb_handle->sacd, ft->current_lsn, block_size, scarletbook_process_frames_callback, ft);

            ft->current_lsn += block_size;
        }
        else if (atomic_read(&outstanding_read_requests) == 0)
        {
            // we are done!
            break;
        }
        ret = sysMutexLock(accessor->processing_mutex, 0);
        if (ret != 0)
        {
            LOG(lm_main, LOG_NOTICE, ("error sysMutexLock"));
            goto close_thread;
        }

        LOG(lm_main, LOG_NOTICE, ("waiting for async read result (6 sec)"));
        ret = sysCondWait(accessor->processing_cond, 6000000);
        if (ret != 0)
        {
            LOG(lm_main, LOG_NOTICE, ("error sysCondWait"));
            sysMutexUnlock(accessor->processing_mutex);
            goto close_thread;
        }

        ret = sysMutexUnlock(accessor->processing_mutex);
        if (ret != 0)
        {
            LOG(lm_main, LOG_NOTICE, ("error sysMutexUnlock"));
            goto close_thread;
        }
    }

    return 0;
}

#else 

static int process_frames(scarletbook_output_format_t * ft)
{
    uint32_t block_size, end_lsn;
    scarletbook_handle_t *handle = ft->sb_handle;

    ft->current_lsn = ft->start_lsn;
    end_lsn = ft->start_lsn + ft->length_lsn;

    stop_processing = 0;
    while (stop_processing == 0)
    {
        if (ft->current_lsn < end_lsn)
        {
            block_size = min(end_lsn - ft->current_lsn, MAX_PROCESSING_BLOCK_SIZE);
            ft->encrypted = 0;

            sacd_read_async_block_raw(ft->sb_handle->sacd, ft->current_lsn, block_size, scarletbook_process_frames_callback, ft);

            ft->current_lsn += block_size;
        }
        else 
        {
            return 0;
        }
    }

    return -1;
}
#endif

uint32_t stats_total_sectors;

int start_ripping(scarletbook_handle_t *handle)
{
    struct list_head * node_ptr;
    scarletbook_output_format_t * output_format_ptr;

    // calculate the total amount of sectors to process
    stats_total_sectors = 0;
    list_for_each(node_ptr, &ripping_queue)
    {
        output_format_ptr = list_entry(node_ptr, scarletbook_output_format_t, siblings);
        stats_total_sectors += output_format_ptr->length_lsn;
    }

    while (!list_empty(&ripping_queue))
    {
        node_ptr = ripping_queue.next;
        output_format_ptr = list_entry(node_ptr, scarletbook_output_format_t, siblings);
        list_del(node_ptr);

        output_format_ptr->sb_handle = handle;

        create_output_file(output_format_ptr);
        process_frames(output_format_ptr);
        close_output_file(output_format_ptr);

        if (stop_processing)
        {
            // remove the file being worked on
            remove(output_format_ptr->filename);
            destroy_output_format(output_format_ptr);
            destroy_ripping_queue();
            return -1;
        }

        destroy_output_format(output_format_ptr);
    } 
    destroy_ripping_queue();

    return 0;
}

void stop_ripping(scarletbook_handle_t *handle)
{
    stop_processing = 1;
}
