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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <inttypes.h>
#ifndef __APPLE__
#include <malloc.h>
#endif
#include <errno.h>
#include <assert.h>
#ifdef __lv2ppu__
#include <sys/file.h>
#include <sys/thread.h>
#elif defined(WIN32) || defined(_WIN32)
#include <io.h>
#endif
#ifndef __lv2ppu__
#include <pthread.h>
#endif
#include <sys/atomic.h>
#include <signal.h>

#include <charset.h>
#include <utils.h>
#include <logging.h>

#include "scarletbook_output.h"
#include "scarletbook_read.h"
#include "sacd_reader.h"

#define WRITE_CACHE_SIZE 1 * 1024 * 1024

extern scarletbook_format_handler_t const * dsdiff_format_fn(void);
extern scarletbook_format_handler_t const * dsdiff_edit_master_format_fn(void);
extern scarletbook_format_handler_t const * dsf_format_fn(void);
extern scarletbook_format_handler_t const * iso_format_fn(void);

typedef const scarletbook_format_handler_t *(*sacd_output_format_fn_t)(void); 
static sacd_output_format_fn_t s_sacd_output_format_fns[] = 
{
    dsdiff_format_fn,
    dsdiff_edit_master_format_fn,
    dsf_format_fn,
    iso_format_fn,
    NULL
}; 

struct scarletbook_output_s
{
    struct list_head    ripping_queue;

    uint8_t            *read_buffer;

#ifdef __lv2ppu__
    sys_ppu_thread_t    processing_thread_id;
#else
    pthread_t           processing_thread_id;
#endif
    atomic_t            stop_processing;            // indicates if the thread needs to stop or has stopped
    atomic_t            processing;

    // stats
    int                 stats_total_tracks;
    int                 stats_current_track;
    uint32_t            stats_total_sectors;
    uint32_t            stats_total_sectors_processed;
    uint32_t            stats_current_file_total_sectors;
    uint32_t            stats_current_file_sectors_processed;
    stats_progress_callback_t stats_progress_callback;
    stats_track_callback_t stats_track_callback;

    fwprintf_callback_t fwprintf_callback;

    scarletbook_handle_t *sb_handle;
};

static scarletbook_format_handler_t const * find_output_format(char const * name)
{
    int i=0;
    while (s_sacd_output_format_fns[i] != NULL)
    {
        scarletbook_format_handler_t const * handler = s_sacd_output_format_fns[i]();
        if (!strcasecmp(handler->name, name)) 
        {
            return handler;
        }
        i++;
    }
    return NULL;
} 

static void destroy_ripping_queue(scarletbook_output_t *output)
{
    struct list_head * node_ptr;
    scarletbook_output_format_t * output_format_ptr;

    while (!list_empty(&output->ripping_queue))
    {
        node_ptr = output->ripping_queue.next;
        output_format_ptr = list_entry(node_ptr, scarletbook_output_format_t, siblings);
        list_del(node_ptr);
        free(output_format_ptr);
    }
}

