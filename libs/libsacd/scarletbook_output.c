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
#include <errno.h>
#include <assert.h>
#ifdef __lv2ppu__
#include <sys/file.h>
#elif defined(WIN32)
#include <io.h>
#endif

#include <charset.h>
#include <utils.h>
#include <logging.h>

#include "scarletbook_output.h"
#include "scarletbook_read.h"
#include "sacd_reader.h"

#define WRITE_CACHE_SIZE 1 * 1024 * 1024

// TODO: - allocate dynamically
//       - rename functions to output_*
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

int queue_track_to_rip(scarletbook_handle_t *sb_handle, int area, int track, char *file_path, char *fmt, int dsd_encoded_export, int gapless)
{
    scarletbook_format_handler_t const * handler;
    scarletbook_output_format_t * output_format_ptr;

    if ((handler = find_output_format(fmt)))
    {
        output_format_ptr = calloc(sizeof(scarletbook_output_format_t), 1);
        output_format_ptr->sb_handle = sb_handle;
        output_format_ptr->area = area;
        output_format_ptr->track = track;
        output_format_ptr->handler = *handler;
        output_format_ptr->filename = strdup(file_path);
        output_format_ptr->channel_count = sb_handle->area[area].area_toc->channel_count;
        output_format_ptr->dst_encoded_import = sb_handle->area[area].area_toc->frame_format == FRAME_FORMAT_DST;
        output_format_ptr->dsd_encoded_export = dsd_encoded_export;

        if (!gapless) 
        {
            output_format_ptr->start_lsn = sb_handle->area[area].area_tracklist_offset->track_start_lsn[track];
            output_format_ptr->length_lsn = sb_handle->area[area].area_tracklist_offset->track_length_lsn[track];
        }
        else 
        {
            if (track > 0) 
            {
                output_format_ptr->start_lsn = sb_handle->area[area].area_tracklist_offset->track_start_lsn[track];
            }
            else 
            {
                output_format_ptr->start_lsn = sb_handle->area[area].area_toc->track_start;
            }
            if (track < sb_handle->area[area].area_toc->track_count - 1) 
            {
                output_format_ptr->length_lsn = sb_handle->area[area].area_tracklist_offset->track_start_lsn[track + 1] - output_format_ptr->start_lsn + 1;
            }
            else 
            {
                output_format_ptr->length_lsn = sb_handle->area[area].area_toc->track_end - output_format_ptr->start_lsn;
            }
        }


        LOG(lm_main, LOG_NOTICE, ("Queuing: %s, area: %d, track %d, start_lsn: %d, length_lsn: %d, dst_encoded_import: %d, dsd_encoded_export: %d", file_path, area, track, output_format_ptr->start_lsn, output_format_ptr->length_lsn, output_format_ptr->dst_encoded_import, output_format_ptr->dsd_encoded_export));

        list_add_tail(&output_format_ptr->siblings, &output.ripping_queue);

        return 0;
    }
    return -1;
}

int queue_raw_sectors_to_rip(scarletbook_handle_t *sb_handle, int start_lsn, int length_lsn, char *file_path, char *fmt)
{
    scarletbook_format_handler_t const * handler;
    scarletbook_output_format_t * output_format_ptr;

    if ((handler = find_output_format(fmt)))
    {
        output_format_ptr = calloc(sizeof(scarletbook_output_format_t), 1);
        output_format_ptr->sb_handle = sb_handle;
        output_format_ptr->handler = *handler;
        output_format_ptr->filename = strdup(file_path);
        output_format_ptr->start_lsn = start_lsn;
        output_format_ptr->length_lsn = length_lsn;

        LOG(lm_main, LOG_NOTICE, ("Queuing raw: %s, start_lsn: %d, length_lsn: %d", file_path, start_lsn, length_lsn));

        list_add_tail(&output_format_ptr->siblings, &output.ripping_queue);

        return 0;
    }
    return -1;
}

