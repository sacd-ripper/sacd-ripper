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

#ifndef DSDIFF_H_INCLUDED
#define DSDIFF_H_INCLUDED

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

#if !defined(ATTRIBUTE_PACKED)
#define ATTRIBUTE_PACKED
#define PRAGMA_PACK                            1
#endif

#define FRM8_MARKER                            (MAKE_MARKER('F', 'R', 'M', '8')) // Format Version Chunk

// local chunks
#define FVER_MARKER                            (MAKE_MARKER('F', 'V', 'E', 'R')) // Format Version Chunk (required)
#define PROP_MARKER                            (MAKE_MARKER('P', 'R', 'O', 'P')) // Property Chunk (required)
        #define SND_MARKER                     (MAKE_MARKER('S', 'N', 'D', ' ')) // Sound properties
                #define FS_MARKER              (MAKE_MARKER('F', 'S', ' ', ' ')) // Sample Rate Chunk
                #define CHNL_MARKER            (MAKE_MARKER('C', 'H', 'N', 'L')) // Channels Chunk
                        #define SLFT_MARKER    (MAKE_MARKER('S', 'L', 'F', 'T')) // stereo left
                        #define SRGT_MARKER    (MAKE_MARKER('S', 'R', 'G', 'T')) // stereo right
                        #define MLFT_MARKER    (MAKE_MARKER('M', 'L', 'F', 'T')) // multi-channel left
                        #define MRGT_MARKER    (MAKE_MARKER('M', 'R', 'G', 'T')) // multi-channel right
                        #define LS_MARKER      (MAKE_MARKER('L', 'S', ' ', ' ')) // multi-channel left surround
                        #define RS_MARKER      (MAKE_MARKER('R', 'S', ' ', ' ')) // multi-channel right surround
                        #define C_MARKER       (MAKE_MARKER('C', ' ', ' ', ' ')) // multi-channel center
                        #define LFE_MARKER     (MAKE_MARKER('L', 'F', 'E', ' ')) // multi-channel low frequency enhancement
                #define CMPR_MARKER            (MAKE_MARKER('C', 'M', 'P', 'R')) // Compression Type Chunk
                #define ABSS_MARKER            (MAKE_MARKER('A', 'B', 'S', 'S')) // Absolute Start Time Chunk
                #define LSCO_MARKER            (MAKE_MARKER('L', 'S', 'C', 'O')) // Loudspeaker Configuration Chunk
#define DSD_MARKER                             (MAKE_MARKER('D', 'S', 'D', ' ')) // DSD Sound Data Chunk (required or DST)
#define DST_MARKER                             (MAKE_MARKER('D', 'S', 'T', ' ')) // DST Sound Data Chunk (required or DSD)
        #define FRTE_MARKER                    (MAKE_MARKER('F', 'R', 'T', 'E')) // DST Frame Information Chunk
        #define DSTF_MARKER                    (MAKE_MARKER('D', 'S', 'T', 'F')) // DST Frame Data Chunk
        #define DSTC_MARKER                    (MAKE_MARKER('D', 'S', 'T', 'C')) // DST Frame CRC Chunk
#define DSTI_MARKER                            (MAKE_MARKER('D', 'S', 'T', 'I')) // DST Sound Index Chunk (optional)
#define COMT_MARKER                            (MAKE_MARKER('C', 'O', 'M', 'T')) // comments Chunk (optional)
#define DIIN_MARKER                            (MAKE_MARKER('D', 'I', 'I', 'N')) // Edited Master Information Chunk (optional)
        #define EMID_MARKER                    (MAKE_MARKER('E', 'M', 'I', 'D')) // Edited Master ID Chunk (optional)
        #define MARK_MARKER                    (MAKE_MARKER('M', 'A', 'R', 'K')) // Edited Master ID Chunk (optional)
        #define DIAR_MARKER                    (MAKE_MARKER('D', 'I', 'A', 'R')) // Artist Chunk (optional)
        #define DITI_MARKER                    (MAKE_MARKER('D', 'I', 'T', 'I')) // Title Chunk (optional)
#define MANF_MARKER                            (MAKE_MARKER('M', 'A', 'N', 'F')) // Manufacturer Specific Chunk (optional)

#define DSDIFF_VERSION                         0x01050000
#define DSDIFF_VERSION_STRING                  "version 1.5.0.0"

#define CEIL_ODD_NUMBER(x)    ((x) % 2 ? (x) + 1 : (x))
#define CALC_CHUNK_SIZE(x)    (hton64(CEIL_ODD_NUMBER(x)))

enum
{
    LS_CONFIG_2_CHNL    = 0,                                            // 2-channel stereo set-up
    LS_CONFIG_5_CHNL    = 3,                                            // 5-channel set-up according to ITU-R BS.775-1 [ITU]
    LS_CONFIG_6_CHNL    = 4,                                            // 6-channel set-up, 5-channel set-up according to ITU-R BS.775-1 [ITU]
    LS_CONFIG_UNDEFINED = 65535                                         // undefined
};

enum
{
    COMT_TYPE_GENERNAL     = 0,                              // General (album) comment
    COMT_TYPE_CHANNEL      = 1,                              // Channel comment
    COMT_TYPE_SOUND_SOURCE = 2,                              // Sound Source
    COMT_TYPE_FILE_HISTORY = 3                               // File History
};

enum
{
    COMT_TYPE_CHANNEL_ALL = 0,                               // all channels
    COMT_TYPE_CHANNEL_1   = 1,                               // first channel in the file
    COMT_TYPE_CHANNEL_2   = 2,                               // second channel in the file
    COMT_TYPE_CHANNEL_3   = 3,                               // third channel in the file
    COMT_TYPE_CHANNEL_4   = 4,                               // ..
    COMT_TYPE_CHANNEL_5   = 5,
    COMT_TYPE_CHANNEL_6   = 6
};

