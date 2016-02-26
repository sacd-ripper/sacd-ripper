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

#ifndef DSF_H_INCLUDED
#define DSF_H_INCLUDED

#include <inttypes.h>
#include "endianess.h"

#undef ATTRIBUTE_PACKED
#undef PRAGMA_PACK_BEGIN
#undef PRAGMA_PACK_END

#if defined(__GNUC__)
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#define ATTRIBUTE_PACKED    __attribute__ ((packed))
#define PRAGMA_PACK         0
#endif
#endif

/**
 * A first year Sony intern must have come up with this format! it's so badly put together 
 * that I should actually not have written this code in the first place..
 */

#if !defined(ATTRIBUTE_PACKED)
#define ATTRIBUTE_PACKED
#define PRAGMA_PACK                            1
#endif

#define DSD_MARKER                             (MAKE_MARKER('D', 'S', 'D', ' ')) // DSD Chunk Header
#define FMT_MARKER                             (MAKE_MARKER('f', 'm', 't', ' ')) // fmt chunkheader
#define DATA_MARKER                            (MAKE_MARKER('d', 'a', 't', 'a')) // data chunkheader

#define DSF_VERSION                         1

#define FORMAT_ID_DSD                       0

// Audiogate cannot deal with "regular" DSD stored as MSB and 1 byte per channel so we do the  
// LSB conversion and set the blocksize to 4096, argh..

#define SACD_BLOCK_SIZE_PER_CHANNEL         4096   // 4096 bytes per channel
#define SACD_BITS_PER_SAMPLE                1      // LSB

enum
{
    CHANNEL_TYPE_MONO           = 1,
    CHANNEL_TYPE_STEREO         = 2,
    CHANNEL_TYPE_3_CHANNELS     = 3,
    CHANNEL_TYPE_QUAD           = 4,
    CHANNEL_TYPE_4_CHANNELS     = 5,
    CHANNEL_TYPE_5_CHANNELS     = 6,
    CHANNEL_TYPE_5_1_CHANNELS   = 7
};

#if PRAGMA_PACK
#pragma pack(1)
#endif

struct dsd_chunk_header_t
{
    uint32_t chunk_id;

    uint64_t chunk_data_size;

    uint64_t total_file_size;

    uint64_t metadata_offset;

} ATTRIBUTE_PACKED;
typedef struct dsd_chunk_header_t   dsd_chunk_header_t;
#define DSD_CHUNK_HEADER_SIZE    28U

struct fmt_chunk_t
{
    uint32_t chunk_id;

    uint64_t chunk_data_size;

    uint32_t version;

    uint32_t format_id;

    uint32_t channel_type;

    uint32_t channel_count;

    uint32_t sample_frequency;

    uint32_t bits_per_sample;

    uint64_t sample_count;

    uint32_t block_size_per_channel;
    
    uint32_t reserved;

} ATTRIBUTE_PACKED;
typedef struct fmt_chunk_t   fmt_chunk_t;
#define FMT_CHUNK_SIZE    52U

struct data_chunk_t
{
    uint32_t chunk_id;

    uint64_t chunk_data_size;

    uint8_t  data[];
} ATTRIBUTE_PACKED;
typedef struct data_chunk_t   data_chunk_t;
#define DATA_CHUNK_SIZE    12U

#if PRAGMA_PACK
#pragma pack()
#endif

#endif  /* DSF_H_INCLUDED */