int scarletbook_output_enqueue_track(scarletbook_output_t *output, int area, int track, char *file_path, char *fmt, int dsd_encoded_export, int dsf_nopad)
{
    scarletbook_format_handler_t const * handler;
    scarletbook_output_format_t * output_format_ptr;
    scarletbook_handle_t *sb_handle = output->sb_handle;

    if ((handler = find_output_format(fmt)))
    {
        output_format_ptr = calloc(sizeof(scarletbook_output_format_t), 1);
        output_format_ptr->sb_handle = sb_handle;
        output_format_ptr->cb_fwprintf = output->fwprintf_callback;
        output_format_ptr->area = area;
        output_format_ptr->track = track;
        output_format_ptr->handler = *handler;
        output_format_ptr->filename = strdup(file_path);
        output_format_ptr->channel_count = sb_handle->area[area].area_toc->channel_count;
        output_format_ptr->dst_encoded_import = sb_handle->area[area].area_toc->frame_format == FRAME_FORMAT_DST;
        output_format_ptr->dsd_encoded_export = dsd_encoded_export;
        

        if (handler->flags & OUTPUT_FLAG_EDIT_MASTER)
        {
            output_format_ptr->start_lsn = sb_handle->area[area].area_toc->track_start;
            output_format_ptr->length_lsn = sb_handle->area[area].area_toc->track_end - sb_handle->area[area].area_toc->track_start + 1;
        }
        else
        {
            // Read traks without pauses
            //output_format_ptr->start_lsn = sb_handle->area[area].area_tracklist_offset->track_start_lsn[track];
            //output_format_ptr->length_lsn = sb_handle->area[area].area_tracklist_offset->track_length_lsn[track];

            // Read all LSNs (including pauses)
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
                output_format_ptr->length_lsn = sb_handle->area[area].area_toc->track_end - output_format_ptr->start_lsn + 1;
            }
            // DEBUG
            //output_format_ptr->cb_fwprintf(stderr, L"\n Debug: Queuing: track %d, area_tracklist_offset->track_start_lsn: %d, area_tracklist_offset->track_length_lsn: %d\n", track,
            //                               sb_handle->area[area].area_tracklist_offset->track_start_lsn[track], output_format_ptr->length_lsn = sb_handle->area[area].area_tracklist_offset->track_length_lsn[track]);
            //output_format_ptr->cb_fwprintf(stderr, L"\n Debug: Queuing: track %d, start_lsn: %d, length_lsn: %d\n", track, output_format_ptr->start_lsn, output_format_ptr->length_lsn);

            // Do some integrity checks
            //The Track_Start_Address of a Track must be in the Track Area of
            //    the corresponding Audio Area
            if (!(output_format_ptr->start_lsn >= sb_handle->area[area].area_toc->track_start) ||
                !(output_format_ptr->start_lsn <= sb_handle->area[area].area_toc->track_end)    )
            {
                LOG(lm_main, LOG_NOTICE, ("Queuing error: track_start_lsn is not is area! area: %d, track %d, start_lsn: %d, length_lsn: %d", area, track, output_format_ptr->start_lsn, output_format_ptr->length_lsn));
                output_format_ptr->cb_fwprintf(stderr, L"\n Queuing error: track_start_lsn is not is area! area: %d, track %d, start_lsn: %d, length_lsn: %d\n", area, track, output_format_ptr->start_lsn, output_format_ptr->length_lsn);
            }

            //  For 1 ≤ track < N_Tracks the following equation must be true:
            //        Track_Start_Address[track+1] ≥ Track_Start_Address[track] + Track_Length[track] - 1
            if (track < sb_handle->area[area].area_toc->track_count - 1)
            {
               if( !(sb_handle->area[area].area_tracklist_offset->track_start_lsn[track + 1] >= sb_handle->area[area].area_tracklist_offset->track_start_lsn[track] + sb_handle->area[area].area_tracklist_offset->track_length_lsn[track] - 1))
                 {
                     LOG(lm_main, LOG_NOTICE, ("Queuing error: equation not valid! area: %d, track %d, start_lsn: %d, length_lsn: %d", area, track, output_format_ptr->start_lsn, output_format_ptr->length_lsn));
                     output_format_ptr->cb_fwprintf(stderr, L"\n Queuing error: equation not valid(beetween track_start_lsn and track_length_lsn)! area: %d, track %d, start_lsn: %d, length_lsn: %d\n", area, track, output_format_ptr->start_lsn, output_format_ptr->length_lsn);
                 }
            }
            else
            {
                if (!(sb_handle->area[area].area_toc->track_end >= sb_handle->area[area].area_tracklist_offset->track_start_lsn[track] + sb_handle->area[area].area_tracklist_offset->track_length_lsn[track] - 1))
                {
                    LOG(lm_main, LOG_NOTICE, ("Queuing error: equation not valid! area: %d, track %d, start_lsn: %d, length_lsn: %d", area, track, output_format_ptr->start_lsn, output_format_ptr->length_lsn));
                    output_format_ptr->cb_fwprintf(stderr, L"\n Queuing error: equation not valid(beetween track_start_lsn and track_length_lsn)! area: %d, track %d, start_lsn: %d, length_lsn: %d\n", area, track, output_format_ptr->start_lsn, output_format_ptr->length_lsn);
                }
            }
        }

        LOG(lm_main, LOG_NOTICE, ("Queuing: %s, area: %d, track %d, start_lsn: %d, length_lsn: %d, dst_encoded_import: %d, dsd_encoded_export: %d", file_path, area, track, output_format_ptr->start_lsn, output_format_ptr->length_lsn, output_format_ptr->dst_encoded_import, output_format_ptr->dsd_encoded_export));

        list_add_tail(&output_format_ptr->siblings, &output->ripping_queue);

        return 0;
    }
    return -1;
}