enum
{
    COMT_TYPE_CHANNEL_SOUND_SOURCE_DSD      = 0,                     // DSD recording
    COMT_TYPE_CHANNEL_SOUND_SOURCE_ANALOGUE = 1,                     // Analogue recording
    COMT_TYPE_CHANNEL_SOUND_SOURCE_PCM      = 2                      // PCM recording
};

enum
{
    COMT_TYPE_CHANNEL_FILE_HISTORY_GENERAL          = 0,     // General Remark
    COMT_TYPE_CHANNEL_FILE_HISTORY_OPERATOR_NAME    = 1,     // Name of the operator
    COMT_TYPE_CHANNEL_FILE_HISTORY_CREATING_MACHINE = 2,     // Name or type of the creating machine
    COMT_TYPE_CHANNEL_FILE_HISTORY_TIME_ZONE        = 3,     // Time zone information
    COMT_TYPE_CHANNEL_FILE_HISTORY_REVISION         = 4      // Revision of the file
};

enum
{
    MARK_MARKER_TYPE_TRACKSTART   = 0,      // TrackStart Entry point for a Track start
    MARK_MARKER_TYPE_TRACKSTOP    = 1,      // TrackStop Entry point for ending a Track
    MARK_MARKER_TYPE_PROGRAMSTART = 2,      // ProgramStart Start point of 2-channel or multi-channel area
    MARK_MARKER_TYPE_INDEX_ENTRY  = 4       // Index Entry point of an Index
};

#if PRAGMA_PACK
#pragma pack(1)
#endif

//Bit 0 TMF4_Mute Indicates whether the 4th channel of this Track is muted
//Bit 1 TMF1_Mute Indicates whether the 1st and 2nd channel of this Track are muted
//Bit 2 TMF2_Mute Indicates whether:
//a) channel_count = 5, the 4th and 5th channel of this Track are muted
//b) channel_count = 6, the 5th and 6th channel of this Track are muted
//Bit 3 TMF3_Mute Indicates whether the 3rd channel of this Track is muted
//Bit 8..10 extra_use Indicates whether the 4th channel is used for an LFE loudspeaker

struct chunk_header_t
{
    // chunk_id describes the format of the chunk's data portion. An application can determine how to
    // interpret the chunk data by examining chunk_id.
    uint32_t chunk_id;

    // chunk_data_size is the size of the data portion of the chunk, in bytes. It does not include the 12
    // bytes used by chunk_id and chunk_data_size.
    uint64_t chunk_data_size;

    // chunk_data[] is the data stored in the chunk. The format of this data is determined by chunk_id. If
    // the data is an odd number of bytes in length, a pad byte must be added at the end. The pad
    // byte is not included in chunk_data_size.
    uint8_t chunk_data[0xffff];
} ATTRIBUTE_PACKED;
typedef struct chunk_header_t   chunk_header_t;
#define CHUNK_HEADER_SIZE    12U


// The FORM chunk of form_type 'DSD ' is called a Form DSD Chunk.
//
// The Form DSD Chunk is required. It may appear only once in the file.
struct form_dsd_chunk_t
{
    // chunk_id is always 'FRM8'. This indicates that this is a Form DSD Chunk. Note that this FORM
    // chunk is slightly different from EA IFF 85 (the chunk_data_size is not a long but a double
    // uint32_t). Using the Form DSD Chunk makes it possible to identify that all local chunks have
    // a chunk_data_size of 8 bytes in length.
    uint32_t chunk_id;                    // 'FRM8'

    // chunk_data_size contains the size of the data portion of the 'FRM8' chunk. Note that the data
    // portion consists of two parts, form_type and frm8_chunks[].
    uint64_t chunk_data_size;                    // FORM's data size, in bytes

    // form_type describes the contents of in the 'FRM8' chunk. For DSDIFF files, form_type is
    // always 'DSD '. This indicates that the chunks within the FORM pertain to sampled sound
    // according to this DSDIFF standard.
    uint32_t form_type;                    // 'DSD '

    // frm8_chunks[] are the chunks within the Form DSD Chunk. These chunks are called local
    // chunks since their own chunk_id's are local to (i.e. specific for) the Form DSD chunk. A Form
    // DSD Chunk together with its local chunks make up a DSDIFF file.
    chunk_header_t *frm8_chunks[];
} ATTRIBUTE_PACKED;
typedef struct form_dsd_chunk_t   form_dsd_chunk_t;
#define FORM_DSD_CHUNK_SIZE    16U


// The Format Version Chunk contains a field indicating the format specification version for
// the DSDIFF file.
//
// The Format Version Chunk can be used to check which chunks of the file are supported.
//
// The Format Version Chunk is required and must be the first chunk in the Form DSD
// Chunk. It may appear only once in the Form DSD Chunk.
struct format_version_chunk_t
{
    // chunk_id is always 'FVER'.
    uint32_t chunk_id;            // 'FVER'

    // chunk_data_size is the size of the data portion of the chunk, in bytes. It does not include the 12
    // bytes used by chunk_id and chunk_data_size. For this chunk, chunk_data_size is 4.
    uint64_t chunk_data_size;            // 4

