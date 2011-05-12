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
 
#ifndef SCARLETBOOK_TYPES_H_INCLUDED
#define SCARLETBOOK_TYPES_H_INCLUDED

#include <inttypes.h>

#undef ATTRIBUTE_PACKED
#undef PRAGMA_PACK_BEGIN
#undef PRAGMA_PACK_END

#if defined(__GNUC__)
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#define ATTRIBUTE_PACKED __attribute__ ((packed))
#define PRAGMA_PACK 0
#endif
#endif

#if !defined(ATTRIBUTE_PACKED)
#define ATTRIBUTE_PACKED
#define PRAGMA_PACK 1
#endif

/**
 * The length of one Logical Block of an SACD.
 */
#define SACD_LSN_SIZE               2048

#define SACD_SAMPLING_FREQUENCY     2822400

#define START_OF_FILE_SYSTEM_AREA   0
#define START_OF_MASTER_TOC         510
#define MASTER_TOC_LEN              10
#define MAX_AREA_TOC_SIZE_LSN       96
#define MAX_LANGUAGE_COUNT          8
#define SAMPLES_PER_FRAME           588
#define SUPPORT_SCARLETBOOK_VERSION 0x114

#define MAX_GENRE_COUNT             29
#define MAX_CATEGORY_COUNT          3

/**
 * Opaque type that is used as a handle for one instance of an opened SACD.
 */
typedef struct sacd_reader_s sacd_reader_t;

enum 
{
    ENCODING_DST              = 0
  , ENCODING_DSD_3_IN_14      = 2
  , ENCODING_DSD_3_IN_16      = 3

} encoding_t;

enum
{
    CHAR_SET_UNKNOWN          = 0
  , CHAR_SET_ISO646           = 1
  , CHAR_SET_ISO8859_1        = 2
  , CHAR_SET_RIS506           = 3
  , CHAR_SET_KSC5601          = 4
  , CHAR_SET_GB2312           = 5
  , CHAR_SET_BIG5             = 6
  , CHAR_SET_ISO8859_1_ESC    = 7

} character_set_t;

static const char *album_genre[] =
{
    "Not used"
  , "Not defined"
  , "Adult Contemporary"
  , "Alternative Rock"
  , "Children's Music"
  , "Classical"
  , "Contemporary Christian"
  , "Country"
  , "Dance"
  , "Easy Listening"
  , "Erotic"
  , "Folk"
  , "Gospel"
  , "Hip Hop"
  , "Jazz"
  , "Latin"
  , "Musical"
  , "New Age"
  , "Opera"
  , "Operetta"
  , "Pop Music"
  , "RAP"
  , "Reggae"
  , "Rock Music"
  , "Rhythm & Blues"
  , "Sound Effects"
  , "Sound Track"
  , "Spoken Word"
  , "World Music"
  , "Blues"
};

enum
{
    GENRE_NOT_USED                      = 0
  , GENRE_NOT_DEFINED                   = 1
  , GENRE_ADULT_CONTEMPORARY            = 2
  , GENRE_ALTERNATIVE_ROCK              = 3
  , GENRE_CHILDRENS_MUSIC               = 4
  , GENRE_CLASSICAL                     = 5
  , GENRE_CONTEMPORARY_CHRISTIAN        = 6
  , GENRE_COUNTRY                       = 7
  , GENRE_DANCE                         = 8
  , GENRE_EASY_LISTENING                = 9
  , GENRE_EROTIC                        = 10
  , GENRE_FOLK                          = 11
  , GENRE_GOSPEL                        = 12
  , GENRE_HIP_HOP                       = 13
  , GENRE_JAZZ                          = 14
  , GENRE_LATIN                         = 15
  , GENRE_MUSICAL                       = 16
  , GENRE_NEW_AGE                       = 17
  , GENRE_OPERA                         = 18
  , GENRE_OPERETTA                      = 19
  , GENRE_POP_MUSIC                     = 20
  , GENRE_RAP                           = 21
  , GENRE_REGGAE                        = 22
  , GENRE_ROCK_MUSIC                    = 23
  , GENRE_RHYTHM_AND_BLUES              = 24
  , GENRE_SOUND_EFFECTS                 = 25
  , GENRE_SOUND_TRACK                   = 26
  , GENRE_SPOKEN_WORD                   = 27
  , GENRE_WORLD_MUSIC                   = 28
  , GENRE_BLUES                         = 29

} genre_t;