int scarletbook_output_enqueue_raw_sectors(scarletbook_output_t *output, int start_lsn, int length_lsn, char *file_path, char *fmt)
{
    scarletbook_format_handler_t const * handler;
    scarletbook_output_format_t * output_format_ptr;
    scarletbook_handle_t *sb_handle = output->sb_handle;

    if ((handler = find_output_format(fmt)))
    {
        output_format_ptr = calloc(sizeof(scarletbook_output_format_t), 1);
        output_format_ptr->sb_handle = sb_handle;
        output_format_ptr->cb_fwprintf = output->fwprintf_callback;
        output_format_ptr->handler = *handler;
        output_format_ptr->filename = strdup(file_path);
        output_format_ptr->start_lsn = start_lsn;
        output_format_ptr->length_lsn = length_lsn;

        LOG(lm_main, LOG_NOTICE, ("Queuing raw: %s, start_lsn: %d, length_lsn: %d", file_path, start_lsn, length_lsn));

        list_add_tail(&output_format_ptr->siblings, &output->ripping_queue);

        return 0;
    }
    return -1;
}

static int create_output_file(scarletbook_output_format_t *ft)
{
    int result;

#if defined(WIN32) || defined(_WIN32)
    wchar_t *wide_filename = (wchar_t *) charset_convert(ft->filename, strlen(ft->filename), "UTF-8", "UCS-2-INTERNAL");
    ft->fd = _wfopen(wide_filename, L"wb");
    free(wide_filename);
#else
    ft->fd = fopen(ft->filename, "wb");	
#endif
    if (ft->fd == NULL)
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

    // close_output_file will be called
    return -1;
}