    // version indicates the version id. It consists of 4 bytes and defines the version number of the
    // file format (version 'byte1'.'byte2'.'byte3'.'byte4' ; from most significant byte to least
    // significant byte).
    // The first byte (most significant byte) defines the "main version number".
    // The second byte defines the "addition version number", indicating additions to the
    // description with respect to previous versions of this document. The possible changes are
    // additional definitions and/or chunks.
    // The last two bytes of version are reserved for future use and are set to zero.
    // A DSDIFF reader with the same "main version number" can still read information but will
    // not recognise additional chunk(s) that are defined in a later version of this document
    // (backwards compatible).
    // The version number of this description is 1.5.0.0.
    uint32_t version;            // 0x01050000 version 1.5.0.0 DSDIFF
} ATTRIBUTE_PACKED;
typedef struct format_version_chunk_t   format_version_chunk_t;
#define FORMAT_VERSION_CHUNK_SIZE    16U

// The Property Chunk is a container chunk, which consists of "local property chunks". These
// "local property chunks" define fundamental parameters of the defined property type.
//
// The Property Chunk is required and must precede the Sound Data Chunk. It may appear
// only once in the Form DSD Chunk.
struct property_chunk_t
{
    // chunk_id is always 'PROP'. It indicates that this is the property container chunk (of type
    // property_type).
    uint32_t chunk_id;                    // 'PROP'

    // chunk_data_size is the summation of the sizes of all local property chunks plus the size of
    // property_type. It does not include the 12 bytes used by chunk_id and chunk_data_size.
    uint64_t chunk_data_size;

    // property_type defines the type to which the "local property chunks" belong. Defined types are 'SND '
    // Other types are reserved for future use and may be defined in future versions.
    // The Property Chunk must precede the Sound Data Chunk.
    uint32_t property_type;                    // 'SND '

    // property_chunks[] are local property chunks. There is no order imposed on these local chunks.
    // The local chunks are :
    // - Sample Rate Chunk
    // - Channels Chunk
    // - Compression Type Chunk
    // - Absolute Start Time Chunk
    // - Loudspeaker Configuration Chunk
    chunk_header_t property_chunks[];      // local chunks
} ATTRIBUTE_PACKED;
typedef struct property_chunk_t   property_chunk_t;
#define PROPERTY_CHUNK_SIZE    16U


// The Sample Rate Chunk defines the sample rate at which the sound data has been sampled.
//
// The Sample Rate Chunk is required and may appear only once in the Property Chunk.
struct sample_rate_chunk_t
{
    // chunk_id is always 'FS '.
    uint32_t chunk_id;                    // 'FS '

    // chunk_data_size is the size of the data portion of the chunk, in bytes. It does not include the 12
    // bytes used by chunk_id and chunk_data_size. For this chunk, chunk_data_size is 4.
    uint64_t chunk_data_size;                    // 4

    // sample_rate indicates how many samples fit in one second. Because DSD signals are 1 bit
    // wide this number gives the total number of bits in one second per channel.
    uint32_t sample_rate;                    // sample rate in [Hz]
} ATTRIBUTE_PACKED;
typedef struct sample_rate_chunk_t   sample_rate_chunk_t;
#define SAMPLE_RATE_CHUNK_SIZE    16U


// The Channels Chunk defines the total number of channels and the channel ID's used in the
// Sound Data Chunk. The order of the channel ID's also determines the order of the sound
// data in the file.
//
// The Channels Chunk is required and may appear only once in the Property Chunk.
struct channels_chunk_t
{
    // chunk_id is always 'CHNL'.
    uint32_t chunk_id;                    // 'CHNL'

    // chunk_data_size is the size of the data portion of the chunk, in bytes. It does not include the 12
    // bytes used by chunk_id and chunk_data_size.
    uint64_t chunk_data_size;

    // channel_count contains the number of audio channels in the file. A value of 1 means one
    // channel, 2 means two channels, etc.. Any number, greater than zero, of audio channels may
    // be represented.
    uint16_t channel_count;                    // number of audio channels

    // channel_ds[] defines the channel ID for each of the channel_count channels. The order in which
    // they are stored in the array is the order in which they are stored in the Sound Data Chunk.
    // There are a number of predefined channel ID's:
    // 'SLFT' : stereo left
    // 'SRGT' : stereo right
    // 'MLFT' : multi-channel left
    // 'MRGT' : multi-channel right
    // 'LS ' : multi-channel left surround
    // 'RS ' : multi-channel right surround
    // 'C ' : multi-channel center
    // 'LFE ' : multi-channel low frequency enhancement
    // ['C000' .. 'C999']: context specific channel contents
    // To prevent misinterpretations the following restrictions apply :
    // -    If the file contains 2 channels with channel ID's 'SLFT' and 'SRGT' the order must be:
    //      'SLFT', 'SRGT'.
    // -    If the file contains 5 channels with channel ID's 'MLFT' and 'MRGT' and 'C ' and
    //      'LS ' and 'RS ' the order must be:
    //      'MLFT', 'MRGT', 'C ', 'LS ', 'RS '.
    // -    If the file contains 6 channels with channel ID's 'MLFT' and 'MRGT' and 'C ' and
    //      'LFE ' and 'LS ' and 'RS ' the order must be:
    //      'MLFT', 'MRGT', 'C ', 'LFE ', 'LS ', 'RS '.
    uint32_t channel_ids[255];                    // channels ID's
} ATTRIBUTE_PACKED;
typedef struct channels_chunk_t   channels_chunk_t;
#define CHANNELS_CHUNK_SIZE    14U


// The Compression Type Chunk defines the compression/decompression algorithm which is
// used for compressing sound data.
//
// The Compression Type Chunk is required and may appear only once in the Property
// Chunk.
struct compression_type_chunk_t
{
    // chunk_id is always 'CMPR'.
    uint32_t chunk_id;                    // 'CMPR'
    // chunk_data_size is the size of the data portion of the chunk, in bytes. It does not include the 12
    // bytes used by chunk_id and chunk_data_size.
    uint64_t chunk_data_size;