static int create_output_file(scarletbook_output_format_t *ft)
{
    int result;

#ifdef _WIN32
    wchar_t *wide_filename = (wchar_t *) charset_convert(ft->filename, strlen(ft->filename), "UTF-8", "UCS-2-INTERNAL");
    ft->fd = _wfopen(wide_filename, L"wb");
    free(wide_filename);
#else
    ft->fd = fopen(ft->filename, "wb");
#endif
    if (ft->fd == 0)
    {
        LOG(lm_main, LOG_ERROR, ("error creating %s, errno: %d, %s", ft->filename, errno, strerror(errno)));
        goto error;
    }

#ifdef __lv2ppu__
    sysFsChmod(ft->filename, S_IFMT | 0777); 
#endif

    ft->write_cache = malloc(WRITE_CACHE_SIZE);
    setvbuf(ft->fd, ft->write_cache, _IOFBF , WRITE_CACHE_SIZE);

    ft->priv = calloc(1, ft->handler.priv_size);

    result = ft->handler.startwrite ? (*ft->handler.startwrite)(ft) : 0;

    return result;

error:
    if (ft->fd)
    {
        fclose(ft->fd);
    }
    free(ft->write_cache);
    free(ft->priv);
    free(ft);
    return -1;
}

static inline int close_output_file(scarletbook_output_format_t * ft)
{
    int result;

    result = ft->handler.stopwrite ? (*ft->handler.stopwrite)(ft) : 0;

    if (ft->fd)
    {
        fclose(ft->fd);
    }
    free(ft->write_cache);
    free(ft->filename);
    free(ft->priv);
    free(ft);

    return result;
}

void init_stats(stats_track_callback_t cb_track, stats_progress_callback_t cb_progress)
{
    if (output.initialized)
    {
        struct list_head * node_ptr;
        scarletbook_output_format_t * output_format_ptr;

        output.stats_total_sectors = 0;
        output.stats_total_sectors_processed = 0;
        output.stats_current_file_total_sectors = 0;
        output.stats_current_file_sectors_processed = 0;
        output.stats_track_callback = cb_track;
        output.stats_progress_callback = cb_progress;
        output.stats_current_track = 0;
        output.stats_total_tracks = 0;
        
        list_for_each(node_ptr, &output.ripping_queue)
        {
            output_format_ptr = list_entry(node_ptr, scarletbook_output_format_t, siblings);
            output.stats_total_sectors += output_format_ptr->length_lsn;
            output.stats_total_tracks++;
        }
    }
}

static inline size_t write_block(scarletbook_output_format_t * ft, const uint8_t *buf, size_t len)
{
    size_t actual = ft->handler.write? (*ft->handler.write)(ft, buf, len) : 0;
    ft->write_length += actual;
    return actual;
}

static void frame_decoded_callback(uint8_t* frame_data, size_t frame_size, void *userdata)
{
    scarletbook_output_format_t *ft = (scarletbook_output_format_t *) userdata;
    write_block(ft, frame_data, frame_size);
}