static inline int close_output_file(scarletbook_output_format_t * ft)
{
    int result;

    result = ft->handler.stopwrite ? (*ft->handler.stopwrite)(ft) : 0;
    if(result ==-1)
	  LOG(lm_main, LOG_ERROR, ("error closing %s", ft->filename));
  
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

static void scarletbook_output_init_stats(scarletbook_output_t *output)
{
    struct list_head * node_ptr;
    scarletbook_output_format_t * output_format_ptr;

    output->stats_total_sectors = 0;
    output->stats_total_sectors_processed = 0;
    output->stats_current_file_total_sectors = 0;
    output->stats_current_file_sectors_processed = 0;
    output->stats_current_track = 0;
    output->stats_total_tracks = 0;

    list_for_each(node_ptr, &output->ripping_queue)
    {
        output_format_ptr = list_entry(node_ptr, scarletbook_output_format_t, siblings);
        output->stats_total_sectors += output_format_ptr->length_lsn;
        output->stats_total_tracks++;
    }
}

static inline int write_block(scarletbook_output_format_t * ft, const uint8_t *buf, size_t len)
{
    int actual = ft->handler.write? (*ft->handler.write)(ft, buf, len) : 0;
    if (actual < 0 ) return -1;
    ft->write_length += actual;
    return actual;
}

static void frame_decoded_callback(uint8_t* frame_data, size_t frame_size, void *userdata)
{
    scarletbook_output_format_t *ft = (scarletbook_output_format_t *) userdata;
    int rezult = write_block(ft, frame_data, frame_size);
    if (rezult == -1)
    {
	 ft->cb_fwprintf(stderr, L"\n ERROR in frame_decoded_callback():write_block()...at writting in file.\n");
	 LOG(lm_main, LOG_ERROR, ("ERROR in frame_decoded_callback():write_block()...writting in file: %s  ",ft->filename) );
	 raise(SIGINT);
	}
}

static void frame_error_callback(int frame_count, int frame_error_code, const char *frame_error_message, void *userdata)
{
    scarletbook_output_format_t *ft = (scarletbook_output_format_t *) userdata;

    ft->cb_fwprintf(stderr, L"\n ERROR %s in frame: %d\n", frame_error_message, frame_count);
}

static void frame_read_callback(scarletbook_handle_t *handle, uint8_t* frame_data, size_t frame_size, void *userdata)
{
    scarletbook_output_format_t *ft = (scarletbook_output_format_t *) userdata;


    if (ft->handler.flags & OUTPUT_FLAG_EDIT_MASTER) //  only for DSDIFF master
    {
        if (ft->dsd_encoded_export && ft->dst_encoded_import) 
        {
            dst_decoder_decode(ft->dst_decoder, frame_data, frame_size);
        }
        else
        {
            int rezult;
            rezult = write_block(ft, frame_data, frame_size);
            if (rezult == -1)
            {
                ft->cb_fwprintf(stderr, L"\n ERROR in frame_read_callback():write_block()..at writting in file. \n");
                LOG(lm_main, LOG_ERROR, ("ERROR in frame_read_callback:write_block()...writting in file: %s  ", ft->filename));
                raise(SIGINT);
            }
        }
    }
    else   // DSF, DSDIFF
    {
        if (ft->sb_handle->audio_frame_trimming > 0)  // (pausese will not be included)
        {
            uint32_t frame_count_time_start = TIME_FRAMECOUNT(&handle->area[ft->area].area_tracklist_time->start[ft->track]);
            uint32_t frame_count_time_end = TIME_FRAMECOUNT(&handle->area[ft->area].area_tracklist_time->start[ft->track]) +
                                            TIME_FRAMECOUNT(&handle->area[ft->area].area_tracklist_time->duration[ft->track]);
            uint32_t frame_timecode = TIME_FRAMECOUNT(&handle->audio_sector.frame[handle->frame_info_idx].timecode);

            if (frame_timecode >= frame_count_time_start &&
                frame_timecode <= frame_count_time_end)
            {
                if (ft->dsd_encoded_export && ft->dst_encoded_import)
                {
                    dst_decoder_decode(ft->dst_decoder, frame_data, frame_size);
                    ft->sb_handle->count_frames++;
                }
                else
                {
                    int rezult;
                    rezult = write_block(ft, frame_data, frame_size);
                    if (rezult == -1)
                    {
                        ft->cb_fwprintf(stderr, L"\n ERROR in frame_read_callback():write_block()..at writting in file. \n");
                        LOG(lm_main, LOG_ERROR, ("ERROR in frame_read_callback:write_block()...writting in file: %s  ", ft->filename));
                        raise(SIGINT);
                    }
                    ft->sb_handle->count_frames++;
                }
            }
        }
        else  // no audioframe trimming (pauses will be included)
        {
            if (ft->dsd_encoded_export && ft->dst_encoded_import)
            {
                dst_decoder_decode(ft->dst_decoder, frame_data, frame_size);
                ft->sb_handle->count_frames++;
            }
            else
            {
                int rezult;
                rezult = write_block(ft, frame_data, frame_size);
                if (rezult == -1)
                {
                    ft->cb_fwprintf(stderr, L"\n ERROR in frame_read_callback():write_block()..at writting in file. \n");
                    LOG(lm_main, LOG_ERROR, ("ERROR in frame_read_callback:write_block()...writting in file: %s  ", ft->filename));
                    raise(SIGINT);
                }
                ft->sb_handle->count_frames++;
            }
        }

    }
}

#ifdef __lv2ppu__
static void processing_thread(void *arg)
#else
static void *processing_thread(void *arg)
#endif
{
    scarletbook_output_t *output = (scarletbook_output_t *) arg;
    scarletbook_handle_t *handle = output->sb_handle;
    struct list_head * node_ptr;
    scarletbook_output_format_t *ft = NULL;
    int non_encrypted_disc = 0;
    int checked_for_non_encrypted_disc = 0;

    sysAtomicSet(&output->processing, 1);
    while (!list_empty(&output->ripping_queue))
    {
        node_ptr = output->ripping_queue.next;

        ft = list_entry(node_ptr, scarletbook_output_format_t, siblings);
        list_del(node_ptr);

        if (ft->dsd_encoded_export && ft->dst_encoded_import)
        {
            ft->dst_decoder = dst_decoder_create(ft->channel_count, frame_decoded_callback, frame_error_callback, ft);
        }

        output->stats_current_file_total_sectors = ft->length_lsn;
        output->stats_current_file_sectors_processed = 0;
        output->stats_current_track++;

        if (output->stats_track_callback)
        {
            output->stats_track_callback(ft->filename, output->stats_current_track, output->stats_total_tracks);
        }

        scarletbook_frame_init(handle);

        if (create_output_file(ft) == 0)
        {
            uint32_t block_size=0, end_lsn=0, blocks_readed = 0;
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

            handle->count_frames = 0;

            sysAtomicSet(&output->stop_processing, 0);

            while (sysAtomicRead(&output->stop_processing) == 0)
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
                    blocks_readed = sacd_read_block_raw(ft->sb_handle->sacd, ft->current_lsn, block_size, output->read_buffer);

                    if (blocks_readed == 0)
                    {
                        output->fwprintf_callback(stdout, L"\n \n Error:blocks_readed =0, current_lsn:%d, end_lsn:%d, block_size:%d \n", ft->current_lsn, end_lsn, block_size);
                        LOG(lm_main, LOG_ERROR, ("Error:blocks_readed = 0, current_lsn:%d, end_lsn:%d, block_size:%d", ft->current_lsn, end_lsn, block_size));                        
                        sysAtomicSet(&output->stop_processing, 1);
                    }

                    block_size=blocks_readed;
                    
                    ft->current_lsn += block_size;
                    output->stats_total_sectors_processed += block_size;
                    output->stats_current_file_sectors_processed += block_size;

                    // the ATAPI call which returns the flag if the disc is encrypted or not is unknown at this point. 
                    // user reports tell me that the only non-encrypted discs out there are DSD 3 14/16 discs. 
                    // this is a quick hack/fix for these discs.
                    if (encrypted && checked_for_non_encrypted_disc == 0)
                    {
                        switch (handle->area[ft->area].area_toc->frame_format)
                        {
                        case FRAME_FORMAT_DSD_3_IN_14:
                        case FRAME_FORMAT_DSD_3_IN_16:
                            non_encrypted_disc = *(uint64_t *)(output->read_buffer + 16) == 0;
                            break;
                        }

                        checked_for_non_encrypted_disc = 1;
                    }

                    // encrypted blocks need to be decrypted first
                    if (encrypted && non_encrypted_disc == 0)
                    {
                        sacd_decrypt(ft->sb_handle->sacd, output->read_buffer, block_size);
                    }

                    //debug
                    //output->fwprintf_callback(stdout, L"\n \n Debug - scarletbook_process_frames(): block_size %d, last bloc=%d \n", block_size, ft->current_lsn == end_lsn);

                    // process DSD & DST frames
                    if (ft->handler.flags & OUTPUT_FLAG_DSD || ft->handler.flags & OUTPUT_FLAG_DST)
                    {
                       int rezult_proc_frames =  scarletbook_process_frames(ft->sb_handle, output->read_buffer, block_size, ft->current_lsn == end_lsn, frame_read_callback, ft);
                       if (rezult_proc_frames < 0)
                           output->fwprintf_callback(stdout, L"\n \n Error in processing frames! \n");
                    }
                    // ISO output is written without frame processing                        
                    else if (ft->handler.flags & OUTPUT_FLAG_RAW)
                    {
                       size_t rezult=  write_block(ft, output->read_buffer, block_size);
					   if (rezult ==(size_t) -1) 
					   {
						   output->fwprintf_callback(stdout, L"\n \n Error in writting ISO in file. \n");
						   sysAtomicSet(&output->stop_processing, 1);
					   }
					    
                    }

                    // debug
                    //output->fwprintf_callback(stdout, L"\n \n After scarlet_processe_frames. Processed: %d audioframes\n", ft->count_frames);

                    // update statistics
                    if (output->stats_progress_callback)
                    {
                        output->stats_progress_callback(output->stats_total_sectors, output->stats_total_sectors_processed, 
                            output->stats_current_file_total_sectors, output->stats_current_file_sectors_processed);
                    }
                }
                else
                {
                    break;
                } // end if (ft->current_lsn < end_lsn)

            } // end while (sysAtomicRead(&output->stop_processing

        }  // end  if (create_output_file(ft)

        // Show statistics only for DSF/DFF : print Error if nr of processed frames < of duration (in nr of frames)
        if (ft->handler.flags & OUTPUT_FLAG_DSD || ft->handler.flags & OUTPUT_FLAG_DST)
        {
            uint32_t duration = (uint32_t)TIME_FRAMECOUNT(&handle->area[ft->area].area_tracklist_time->duration[ft->track]);
            if (handle->count_frames < duration) //output->stats_current_count_frames
            {
                output->fwprintf_callback(stdout, L"\n \n Warning:! Number of processed audioframes (%d) is smaller than number of frames in duration (%d) \n", handle->count_frames, duration);
            }
            output->fwprintf_callback(stdout, L"\n \n Processed %d audioframes. Duration specified: %d (%02d:%02d:%02d [mins:secs:frames])\n", 
                      handle->count_frames, duration, 
                      handle->area[ft->area].area_tracklist_time->duration[ft->track].minutes, 
                      handle->area[ft->area].area_tracklist_time->duration[ft->track].seconds, 
                      handle->area[ft->area].area_tracklist_time->duration[ft->track].frames);           
        }

        if (sysAtomicRead(&output->stop_processing) == 1)
        {
            output->fwprintf_callback(stdout, L"\n ...stop processing\n");
            // make a copy of the filename
            //char *file_to_remove = strdup(ft->filename);

            sysAtomicSet(&output->processing, 0);

            if (ft->dsd_encoded_export && ft->dst_encoded_import)
            {
                dst_decoder_destroy(ft->dst_decoder);
            }

            close_output_file(ft);

            // remove the file being worked on
#ifdef __lv2ppu__
           // if (sysFsUnlink(file_to_remove) != 0)
#else
           // if (remove(file_to_remove) != 0)
#endif
            //{
            //    LOG(lm_main, LOG_ERROR, ("user cancelled, error removing: %s, [%s]", file_to_remove, strerror(errno)));
            //}

            //free(file_to_remove);

            destroy_ripping_queue(output);
#ifdef __lv2ppu__
            sysThreadExit(-1);
#else
            pthread_exit(0);
#endif
        }

        if (ft->dsd_encoded_export && ft->dst_encoded_import)
        {
            dst_decoder_destroy(ft->dst_decoder);
        }

        close_output_file(ft);
    } 
    destroy_ripping_queue(output);
    sysAtomicSet(&output->processing, 0);