    // compression_type is used by applications to identify the compression algorithm, if any, used
    // on the sound data.
    uint32_t compression_type;                    // compression ID code

    // count the length in bytes of compression_name[].
    uint8_t  count;                    // length of the compression name

    // compression_name[] can be used by applications to display a message when the needed
    // decompression routine is not available.
    // Already defined compression types and names:
    // compression_type compression_name Meaning
    // 'DSD ' "not compressed" Uncompressed, plain DSD audio data
    // 'DST ' "DST Encoded" DST Encoded audio data
    // Other types may be defined in later versions.
    char compression_name[255];                        // human readable type name
} ATTRIBUTE_PACKED;
typedef struct compression_type_chunk_t   compression_type_chunk_t;
#define COMPRESSION_TYPE_CHUNK_SIZE    17U


// The Absolute Start Time Chunk defines the point on the time axis at which the sound data
// starts. The resolution for the Absolute Start Time is determined by the sample_rate which is
// defined in the Sample Rate Chunk.
//
// The Absolute Start Time Chunk is optional but if used it may appear only once in the
// Property Chunk.
struct absolute_start_time_chunk_t
{
    // chunk_id is always 'ABSS'.
    uint32_t chunk_id;            // 'ABSS'

    // chunk_data_size is the size of the data portion of the chunk, in bytes. It does not include the 12
    // bytes used by chunk_id and chunk_data_size.
    uint64_t chunk_data_size;

    // hours defines the hours on the time axis. This value is within the range [0..23].
    uint16_t hours;            // hours

    // minutes defines the minutes on the time axis. This value is within the range [0..59].
    uint8_t  minutes;            // minutes

    // seconds defines the seconds on the time axis. This value is within the range [0..59].
    uint8_t  seconds;            // seconds

    // samples defines the samples on the time axis. This value is within the range
    // [0..(sample_rate-1)]. sample_rate is defined in the Sample Rate Chunk.
    // If there is no Absolute Start Time Chunk, the start time is 00:00:00:00 [hh:mm:ss:samples].
    uint32_t samples;            // samples
} ATTRIBUTE_PACKED;
typedef struct absolute_start_time_chunk_t   absolute_start_time_chunk_t;
#define ABSOLUTE_START_TIME_CHUNK_SIZE    20U


// The Loudspeaker Configuration Chunk defines the set-up of the loudspeakers.
//
// The Loudspeaker Configuration Chunk is optional but if used it may appear only once in
// the Property Chunk.
struct loudspeaker_config_chunk_t
{
    // chunk_id is always 'LSCO'.
    uint32_t chunk_id;                    // 'LSCO'

    // chunk_data_size is the size of the data portion of the chunk, in bytes, which is always 2.
    uint64_t chunk_data_size;                    // 2

    // The loudspeaker configuration is defined as:
    // lsConfig Meaning
    // 0 2-channel stereo set-up
    // 1..2 Reserved for future use
    // 3 5-channel set-up according to ITU-R BS.775-1 [ITU]
    // 4 6-channel set-up, 5-channel set-up according to ITU-R BS.775-1 [ITU], plus
    // additional Low Frequency Enhancement (LFE) loudspeaker. Also known as
    // "5.1 configuration"
    // 5..65534 Reserved for future use
    // 65535 Undefined channel set-up.
    uint16_t loudspeaker_config;                    // loudspeaker configuration
} ATTRIBUTE_PACKED;
typedef struct loudspeaker_config_chunk_t   loudspeaker_config_chunk_t;
#define LOADSPEAKER_CONFIG_CHUNK_SIZE    14U


// The DSD Sound Data Chunk contains the non-compressed sound data.
//
// Either the DSD or DST Sound Data (described below) chunk is required and may appear
// only once in the Form DSD Chunk. The chunk must be placed after the Property Chunk.
struct dsd_sound_data_chunk_t
{
    // chunk_id is the same ID used as compression_type in the Compression Type Chunk in the
    // Property Chunk.
    uint32_t chunk_id;                    // 'DSD '

    // chunk_data_size is the size of the data portion of the chunk in bytes. It does not include the 12
    // bytes used by chunk_id and chunk_data_size. DSDsoundData[] contains the data that make up the
    // sound.
    uint64_t chunk_data_size;

    // DSDsoundData[] contains the data that make up the sound. If DSDsoundData[] contains
    // an odd number of bytes, a pad byte is added at the end (but not used for playback ).
    // DSD material consists of a sampled signal, where each sample is just one bit. Eight bits
    // (samples) of a channel are clustered together in a Channel Byte (most significant bit is the
    // oldest bit in time). For sound that consists of more than one channel, channel bytes are
    // interleaved in the order as identified in the Channels Chunk (3.2.2). See also section 5.6 of
    // [ScarletBook]. A set of interleaved channel bytes is called a Clustered Frame.
    //
    // Note that this implicates that the total number of bits for a channel in a Sound Data Chunk
    // is a multiple of 8 bits. Furthermore there is a restriction that all clustered frames are
    // channel_count bytes in length preserving the total number of bits per channel is equal for all
    // channels.
    uint8_t sound_data[];                     // (interleaved) DSD data
} ATTRIBUTE_PACKED;
typedef struct dsd_sound_data_chunk_t   dsd_sound_data_chunk_t;
#define DSD_SOUND_DATA_CHUNK_SIZE    12U