void frame_read_callback(scarletbook_handle_t *handle, uint8_t* frame_data, size_t frame_size, void *userdata)
{
    scarletbook_output_format_t *ft = (scarletbook_output_format_t *) userdata;

    if (ft->dsd_encoded_export && ft->dst_encoded_import)
    {
        dst_decoder_decode(output.dst_decoder, frame_data, frame_size);
    }
    else
    {
        write_block(ft, frame_data, frame_size);
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
    scarletbook_output_format_t * ft;
    int non_encrypted_disc = 0;
    int checked_for_non_encrypted_disc = 0;
    ssize_t ret;

    sysAtomicSet(&output.processing, 1);
    if (output.initialized)
    {
        while (!list_empty(&output.ripping_queue))
        {
            node_ptr = output.ripping_queue.next;

            ft = list_entry(node_ptr, scarletbook_output_format_t, siblings);
            list_del(node_ptr);

            output.dst_decoder = dst_decoder_create(ft->channel_count, frame_decoded_callback, ft);

            output.stats_current_file_total_sectors = ft->length_lsn;
            output.stats_current_file_sectors_processed = 0;
            output.stats_current_track++;

            if (output.stats_track_callback)
            {
                output.stats_track_callback(ft->filename, output.stats_current_track, output.stats_total_tracks);
            }

            scarletbook_frame_init(handle);

            if (create_output_file(ft) == 0)
            {
                uint32_t block_size, end_lsn;
                uint32_t encrypted_start_1 = 0;
                uint32_t encrypted_start_2 = 0;
                uint32_t encrypted_end_1 = 0;
                uint32_t encrypted_end_2 = 0;
                int encrypted;

                // set the encryption range
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

                // what blocks do we need to process?
                ft->current_lsn = ft->start_lsn;
                end_lsn = ft->start_lsn + ft->length_lsn;

                sysAtomicSet(&output.stop_processing, 0);

                while (sysAtomicRead(&output.stop_processing) == 0)
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
                        ret = sacd_read_block_raw(ft->sb_handle->sacd, ft->current_lsn, block_size, output.read_buffer);

                        ft->current_lsn += block_size;
                        output.stats_total_sectors_processed += block_size;
                        output.stats_current_file_sectors_processed += block_size;

                        // the ATAPI call which returns the flag if the disc is encrypted or not is unknown at this point. 
                        // user reports tell me that the only non-encrypted discs out there are DSD 3 14/16 discs. 
                        // this is a quick hack/fix for these discs.
                        if (encrypted && checked_for_non_encrypted_disc == 0)
                        {
                            switch (handle->area[ft->area].area_toc->frame_format)
                            {
                            case FRAME_FORMAT_DSD_3_IN_14:
                            case FRAME_FORMAT_DSD_3_IN_16:
                                non_encrypted_disc = *(uint64_t *)(output.read_buffer + 16) == 0;
                                break;
                            }

                            checked_for_non_encrypted_disc = 1;
                        }

                        // encrypted blocks need to be decrypted first
                        if (encrypted && non_encrypted_disc == 0)
                        {
                            sacd_decrypt(ft->sb_handle->sacd, output.read_buffer, block_size);
                        }

                        // process DSD & DST frames
                        if (ft->handler.flags & OUTPUT_FLAG_DSD || ft->handler.flags & OUTPUT_FLAG_DST)
                        {
                            scarletbook_process_frames(ft->sb_handle, output.read_buffer, block_size, ft->current_lsn == end_lsn, frame_read_callback, ft);
                        }
                        // ISO output is written without frame processing                        
                        else if (ft->handler.flags & OUTPUT_FLAG_RAW)
                        {
                            write_block(ft, output.read_buffer, block_size);
                        }

                        // update statistics
                        if (output.stats_progress_callback)
                        {
                            output.stats_progress_callback(output.stats_total_sectors, output.stats_total_sectors_processed, 
                                output.stats_current_file_total_sectors, output.stats_current_file_sectors_processed);
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }

            if (sysAtomicRead(&output.stop_processing) == 1)
            {
                // make a copy of the filename
                char *file_to_remove = strdup(ft->filename);

                sysAtomicSet(&output.processing, 0);

                dst_decoder_destroy(output.dst_decoder);

                close_output_file(ft);

                // remove the file being worked on
#ifdef __lv2ppu__
                if (sysFsUnlink(file_to_remove) != 0)
#else
                if (remove(file_to_remove) != 0)
#endif
                {
                    LOG(lm_main, LOG_ERROR, ("user cancelled, error removing: %s, [%s]", file_to_remove, strerror(errno)));
                }
                free(file_to_remove);

                destroy_ripping_queue();
#ifdef __lv2ppu__
                sysThreadExit(-1);
#else
                pthread_exit(0);
#endif
            }

            dst_decoder_destroy(output.dst_decoder);

            close_output_file(ft);
        } 
        destroy_ripping_queue();
    }

    sysAtomicSet(&output.processing, 0);

#ifdef __lv2ppu__
    sysThreadExit(-1);
#else
    pthread_exit(0);

    return 0;
#endif
}

int initialize_ripping(void)
{
    int ret = 0;

    memset(&output, 0, sizeof(scarletbook_output_t));

    INIT_LIST_HEAD(&output.ripping_queue);
    output.read_buffer = (uint8_t *) malloc(MAX_PROCESSING_BLOCK_SIZE * SACD_LSN_SIZE);
    output.initialized = 1;

    return ret;
}

int is_ripping(void)
{
    return sysAtomicRead(&output.processing);
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
    sysAtomicSet(&output.stop_processing, 1);
}

int stop_ripping(scarletbook_handle_t *handle)
{
    int ret = 0;

    if (output.initialized)
    {

#ifdef __lv2ppu__
        uint64_t thr_exit_code;
        interrupt_ripping();
        ret = sysThreadJoin(output.processing_thread_id, &thr_exit_code);
#else
        void *thr_exit_code;
        interrupt_ripping();
        ret = pthread_join(output.processing_thread_id, &thr_exit_code);
#endif    
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("processing thread didn't close properly... %x", thr_exit_code));
        }

        // If decoding is aborted (eg. ctrl+C), then free() buffers after the decoder has been destroyed,
        // to ensure that buffers aren't still in use when they're free()d.
        free(output.read_buffer);

        output.initialized = 0;
    }

    return ret;
}