enum
{
    CATEGORY_NOT_USED                   = 0
  , CATEGORY_GENERAL                    = 1
  , CATEGORY_JAPANESE                   = 2

} category_t;

static const char *album_category[] =
{
      "Not used"
    , "General"
    , "Japanese"
};

enum
{
    TRACK_TYPE_TITLE                    = 0x01
  , TRACK_TYPE_PERFORMER                = 0x02
  , TRACK_TYPE_SONGWRITER               = 0x03
  , TRACK_TYPE_COMPOSER                 = 0x04
  , TRACK_TYPE_ARRANGER                 = 0x05
  , TRACK_TYPE_MESSAGE                  = 0x06
  , TRACK_TYPE_EXTRA_MESSAGE            = 0x07

  , TRACK_TYPE_TITLE_PHONETIC           = 0x81
  , TRACK_TYPE_PERFORMER_PHONETIC       = 0x82
  , TRACK_TYPE_SONGWRITER_PHONETIC      = 0x83
  , TRACK_TYPE_COMPOSER_PHONETIC        = 0x84
  , TRACK_TYPE_ARRANGER_PHONETIC        = 0x85
  , TRACK_TYPE_MESSAGE_PHONETIC         = 0x86
  , TRACK_TYPE_EXTRA_MESSAGE_PHONETIC   = 0x87

} track_type_t;

#if PRAGMA_PACK
#pragma pack(1)
#endif

/**
 * Common
 *
 * The following structures are used in both the Master and Channel TOC.
 */

/**
 * Genre Information.
 */
typedef struct
{
    uint8_t        category;                  // category_t
    uint16_t       zero_01;
    uint8_t        genre;                     // genre_t
} ATTRIBUTE_PACKED genre_table_t;

/**
 * Language & character set
 */
typedef struct
{
    char           language_code[2];          // ISO639-2 Language code
    uint8_t        character_set;             // char_set_t
    uint8_t        zero_01;
} ATTRIBUTE_PACKED locale_table_t;

/**
 * Master TOC
 *
 * The following structures are needed for Master TOC information.
 */
typedef struct
{
    char           id[8];                     // SACDMTOC
    uint16_t       disc_version;              // 1.20 / 0x0114
    uint8_t        zero_01[6];
    uint16_t       album_set_size;
    uint16_t       album_sequence_number;
    uint8_t        zero_02[4];
    char           album_catalog_number[16];  // 0x00 when empty, else padded with spaces for short strings
    genre_table_t  album_genre[4];
    uint8_t        zero_03[8];
    uint32_t       channel_1_toc_area_1_start;
    uint32_t       channel_1_toc_area_2_start;
    uint32_t       channel_2_toc_area_1_start;
    uint32_t       channel_2_toc_area_2_start;
    uint8_t        disc_type;                 // 0x80 hybrid, 0x00 for single, 0x?? for dual?
    uint8_t        zero_04[3];
    uint16_t       channel_1_toc_area_size;
    uint16_t       channel_2_toc_area_size;
    char           disc_catalog_number[16];   // 0x00 when empty, else padded with spaces for short strings
    genre_table_t  disc_genre[4];
    uint16_t       disc_date_year;
    uint8_t        disc_date_month;
    uint8_t        disc_date_day;
    uint8_t        zero_05[4];                // time?
    uint8_t        text_channel_count;
    uint8_t        zero_06[7];
    locale_table_t locales[8];
} ATTRIBUTE_PACKED master_toc_t;

/**
 * Master Album Information
 */
typedef struct
{
    char           id[8];                     // SACDText
    uint64_t       zero_01;
    uint16_t       album_title_position;
    uint16_t       album_title_phonetic_position;
    uint16_t       album_artist_position;
    uint16_t       album_artist_phonetic_position;
    uint16_t       album_publisher_position;
    uint16_t       album_publisher_phonetic_position;
    uint16_t       album_copyright_position;
    uint16_t       album_copyright_phonetic_position;
    uint16_t       disc_title_position;
    uint16_t       disc_title_phonetic_position;
    uint16_t       disc_artist_position;
    uint16_t       disc_artist_phonetic_position;
    uint16_t       disc_publisher_position;
    uint16_t       disc_publisher_phonetic_position;
    uint16_t       disc_copyright_position;
    uint16_t       disc_copyright_phonetic_position;
    uint8_t        data[2000];
} ATTRIBUTE_PACKED master_text_t;

