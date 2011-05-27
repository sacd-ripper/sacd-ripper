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

#include "scarletbook.h"

// forward declaration
typedef struct scarletbook_output_format_t scarletbook_output_format_t;

// Handler structure defined by each output format.
typedef struct scarletbook_format_handler_t 
{
    char const *description;
    char const *name;
    int (*startwrite)(scarletbook_output_format_t *ft);
    size_t (*write)(scarletbook_output_format_t *ft, const uint8_t *buf, size_t len);
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

scarletbook_format_handler_t const * sacd_find_output_format(char const *);

int start_ripping(scarletbook_handle_t *);
void stop_ripping(scarletbook_handle_t *);
int queue_track_to_rip(int area, int track, char *file_path, char *fmt, 
                                uint32_t start_lsn, uint32_t length_lsn, int dst_encoded);

#endif /* SCARLETBOOK_OUTPUT_H_INCLUDED */