// The DST Sound Data Chunk contains the DST compressed sampled sound data. The DST
// Sound Data Chunk contains frames of DST encoded DSD data and the (optional) CRC over
// the original DSD data.
//
// Either the DSD or DST Sound Data Chunk is required and may appear only once in the
// Form DSD Chunk. The chunk must be placed after the Property Chunk
struct dst_sound_data_chunk_t
{
    // chunk_id is the same ID used as compression_type in the Compression Type Chunk in the
    // Property Chunk.
    uint32_t chunk_id;                                    // 'DST '

    // chunk_data_size is the size of the data portion of the chunk in bytes. It does not include the 12
    // bytes used by chunk_id and chunk_data_size.
    uint64_t chunk_data_size;

    // DstChunks[] contain the local chunks.
    // The local chunks are:
    // -  DST Frame Information Chunk
    // -  DST Frame Data Chunk
    // -  DST Frame CRC Chunk
    //
    // The DST Frame Data Chunks must be stored in their natural, chronological order.
    //
    // When the DST Frame CRC Chunks exist, the number o f DST Frame Data Chunks must be
    // the same as the number of DST Frame CRC Chunks. The DST Frame Data Chunks and the
    // DST Frame CRC Chunks will be interleaved, starting with the DST Frame Data Chunk.
    // The CRC, stored in a DST Frame CRC Chunk, must be calculated over the original (noncompressed)
    // DSD data corresponding to the audio of the preceding DST Frame Data
    // Chunk.
    chunk_header_t dst_chunks[];              // container
} ATTRIBUTE_PACKED;
typedef struct dst_sound_data_chunk_t   dst_sound_data_chunk_t;
#define DST_SOUND_DATA_CHUNK_SIZE    12U


// The DST Frame Information Chunk contains the actual number of DST frames and the
// number of DST frames per second (DST frame rate).
//
// The DST Frame Information Chunk is required if a DST Sound Data Chunk is used. The
// DST Frame Information Chunk must be the first chunk within the DST Sound Data Chunk.
// It may appear only once in a DST Sound Data Chunk.
struct dst_frame_information_chunk_t
{
    // chunk_id is always 'FRTE'.
    uint32_t chunk_id;                                    // 'FRTE'

    // chunk_data_size is the size of the data portion of the chunk in bytes. It does not include the 12
    // bytes used by chunk_id and chunk_data_size.
    uint64_t chunk_data_size;

    // num_frames is the number of DST frames (the number of chunks) in the file.
    uint32_t num_frames;            // number of DST frames.

    // frame_rate is the actual DST frame rate per second. The only valid value is 75.
    uint16_t frame_rate;            // DST frame rate per second
} ATTRIBUTE_PACKED;
typedef struct dst_frame_information_chunk_t   dst_frame_information_chunk_t;
#define DST_FRAME_INFORMATION_CHUNK_SIZE    18U


// The DST Frame Data Chunk contains the compressed sound data.
//
// The DST Frame Data Chunk is optional, and may appear more than once in the DST Sound
// Data Chunk.
struct dst_frame_data_chunk_t
{
    // chunk_id is always 'DSTF'.
    uint32_t chunk_id;                                    // 'DSTF'

    // chunk_data_size is the size of the data portion of the chunk in bytes. It does not include the 12
    // bytes used by chunk_id and chunk_data_size. DSTsoundData[] contains the data that make up the
    // sound. If the DSTsoundData[] contains an odd number of bytes, a pad byte must be added
    // at the end of the data to preserve an even length for this chunk. This pad byte is not
    // included in chunk_data_size and is not used for playback.
    uint64_t chunk_data_size;

    // DSTsoundData[] contains the data that make up the sound. DST material consists of
    // compressed DSD data (See [ScarletBook]).
    //uint8_t       DSTsoundData[1]; // The DST data for one frame
} ATTRIBUTE_PACKED;
typedef struct dst_frame_data_chunk_t   dst_frame_data_chunk_t;
#define DST_FRAME_DATA_CHUNK_SIZE    12U


// The DST Frame Data Chunk always precedes its corresponding DST Frame CRC Chunk.
//
// The DST Frame CRC Chunk is optional and if used exactly one chunk must be placed after
// each DST Frame Data Chunk.
struct dst_frame_crc_chunk_t
{
    // chunk_id is always 'DSTC'.
    uint32_t chunk_id;                                    // 'DSTC'

    // chunk_data_size is the size of the data portion of the chunk in bytes. It does not include the 12
    // bytes used by chunk_id and chunk_data_size. If the crcData[] contains an odd number of bytes, a
    // pad byte is added at the end of the data to preserve an even length for this chunk. This pad
    // byte is not included in chunk_data_size.
    uint64_t chunk_data_size;

    // crcData[] contains the data that make up the CRC value over the original (interleaved)
    // DSD data of the preceding DST Frame Data Chunk.
    // The specification of the CRC Algorithm :
    // A four-byte CRC is computed over the DSDsoundData[], per frame.
    //uint8_t       crcData[1]; // the value of the CRC
} ATTRIBUTE_PACKED;
typedef struct dst_frame_crc_chunk_t   dst_frame_crc_chunk_t;


struct dst_frame_index_t
{
    uint64_t offset;            // offset in the file [in bytes] of the sound in the DST Sound Data Chunk
    uint32_t length;            // length of the sound in bytes
} ATTRIBUTE_PACKED;
typedef struct dst_frame_index_t   dst_frame_index_t;
#define DST_FRAME_INDEX_SIZE    12U

