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
#include <sys/thread.h>
#else
#include <pthread.h>
#endif
#include <sys/atomic.h>

#include "scarletbook.h"
#ifdef __lv2ppu__
#include "dst_decoder.h"
#endif

#define PACKET_FRAME_BUFFER_COUNT 5

// forward declaration
typedef struct scarletbook_output_format_t scarletbook_output_format_t;

enum
{
    OUTPUT_FLAG_RAW = 1 << 0,
    OUTPUT_FLAG_DSD = 1 << 1,
    OUTPUT_FLAG_DST = 1 << 2
};

// Handler structure defined by each output format.
typedef struct scarletbook_format_handler_t 
{
    char const *description;
    char const *name;
    int (*startwrite)(scarletbook_output_format_t *ft);
    size_t (*write)(scarletbook_output_format_t *ft, const uint8_t *buf, size_t len, int last_frame);
    int (*stopwrite)(scarletbook_output_format_t *ft);
    int         flags;
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

    FILE                           *fd;
    char                           *write_cache;
    uint64_t                        write_length;
    uint64_t                        write_offset;

    int                             dst_encoded;
    int                             channel_count;

    int                             decode_dst;

    scarletbook_format_handler_t    handler;
    void                           *priv;

    int                             error_nr;
    char                            error_str[256];

    scarletbook_handle_t           *sb_handle;

    struct list_head                siblings;
}; 

typedef void (*stats_progress_callback_t)(uint32_t stats_total_sectors, uint32_t stats_total_sectors_processed,
                                          uint32_t stats_current_file_total_sectors, uint32_t stats_current_file_sectors_processed);

typedef void (*stats_track_callback_t)(char *filename, int current_track, int total_tracks);

typedef struct audio_frame_t
{
    struct list_head    siblings;
    uint8_t            *data;
    int                 size;
    int                 full;
    int                 dst_encoded;
} audio_frame_t;


typedef struct scarletbook_output_t
{
    int                 initialized;

    struct list_head    ripping_queue;

    uint8_t            *read_buffer;

#ifdef __lv2ppu__
    sys_ppu_thread_t    read_thread_id;
#else
    pthread_t           read_thread_id;
#endif
    atomic_t            stop_processing;            // indicates if the thread needs to stop or has stopped
    atomic_t            processing;

    audio_sector_t      scarletbook_audio_sector;

    // we pre-cache our processed frames so we can do 
    // parallel DST conversion using 5 SPUs
    struct list_head    frames;
    audio_frame_t      *frame;
    uint8_t            *current_frame_ptr;
    int                 full_frame_count;
    uint8_t            *dsd_data;


    // stats
    int                 stats_total_tracks;
    int                 stats_current_track;
    uint32_t            stats_total_sectors;
    uint32_t            stats_total_sectors_processed;
    uint32_t            stats_current_file_total_sectors;
    uint32_t            stats_current_file_sectors_processed;
    stats_progress_callback_t stats_progress_callback;
    stats_track_callback_t stats_track_callback;

#ifdef __lv2ppu__
    dst_decoder_t       dst_decoder;
#endif
}
scarletbook_output_t;

void init_stats(stats_track_callback_t, stats_progress_callback_t);

scarletbook_format_handler_t const * sacd_find_output_format(char const *);

int initialize_ripping(void);
void interrupt_ripping(void);
int is_ripping(void);
int start_ripping(scarletbook_handle_t *);
int stop_ripping(scarletbook_handle_t *);
int queue_track_to_rip(int area, int track, char *file_path, char *fmt, 
                                uint32_t start_lsn, uint32_t length_lsn, int dst_encoded, int decode_dst);

#endif /* SCARLETBOOK_OUTPUT_H_INCLUDED */