#ifdef __lv2ppu__
    sysThreadExit(-1);
#else
    pthread_exit(0);

    return 0;
#endif
}

scarletbook_output_t *scarletbook_output_create(scarletbook_handle_t *handle, stats_track_callback_t cb_track, stats_progress_callback_t cb_progress, fwprintf_callback_t cb_fwprintf)
{
    scarletbook_output_t *output = (scarletbook_output_t *) calloc(1, sizeof(scarletbook_output_t));

    INIT_LIST_HEAD(&output->ripping_queue);
    output->read_buffer = (uint8_t *) malloc(MAX_PROCESSING_BLOCK_SIZE * SACD_LSN_SIZE);
    output->sb_handle = handle;
    output->stats_track_callback = cb_track;
    output->stats_progress_callback = cb_progress;
    output->fwprintf_callback = cb_fwprintf;

    return output;
}

int scarletbook_output_is_busy(scarletbook_output_t *output)
{
    return sysAtomicRead(&output->processing);
}

int scarletbook_output_start(scarletbook_output_t *output)
{
    int ret = 0;

    scarletbook_output_init_stats(output);

#ifdef __lv2ppu__
    ret = sysThreadCreate(&output->processing_thread_id,
                          processing_thread,
                          (void *) output,
                          1050,
                          8192,
                          THREAD_JOINABLE,
                          "processing_thread");
#else
    ret = pthread_create(&output->processing_thread_id, NULL, processing_thread, (void *) output);
#endif
    if (ret)
    {
        LOG(lm_main, LOG_ERROR, ("return code from processing thread creation is %d\n", ret));
    }

    return ret;
}

void scarletbook_output_interrupt(scarletbook_output_t *output)
{
    sysAtomicSet(&output->stop_processing, 1);
}

int scarletbook_output_destroy(scarletbook_output_t *output)
{
#ifdef __lv2ppu__
    uint64_t thr_exit_code;
#else
    void *thr_exit_code;
#endif
    int ret = 0;

    if (!output)
        return -1;

#ifdef __lv2ppu__
    scarletbook_output_interrupt(output);
    ret = sysThreadJoin(output->processing_thread_id, &thr_exit_code);
#else
    scarletbook_output_interrupt(output);
    ret = pthread_join(output->processing_thread_id, &thr_exit_code);
#endif    
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("processing thread didn't close properly... %x", thr_exit_code));
    }

    // If decoding is aborted (eg. ctrl+C), then free() buffers after the decoder has been destroyed,
    // to ensure that buffers aren't still in use when they're free()d.
    free(output->read_buffer);
    free(output);

    return ret;
}