// The DST Sound Index Chunk contains indexes to the DST Frame Data Chunks.
//
// The DST Sound Index Chunk is recommended, and may appear only when the compression
// type is 'DST '. Only one DST Sound Index Chunk is allowed in a Form DSD Chunk.
struct dst_sound_index_chunk_t
{
    // chunk_id is always 'DSTI'.
    uint32_t chunk_id;                    // 'DSTI'

    // chunk_data_size is the size of the data portion of the DST Sound Index Chunk in bytes. It does
    // not include the 12 bytes used by chunk_id and chunk_data_size. If the indexData[] contains an odd
    // number of bytes, a pad byte is added at the end to preserve an even length for this chunk.
    // This pad byte is not included in chunk_data_size.
    uint64_t chunk_data_size;

    // indexData[] contains the file positions of the DST Frame Data Chunks and their lengths. It
    // is an array of dst_frame_index_t structs.
    dst_frame_index_t index_data[255];           // array of index structs
} ATTRIBUTE_PACKED;
typedef struct dst_sound_index_chunk_t   dst_sound_index_chunk_t;
#define DST_SOUND_INDEX_CHUNK_SIZE    12U

// The format for describing each of the comments.
//
// Applications and or machines without a real time clock must use a time stamp according to
// yyyy-00-00 00:00, where yyyy is in the range of [0000-1999]. This will allow 2000
// comments to be created. The numbering of the comment should start at 0000. Each next
// comment should increase the year number by one.
struct comment_t
{
    // timestamp_year indicates the year of the comment creation. Values are in the range
    // [0..65535].
    uint16_t timestamp_year;                    // creation year

    // timestamp_month indicates the month of the comment creation. Values are in the range
    // [0..12].
    uint8_t timestamp_month;                     // creation month

    // timestamp_day indicates the day of the comment creation. Values are in the range [0..31].
    uint8_t timestamp_day;                     // creation day

    // timestamp_hour indicates the hour of the comment creation. Values are in the range [0..23].
    uint8_t timestamp_hour;                     // creation hour

    // timestamp_minutes indicates the minutes of the comment creation. Values are in the range
    // [0..59].
    uint8_t timestamp_minutes;                     // creation minutes

    // comment_type indicates the comment type. The following types are defined:
    // 0 General (album) comment
    // 1 Channel comment
    // 2 Sound Source
    // 3 File History
    // 4..65535 Reserved
    uint16_t comment_type;                    // comment type

    // comment_reference is the comment reference and indicates to which the comment refers.
    // If the comment type is General comment the comment reference must be 0.
    //
    // If the comment type is Channel comment, the comment reference defines the channel
    // number to which the comment belongs.
    // 0 all channels
    // 1 first channel in the file
    // 2 second channel in the file
    // .. ..
    // channel_count last channel of the file
    // The value of comment_reference is limited to [0...channel_count].
    //
    // If the comment type is Sound Source the comment reference is defined as:
    // 0 DSD recording
    // 1 Analogue recording
    // 2 PCM recording
    // 3.. 65535 Reserved
    // The Sound Source indicates the original storage format used during the live recording.
    //
    // If the sound type is PCM recording then it is recommended to describe (textually) the bit
    // length and the sample frequency in the comment_text[].
    // If the comment type is File History the comment reference can be one of:
    // 0 General Remark
    // 1 Name of the operator
    // 2 Name or type of the creating machine
    // 3 Time zone information
    // 4 Revision of the file
    // 5..65535 Reserved for future use
    //
    // The format for File History-Place or Zone is
    // "(GMT ±hh:mm)", if desired followed by a textual description. A space (0x20) is used
    // after GMT. When the File History-Place or Zone is omitted the indicating time is
    // Greenwich Mean Time "(GMT +00:00)".
    //
    // The format for File History-Revision is "(N)", where N is the revision number starting with
    // 1 for the first revision.
    uint16_t comment_reference;                    // comment reference

    // count is the length of the comment_text[] that makes up the comment.
    uint32_t count;                    // string length

    // comment_text[] is the description of the comment. This text must be padded with a byte at
    // the end, if needed, to make it an even number of bytes long. This pad byte, if present, is not
    // included in count.
    char comment_text[512];                        // text
} ATTRIBUTE_PACKED;
typedef struct comment_t   comment_t;
#define COMMENT_SIZE    14U


// The comments Chunk is used to store comments in DSDIFF.
//
// Remarks:
// -  The history of the file content can be tracked from the timestamp of each comment.
// -  When a time stamp with a timeStampYear earlier than 2000 occurs, the order of the
//    comments in the file designates the sequence of changes described by the comments.
// -  File history is useful for inquiry if a problem has occurred.
// -  comments describing the same action are linked together by means of equal time
//    stamps.
//
// The comment Chunk is optional but if used it may appear only once in the Form DSD
// Chunk.
struct comments_chunk_t
{
    // chunk_id is always 'COMT'.
    uint32_t chunk_id;                    // 'COMT'

    // chunk_data_size is the size of the data portion of the chunk, in bytes. It does not include the 12
    // bytes used by chunk_id and chunk_data_size.
    uint64_t chunk_data_size;

    // numcomments contains the number of comments in the comments Chunk.
    uint16_t numcomments;                    // number of comments

    // comments[] are the comments themselves. Each Comment consists of an even number of
    // bytes, so no pad bytes are needed within the comment chunks.
    comment_t comments[255];                   // the concatenated comments
} ATTRIBUTE_PACKED;
typedef struct comments_chunk_t   comments_chunk_t;
#define COMMENTS_CHUNK_SIZE    14U


// The Edited Master Information Chunk is a container chunk for storing edited master
// information.
//
// The Edited Master Information Chunk is optional but if used it may appear only once in the
// Form DSD Chunk.
struct edited_master_information_chunk_t
{
    // chunk_id is always 'DIIN'.
    uint32_t chunk_id;                            // 'DIIN'

