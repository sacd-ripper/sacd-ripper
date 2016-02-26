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
#include <inttypes.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#ifdef __lv2ppu__
#include <sys/file.h>
#elif defined(WIN32)
#include <io.h>
#endif

#include "sacd_reader.h"
#include "scarletbook_id3.h"
#include "scarletbook_output.h"
#include "endianess.h"
#include "version.h"
#include "scarletbook.h"
#include "dsf.h"

#define DSF_BUFFER_SIZE    2048

typedef struct
{
    uint8_t            *header;
    size_t              header_size;
    uint8_t            *footer;
    size_t              footer_size;

    uint64_t            audio_data_size;

    int                 channel_count;
    uint64_t            sample_count;

    uint8_t             buffer[MAX_CHANNEL_COUNT][SACD_BLOCK_SIZE_PER_CHANNEL];
    uint8_t            *buffer_ptr[MAX_CHANNEL_COUNT];
} 
dsf_handle_t;

static const uint8_t bit_reverse_table[] = 
{
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0, 
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4, 
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc, 
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2, 
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6, 
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9, 
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3, 
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7, 
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

static int dsf_create_header(scarletbook_output_format_t *ft)
{
    dsd_chunk_header_t *dsd_chunk;
    uint8_t          *write_ptr;
    scarletbook_handle_t *sb_handle = ft->sb_handle;
    dsf_handle_t  *handle = (dsf_handle_t *) ft->priv;

    if (!handle->header)
        handle->header = (uint8_t *) calloc(DSF_BUFFER_SIZE, 1);
    if (!handle->footer)
        handle->footer = (uint8_t *) calloc(DSF_BUFFER_SIZE, 1);
    handle->header_size = 0;
    handle->footer_size = 0;

    write_ptr = handle->header;

    dsd_chunk            = (dsd_chunk_header_t *) handle->header;
    dsd_chunk->chunk_id  = DSD_MARKER;
    dsd_chunk->chunk_data_size = htole64(DSD_CHUNK_HEADER_SIZE);

    handle->header_size += DSD_CHUNK_HEADER_SIZE;
    write_ptr            = (uint8_t *) handle->header + DSD_CHUNK_HEADER_SIZE;

    {
        fmt_chunk_t *fmt_chunk              = (fmt_chunk_t *) write_ptr;
        area_toc_t *area_toc                = sb_handle->area[ft->area].area_toc;

        fmt_chunk->chunk_id                 = FMT_MARKER;
        fmt_chunk->chunk_data_size          = htole64(FMT_CHUNK_SIZE);
        fmt_chunk->version                  = htole32(DSF_VERSION);
        fmt_chunk->format_id                = htole32(FORMAT_ID_DSD);
        if (area_toc->channel_count == 2 && area_toc->extra_settings == 0)
        {
            fmt_chunk->channel_type         = htole32(CHANNEL_TYPE_STEREO);
        }
        else if (area_toc->channel_count == 5 && area_toc->extra_settings == 3)
        {
            fmt_chunk->channel_type         = htole32(CHANNEL_TYPE_5_CHANNELS);
        }
        else if (area_toc->channel_count == 6 && area_toc->extra_settings == 4)
        {
            fmt_chunk->channel_type         = htole32(CHANNEL_TYPE_5_1_CHANNELS);
        }
        else
        {
            fmt_chunk->channel_type         = htole32(CHANNEL_TYPE_STEREO);
        }
        fmt_chunk->channel_count            = htole32(area_toc->channel_count);
        fmt_chunk->sample_frequency         = htole32(SACD_SAMPLING_FREQUENCY);
        fmt_chunk->bits_per_sample          = htole32(SACD_BITS_PER_SAMPLE);
        fmt_chunk->sample_count             = htole64(handle->sample_count / area_toc->channel_count * 8);
        fmt_chunk->block_size_per_channel   = htole32(SACD_BLOCK_SIZE_PER_CHANNEL);
        fmt_chunk->reserved                 = 0;

        handle->channel_count = area_toc->channel_count;
        handle->header_size += FMT_CHUNK_SIZE;
        write_ptr += FMT_CHUNK_SIZE;
    }

    {
        data_chunk_t * data_chunk;
        data_chunk                  = (data_chunk_t *) write_ptr;
        data_chunk->chunk_id        = DATA_MARKER;
        data_chunk->chunk_data_size = htole64(DATA_CHUNK_SIZE + handle->audio_data_size);

        write_ptr += DATA_CHUNK_SIZE;
        handle->header_size += DATA_CHUNK_SIZE;
    }

    {
        handle->footer_size = scarletbook_id3_tag_render(sb_handle, handle->footer, ft->area, ft->track);
    }

    dsd_chunk->total_file_size = htole64(handle->header_size + handle->audio_data_size + handle->footer_size);
    dsd_chunk->metadata_offset = htole64(handle->footer_size ? handle->header_size + handle->audio_data_size : 0);

    fwrite(handle->header, 1, handle->header_size, ft->fd);

    return 0;
}


static int dsf_create(scarletbook_output_format_t *ft)
{
    return dsf_create_header(ft);
}

static int dsf_close(scarletbook_output_format_t *ft)
{
    dsf_handle_t *handle = (dsf_handle_t *) ft->priv;
    int i;

    // write out what was left in the ring buffers
    for (i = 0; i < handle->channel_count; i++)
    {
	if (handle->buffer_ptr[i] > handle->buffer[i]) {
	    handle->sample_count += handle->buffer_ptr[i] - handle->buffer[i];

	    fwrite(handle->buffer[i], 1, SACD_BLOCK_SIZE_PER_CHANNEL, ft->fd);
	    memset(handle->buffer[i], 0, SACD_BLOCK_SIZE_PER_CHANNEL);

	    handle->buffer_ptr[i] = handle->buffer[i];
	    handle->audio_data_size += SACD_BLOCK_SIZE_PER_CHANNEL;
	}
    }

    // write the footer
    fwrite(handle->footer, 1, handle->footer_size, ft->fd);
    fseek(ft->fd, 0, SEEK_SET);
    
    // write the final header
    dsf_create_header(ft);

    if (handle->header)
        free(handle->header);
    if (handle->footer)
        free(handle->footer);

    return 0;
}

static size_t dsf_write_frame(scarletbook_output_format_t *ft, const uint8_t *buf, size_t len)
{
    dsf_handle_t *handle = (dsf_handle_t *) ft->priv;
    const uint8_t *buf_end_ptr = buf + len;
    const uint8_t *buf_ptr = buf;
    uint64_t prev_audio_data_size = handle->audio_data_size;
    int i;

    while(buf_ptr < buf_end_ptr)
    {
        for (i = 0; i < handle->channel_count; i++)
        {
            if (!handle->buffer_ptr[i])
            {
                handle->buffer_ptr[i] = handle->buffer[i];
            }

            if (handle->buffer_ptr[i] < handle->buffer[i] + SACD_BLOCK_SIZE_PER_CHANNEL && 
                handle->buffer_ptr[i] < handle->buffer[i] + len / handle->channel_count)
            {
                *handle->buffer_ptr[i] = bit_reverse_table[*buf_ptr];

                handle->buffer_ptr[i]++;
                buf_ptr++;
            } else {
		handle->sample_count += handle->buffer_ptr[i] - handle->buffer[i];

		fwrite(handle->buffer[i], 1, SACD_BLOCK_SIZE_PER_CHANNEL, ft->fd);
		memset(handle->buffer[i], 0, SACD_BLOCK_SIZE_PER_CHANNEL);

		handle->buffer_ptr[i] = handle->buffer[i];
		handle->audio_data_size += SACD_BLOCK_SIZE_PER_CHANNEL;
	    }
        }
    }

    return (size_t) (handle->audio_data_size - prev_audio_data_size);
}

scarletbook_format_handler_t const * dsf_format_fn(void) 
{
    static scarletbook_format_handler_t handler = 
    {
        "Sony DSD stream file (dsf)", 
        "dsf", 
        dsf_create, 
        dsf_write_frame,
        dsf_close, 
        OUTPUT_FLAG_DSD,
        sizeof(dsf_handle_t)
    };
    return &handler;
}
