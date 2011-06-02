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

#ifndef SCARLETBOOK_OUTPUT_H_INCLUDED
#define SCARLETBOOK_OUTPUT_H_INCLUDED

#ifdef __lv2ppu__
#include <sys/atomic.h>
#endif

#include "scarletbook.h"

#define PACKET_FRAME_BUFFER_COUNT 5

// forward declaration
typedef struct scarletbook_output_format_t scarletbook_output_format_t;

// Handler structure defined by each output format.
typedef struct scarletbook_format_handler_t 
{
    char const *description;
    char const *name;
    int (*startwrite)(scarletbook_output_format_t *ft);
    size_t (*write)(scarletbook_output_format_t *ft, const uint8_t *buf, size_t len, int last_frame);
    int (*stopwrite)(scarletbook_output_format_t *ft);
    int         preprocess_audio_frames;
    size_t      priv_size;
} 
scarletbook_format_handler_t;

struct scarletbook_output_format_t 
{
    int                             area;
    int                             track;
    uint32_t                        start_lsn;
    uint32_t                        length_lsn;
    uint32_t                        current_lsn;
    char                           *filename;

    int                             fd;
    uint64_t                        write_length;
    uint64_t                        write_offset;
    int                             dst_encoded;
    int                             encrypted;

    scarletbook_format_handler_t    handler;
    void                           *priv;

    int                             error_nr;
    char                            error_str[256];

    scarletbook_handle_t           *sb_handle;

    struct list_head                siblings;
}; 

typedef void (*stats_callback_t)(uint32_t stats_total_sectors, uint32_t stats_total_sectors_processed,
                                 uint32_t stats_current_file_total_sectors, uint32_t stats_current_file_sectors_processed,
                                 char *filename);

typedef struct audio_frame_t
{
    struct list_head    siblings;
    uint8_t            *data;
    int                 size;
    int                 full;

} audio_frame_t;


typedef struct scarletbook_output_t
{
    int                 initialized;

    struct list_head    ripping_queue;

#ifdef __lv2ppu__
    // processing 
    sys_cond_t          processing_cond;
    sys_mutex_t         processing_mutex;

    sys_ppu_thread_t    processing_thread_id;

    atomic_t            stop_processing;            // indicates if the thread needs to stop or has stopped
    atomic_t            outstanding_read_requests;
#else
    int                 stop_processing;
#endif

    audio_sector_t      scarletbook_audio_sector;

    // we pre-cache our processed frames so we can do 
    // parallel DST conversion using 5 SPUs
    struct list_head    frames;
    audio_frame_t      *frame;
    uint8_t            *current_frame_ptr;
    int                 full_frame_count;

    // stats
    uint32_t            stats_total_sectors;
    uint32_t            stats_total_sectors_processed;
    uint32_t            stats_current_file_total_sectors;
    uint32_t            stats_current_file_sectors_processed;
    stats_callback_t    stats_callback;
}
scarletbook_output_t;

void init_stats(stats_callback_t);

scarletbook_format_handler_t const * sacd_find_output_format(char const *);

void initialize_ripping(void);
int start_ripping(scarletbook_handle_t *);
int stop_ripping(scarletbook_handle_t *);
int queue_track_to_rip(int area, int track, char *file_path, char *fmt, 
                                uint32_t start_lsn, uint32_t length_lsn, int dst_encoded);

#endif /* SCARLETBOOK_OUTPUT_H_INCLUDED */