    // chunk_data_size is the size of the data portion of the chunk, in bytes. It does not include the 12
    // bytes used by chunk_id and chunk_data_size. It is the total length of the local chunks.
    uint64_t chunk_data_size;

    // EmChunks[] contain local chunks.
    // The local chunks are:
    // -  Edited Master ID Chunk
    // -  Marker Chunk
    // -  Artist Chunk
    // -  Title Chunk
    //chunk_header_t            EmChunks[1]; // container
} ATTRIBUTE_PACKED;
typedef struct edited_master_information_chunk_t   edited_master_information_chunk_t;
#define EDITED_MASTER_INFORMATION_CHUNK_SIZE    12U


// The Edited Master ID Chunk stores an identifier.
//
// The Edited Master ID Chunk is optional but if used it may appear only once in the Edited
// Master Information Chunk.
struct edited_master_id_chunk_t
{
    // chunk_id is always 'EMID'.
    uint32_t chunk_id;                            // 'EMID'
    // chunk_data_size is the size of the data portion of the chunk, in bytes. It does not include the 12
    // bytes used by chunk_id and chunk_data_size.
    uint64_t chunk_data_size;
    // emid[] contains the identifier. The length and contents of the Edited Master identifier are
    // not specified, they are application specific . It is recommended that the emid[] is unique for
    // each Edited Master file. Therefore it is recommended to use date, time, machine name,
    // serial number, and so on, for an emid[].
    char emid[255];                                // unique sequence of bytes
} ATTRIBUTE_PACKED;
typedef struct edited_master_id_chunk_t   edited_master_id_chunk_t;
#define EDITED_MASTER_ID_CHUNK_SIZE 12U

// The Marker Chunk defines a marker within the sound data. It defines a type of marker, the
// position within the sound data and a description.
//
// The position of the marker is determined by hours, minutes, seconds and samples plus
// offset.
//
// Marker times must be regarded as absolute times.
//
// The Marker Chunk is optional and if used it may appear more than once in the Edited
// Master Information Chunk.
struct marker_chunk_t
{
    // chunk_id is always 'MARK'.
    uint32_t chunk_id;            // 'MARK'

    // chunk_data_size is the size of the data portion of the chunk, in bytes. It does not include the 12
    // bytes used by chunk_id and chunk_data_size.
    uint64_t chunk_data_size;

    // hours defines the hours on the time axis. This value is within the range [0..23].
    uint16_t hours;            // marker position in hours

    // minutes defines the minutes on the time axis. This value is within the range [0 ..59].
    uint8_t  minutes;            // marker position in minutes

    // seconds defines the seconds on the time axis. This value is within the range [0..59].
    uint8_t  seconds;            // marker position in seconds

    // samples defines the samples on the time axis. This value is within the range
    // [0 .. (sample_rate-1)]. The sample_rate is defined in the Sample Rate Chunk.
    uint32_t samples;            // marker position in samples

    // offset defines the offset of the marker in samples in range of [-231,231-1] with respect to
    // marker time. The offset can be used for modifying a marker position, without changing the
    // original position.
    int32_t offset;             // marker offset in samples

    // markType defines the type of marker. Currently the following types have been defined:
    // 0 TrackStart - Entry point for a Track start
    // 1 TrackStop - Entry point for ending a Track
    // 2 ProgramStart - Start point of 2-channel or multi-channel area
    // 3 Obsolete
    // 4 Index - Entry point of an Index
    // 5..65535 Reserved for future use
    // TrackStop of the last Track is also called ProgramEnd.
    uint16_t mark_type;            // type of marker

    // markChannel defines the channel to which this marker belongs.
    // 0 All channels
    // 1 First channel in the file
    // 2 Second channel in the file
    // .. ..
    // channel_count Last channel of the file
    // The value of markChannel is limited to [0...channel_count].
    uint16_t mark_channel;            // channel reference