/**
 * Unknown Structure
 */
typedef struct
{
    char           id[8];                      // SACD_Man
    uint8_t        data[2040];
} ATTRIBUTE_PACKED master_man_t;

/**
 * Channel TOC
 *
 * The following structures are needed for Channel TOC information.
 * 
 */
typedef struct
{
    char           id[8];                     // TWOCHTOC or MULCHTOC
    uint16_t       version;                   // 1.20, 0x0114
    uint16_t       size;                      // ex. 40 (total size of TOC)
    uint8_t        zero_01[4];
	uint32_t       max_byte_rate;
	uint8_t        sample_frequency;		  // 0x04 = 2822400 (physically there can be no others values, or..? :)
	uint8_t        encoding;
	uint8_t        zero_02[10];
    uint8_t        channel_count;
	uint8_t        loudspeaker_config;
	uint8_t        unknown_01;
    uint8_t        zero_03[29];
    uint32_t       unknown_02;
    uint16_t       track_count;
    uint16_t       zero_04;
    uint32_t       track_position;
    uint32_t       track_length;
    uint8_t        text_channel_count;
    uint8_t        zero_05[7];
    locale_table_t languages[10];
	uint32_t       unknown_03;
	uint16_t       unknown_04;
	uint8_t        zero_06[10];
    uint16_t       area_description_offset;
    uint16_t       copyright_offset;
    uint16_t       area_description_phonetic_offset;
    uint16_t       copyright_phonetic_offset;
    uint8_t        data[1896];
} ATTRIBUTE_PACKED channel_toc_t;

typedef struct
{
    uint8_t        track_text_count;
    uint16_t       max_byte_rate;
    uint8_t        track_type;
    uint8_t        unknown_02;
    char           track_text[];
} ATTRIBUTE_PACKED track_text_t;

typedef struct
{
    char           id[8];                     // SACDText
    uint16_t       track_text_position[255];
    uint8_t        data[];
} ATTRIBUTE_PACKED channel_text_t;

typedef struct
{
    char           country_code[2];
    char           owner_code[3];
    char           recording_year[2];
    char           designation_code[5];
} ATTRIBUTE_PACKED isrc_t;

typedef struct
{
    char           id[8];                     // SACD_IGL
    isrc_t         isrc[170];
} ATTRIBUTE_PACKED channel_isrc_t;

typedef struct
{
    char           id[8];                     // SACD_ACC
    uint8_t        data[0xfff8];
} ATTRIBUTE_PACKED channel_index_t;

typedef struct
{
    char           id[8];                     // SACDTRL1
    uint32_t       track_pos_lsn[255];
    uint32_t       track_length_lsn[255];
} ATTRIBUTE_PACKED channel_tracklist_offset_t;

/**
 * SACD Time Information.
 */
typedef struct {
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t frame_u; /* The two high bits are the frame rate. */
} ATTRIBUTE_PACKED sacd_time_t;

typedef struct
{
    char           id[8];                     // SACDTRL2
    uint32_t       track_pos_abs[255];
    uint32_t       track_length_abs[255];
} ATTRIBUTE_PACKED channel_tracklist_abs_t;

#if PRAGMA_PACK
#pragma pack()
#endif

typedef struct {

  sacd_reader_t*				sacd;

  uint8_t *						master_data;
  master_toc_t *				master_toc;
  master_text_t *				master_text[8];
  master_man_t *				master_man;

  uint8_t *						channel_data[2];
  int							channel_count;
  channel_toc_t *				channel_toc[2];
  channel_tracklist_offset_t *	channel_tracklist_offset[2];
  channel_tracklist_abs_t *	channel_tracklist_time[2];
  channel_text_t *				channel_text[2][8];
  channel_isrc_t *				channel_isrc[2];

} scarletbook_handle_t;

#endif /* SCARLETBOOK_TYPES_H_INCLUDED */