    // TrackFlags define a series of flags to be used with a marker. The behaviour of the
    // TrackFlags is determined by the number and order of the channels defined in the file. Bit 0
    // is the least significant bit of TrackFlags.
    // Restrictions on the TrackFlags:
    // -  TrackFlags is only meaningful when markType is zero (TrackStart); if markType
    // is not zero TrackFlags must be zero
    // -  TrackFlags is only meaningful when markChannel is zero (All channels)
    // If a TrackFlag is set, the corresponding channel(s) must contain a Silence
    // Pattern [ScarletBook] during the whole Track, with two exceptions:
    // 1. it is not required to have a Silence Pattern at the start of the Track only if
    // the channel is available (i.e. the same TrackFlag is not set) within the
    // previous Track
    // 2. it is not required to have a Silence Pattern at the end of the Track only if
    // the channel is available (i.e. the same TrackFlag is not set) within the next
    // Track
    // Flags have been defined for files
    // -  with channel_count = 2 using the channel ID's :
    // 'SLFT', 'SRGT'. or
    // -  with channel_count = 5 using the channel ID's :
    // 'MLFT', 'MRGT', 'C ', 'LS ', 'RS '. or
    // -  with channel_count = 6 using the channel ID's :
    // 'MLFT', 'MRGT', 'C ', 'LFE ', 'LS ', 'RS '
    // Currently the following flags have been defined
    // Bit 0 TMF4_Mute - Indicates whether the 4th channel of this Track is muted
    // Bit 1 TMF1_Mute - Indicates whether the 1st and 2nd channel of this Track are muted
    // Bit 2 TMF2_Mute - Indicates whether:
    //          a) channel_count = 5, the 4th and 5th channel of this Track are muted
    //          b) channel_count = 6, the 5th and 6th channel of this Track are muted
    // Bit 3 TMF3_Mute - Indicates whether the 3rd channel of this Track is muted
    // Bit 4..7 Reserved
    // Bit 8..10 extra_use - Indicates whether the 4th channel is used for an LFE loudspeaker
    // Bit11..15 Reserved for future expansion
    // Restrictions on the TMFn_Mute flags (n=1..4):
    // -  Value one indicates muting, value zero indicates no-muting.
    // -  1st and 2nd channel are always available (TMF1_Mute is equal to zero)
    // -  For channel_count = 2, TMF1_Mute, TMF2_Mute, TMF3_Mute and TMF4_Mute
    // must be set to zero.
    // -  For channel_count = 5, TMF4_Mute must be set to zero.
    // -  For channel_count = 5, minimal one of TMF1_Mute, TMF2_Mute, TMF3_Mute
    // must be set to zero.
    // -  For channel_count = 5 or 6, minimal three channels are available.
    // -  For channel_count = 6, minimal one of TMF1_Mute, TMF2_Mute, TMF3_Mute,
    // TMF4_Mute must be set to zero.
    // -  For channel_count = 6 and TMF4_Mute is equal to zero, minimal four channels are
    // available.
    // Restrictions on the extra_use Flags:
    // -  For channel_count = 5, the three bit values must be zero, no LFE loudspeaker
    // available.
    // -  For numChannel = 6, the three bit values must be zero (Implicit usage of the LFE
    // loudspeaker).
    // Note: The previously defined LFE_mute flag is still compatible, because the LFE channel
    // is the 4th channel of a 5.1 multi-channel file.
    uint16_t track_flags;            // special purpose flags

    // count is the length of the markerText that makes up the description of the marker.
    uint32_t count;            // string length

    // markerText[] is the description of the marker. This text must be padded with a byte at the
    // end, if needed, to make it an even number of bytes long. This pad byte, if present, is not
    // included in count.
    char marker_text[8192];                // description
} ATTRIBUTE_PACKED;
typedef struct marker_chunk_t   marker_chunk_t;
#define EDITED_MASTER_MARKER_CHUNK_SIZE 34U


// The Artist Chunk defines the name of the Artist.
//
// The Artist Chunk is optional, but if it exists it may appear only once in the Edited Master
// Information Chunk.
struct artist_chunk_t
{
    // chunk_id is always 'DIAR'.
    uint32_t chunk_id;            // 'DIAR'

    // chunk_data_size is the size of the data portion of the chunk, in bytes. It does not include the 12
    // bytes used by chunk_id and chunk_data_size.
    uint64_t chunk_data_size;

    // count is the length of the artistText[] that makes up the name of the artist.
    uint32_t count;            // string length

    // artistText[] is the name of the Artist. This text must be padded with a byte at the end, if
    // needed, to make it an even number of bytes long. This pad byte, if present, is not included
    // in count.
    char artist_text[65535];                // description
} ATTRIBUTE_PACKED;
typedef struct artist_chunk_t   artist_chunk_t;
#define EDITED_MASTER_ARTIST_CHUNK_SIZE 16U

// The Title Chunk defines the title of the project in the file.
//
// The Title Chunk is optional, but if it exists it may appear only once in Edited Master
// Information Chunk.
struct title_chunk_t
{
    // chunk_id is always 'DITI'.
    uint32_t chunk_id;            // 'DITI'

    // chunk_data_size is the size of the data portion of the chunk, in bytes. It does not include the 12
    // bytes used by chunk_id and chunk_data_size.
    uint64_t chunk_data_size;

    // count is the length of the titleText[] that makes up the working title of the project.
    uint32_t count;            // string length

    // titleText[] is the name of the project in the file. This text must be padded with a byte at the
    // end, if needed, to make it an even number of bytes long. This pad byte, if present, is not
    // included in count.
    char title_text[65535];                // description
} ATTRIBUTE_PACKED;
typedef struct title_chunk_t   title_chunk_t;
#define EDITED_MASTER_TITLE_CHUNK_SIZE 16U

// The Manufacturer Specific Chunk is a chunk for storing manufacturer specific information.
//
// The Manufacturer Specific Chunk is optional, but if used it may appear only once in the
// Form DSD Chunk and it must be placed behind the Sound Data Chunk.
//
// It is recommended to maintain the chunk structure within the manData[]- field of the
// Manufacturer Specific Chunk according to the common chunk structure.
struct manufacturer_specific_chunk_t
{
    // chunk_id is always 'MANF'.
    uint32_t chunk_id;            // 'MANF'

    // chunk_data_size is the size of the data portion of the chunk, in bytes. It does not include the 12
    // bytes used by chunk_id and chunk_data_size. It is the total length of manID and manData[].
    uint64_t chunk_data_size;

    // manID contains the manufacturer identifier which must contain a unique ID.
    // A manufacturer who wants to use this chunk must request a unique manID from the
    // administrator of DSDIFF.
    uint32_t manufacturer_id;            // unique manufacturer ID [4 characters]

    // manData[] contains the manufacturer specific data. If manData[] contains an odd number
    // of bytes, a pad byte must be added at the end. The pad byte is not included in chunk_data_size.
    uint8_t manufacturer_data[1];             // manufacturer specific data
} ATTRIBUTE_PACKED;
typedef struct manufacturer_specific_chunk_t   manufacturer_specific_chunk_t;

#if PRAGMA_PACK
#pragma pack()
#endif

#endif  /* DSDIFF_H_INCLUDED */
