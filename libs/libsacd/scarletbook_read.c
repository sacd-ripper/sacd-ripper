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
#ifndef __APPLE__
#include <malloc.h>
#endif

#include <charset.h>

#include "endianess.h"
#include "scarletbook.h"
#include "scarletbook_read.h"
#include "scarletbook_helpers.h"
#include "sacd_reader.h"
#include "sacd_read_internal.h"

#ifndef NDEBUG
#define CHECK_ZERO0(arg)                                                       \
    if (arg != 0) {                                                            \
        fprintf(stderr, "*** Zero check failed in %s:%i\n    for %s = 0x%x\n", \
                __FILE__, __LINE__, # arg, arg);                               \
    }
#define CHECK_ZERO(arg)                                                    \
    if (memcmp(my_friendly_zeros, &arg, sizeof(arg))) {                    \
        unsigned int i_CZ;                                                 \
        fprintf(stderr, "*** Zero check failed in %s:%i\n    for %s = 0x", \
                __FILE__, __LINE__, # arg);                                \
        for (i_CZ = 0; i_CZ < sizeof(arg); i_CZ++)                         \
            fprintf(stderr, "%02x", *((uint8_t *) &arg + i_CZ));           \
        fprintf(stderr, "\n");                                             \
    }
static const uint8_t my_friendly_zeros[2048];
#else
#define CHECK_ZERO0(arg)    (void) (arg)
#define CHECK_ZERO(arg)     (void) (arg)
#endif

/* Prototypes for internal functions */
static int scarletbook_read_master_toc(scarletbook_handle_t *);
static int scarletbook_read_area_toc(scarletbook_handle_t *, int);

scarletbook_handle_t *scarletbook_open(sacd_reader_t *sacd, int title)
{
    scarletbook_handle_t *sb;

    sb = (scarletbook_handle_t *) calloc(sizeof(scarletbook_handle_t), 1);
    if (!sb)
        return NULL;

#ifdef __lv2ppu__
    sb->frame.data = (uint8_t *) memalign(128, MAX_DST_SIZE);
#else
    sb->frame.data = (uint8_t *) malloc(MAX_DST_SIZE);
#endif

    if (!sb->frame.data)
        return NULL;

    sb->sacd      = sacd;
    sb->twoch_area_idx = -1;
    sb->mulch_area_idx = -1;

    if (!scarletbook_read_master_toc(sb))
    {
        fprintf(stderr, "libsacdread: Can't read Master TOC.\n");
        scarletbook_close(sb);
        return NULL;
    }

    if (sb->master_toc->area_1_toc_1_start)
    {
        sb->area[sb->area_count].area_data = malloc(sb->master_toc->area_1_toc_size * SACD_LSN_SIZE);
        if (!sb->area[sb->area_count].area_data)
        {
            scarletbook_close(sb);
            return 0;
        }

        if (!sacd_read_block_raw(sacd, sb->master_toc->area_1_toc_1_start, sb->master_toc->area_1_toc_size, sb->area[sb->area_count].area_data))
        {
            sb->master_toc->area_1_toc_1_start = 0;
        }
        else
        {
            if (!scarletbook_read_area_toc(sb, sb->area_count))
            {
                fprintf(stderr, "libsacdread: Can't read Area TOC 1.\n");
            }
            else
                ++sb->area_count;
        }
    }
    if (sb->master_toc->area_2_toc_1_start)
    {
        sb->area[sb->area_count].area_data = malloc(sb->master_toc->area_2_toc_size * SACD_LSN_SIZE);
        if (!sb->area[sb->area_count].area_data)
        {
            scarletbook_close(sb);
            return 0;
        }

        if (!sacd_read_block_raw(sacd, sb->master_toc->area_2_toc_1_start, sb->master_toc->area_2_toc_size, sb->area[sb->area_count].area_data))
        {
            sb->master_toc->area_2_toc_1_start = 0;
            return sb;
        }

        if (!scarletbook_read_area_toc(sb, sb->area_count))
        {
            fprintf(stderr, "libsacdread: Can't read Area TOC 2.\n");
        }
        else
            ++sb->area_count;
    }


    return sb;
}

static void free_area(scarletbook_area_t *area)
{
    int i;
    
    for (i = 0; i < area->area_toc->track_count; i++)
    {
        free(area->area_track_text[i].track_type_title);
        free(area->area_track_text[i].track_type_performer);
        free(area->area_track_text[i].track_type_songwriter);
        free(area->area_track_text[i].track_type_composer);
        free(area->area_track_text[i].track_type_arranger);
        free(area->area_track_text[i].track_type_message);
        free(area->area_track_text[i].track_type_extra_message);
        free(area->area_track_text[i].track_type_title_phonetic);
        free(area->area_track_text[i].track_type_performer_phonetic);
        free(area->area_track_text[i].track_type_songwriter_phonetic);
        free(area->area_track_text[i].track_type_composer_phonetic);
        free(area->area_track_text[i].track_type_arranger_phonetic);
        free(area->area_track_text[i].track_type_message_phonetic);
        free(area->area_track_text[i].track_type_extra_message_phonetic);
    }

    free(area->description);
    free(area->copyright);
    free(area->description_phonetic);
    free(area->copyright_phonetic);
}

void scarletbook_close(scarletbook_handle_t *handle)
{
    if (!handle)
        return;

    if (has_two_channel(handle))
    {
        free_area(&handle->area[handle->twoch_area_idx]);
        free(handle->area[handle->twoch_area_idx].area_data);
    }

    if (has_multi_channel(handle))
    {
        free_area(&handle->area[handle->mulch_area_idx]);
        free(handle->area[handle->mulch_area_idx].area_data);
    }

    {
        master_text_t *mt = &handle->master_text;
        free(mt->album_title);
        free(mt->album_title_phonetic);
        free(mt->album_artist);
        free(mt->album_artist_phonetic);
        free(mt->album_publisher);
        free(mt->album_publisher_phonetic);
        free(mt->album_copyright);
        free(mt->album_copyright_phonetic);
        free(mt->disc_title);
        free(mt->disc_title_phonetic);
        free(mt->disc_artist);
        free(mt->disc_artist_phonetic);
        free(mt->disc_publisher);
        free(mt->disc_publisher_phonetic);
        free(mt->disc_copyright);
        free(mt->disc_copyright_phonetic);
    } 

    if (handle->master_data)
        free((void *) handle->master_data);

    if (handle->frame.data)
        free((void *) handle->frame.data);

    memset(handle, 0, sizeof(scarletbook_handle_t));

    free(handle);
    handle = 0;
}

static int scarletbook_read_master_toc(scarletbook_handle_t *handle)
{
    int          i;
    uint8_t      * p;
    master_toc_t *master_toc;

    handle->master_data = malloc(MASTER_TOC_LEN * SACD_LSN_SIZE);
    if (!handle->master_data)
        return 0;

    if (!sacd_read_block_raw(handle->sacd, START_OF_MASTER_TOC, MASTER_TOC_LEN, handle->master_data))
        return 0;

    master_toc = handle->master_toc = (master_toc_t *) handle->master_data;

    if (strncmp("SACDMTOC", master_toc->id, 8) != 0)
    {
        fprintf(stderr, "libsacdread: Not a ScarletBook disc!\n");
        return 0;
    }

    SWAP16(master_toc->album_set_size);
    SWAP16(master_toc->album_sequence_number);
    SWAP32(master_toc->area_1_toc_1_start);
    SWAP32(master_toc->area_1_toc_2_start);
    SWAP16(master_toc->area_1_toc_size);
    SWAP32(master_toc->area_2_toc_1_start);
    SWAP32(master_toc->area_2_toc_2_start);
    SWAP16(master_toc->area_2_toc_size);
    SWAP16(master_toc->disc_date_year);

    if (master_toc->version.major > SUPPORTED_VERSION_MAJOR || master_toc->version.minor > SUPPORTED_VERSION_MINOR)
    {
        fprintf(stderr, "libsacdread: Unsupported version: %i.%02i\n", master_toc->version.major, master_toc->version.minor);
        return 0;
    }

    CHECK_ZERO(master_toc->reserved01);
    CHECK_ZERO(master_toc->reserved02);
    CHECK_ZERO(master_toc->reserved03);
    CHECK_ZERO(master_toc->reserved04);
    CHECK_ZERO(master_toc->reserved05);
    CHECK_ZERO(master_toc->reserved06);
    for (i = 0; i < 4; i++)
    {
        CHECK_ZERO(master_toc->album_genre[i].reserved);
        CHECK_ZERO(master_toc->disc_genre[i].reserved);
        CHECK_VALUE(master_toc->album_genre[i].category <= MAX_CATEGORY_COUNT);
        CHECK_VALUE(master_toc->disc_genre[i].category <= MAX_CATEGORY_COUNT);
        CHECK_VALUE(master_toc->album_genre[i].genre <= MAX_GENRE_COUNT);
        CHECK_VALUE(master_toc->disc_genre[i].genre <= MAX_GENRE_COUNT);
    }

    CHECK_VALUE(master_toc->text_area_count <= MAX_LANGUAGE_COUNT);

    // point to eof master header
    p = handle->master_data + SACD_LSN_SIZE;

    // set pointers to text content
    for (i = 0; i < MAX_LANGUAGE_COUNT; i++)
    {
        master_sacd_text_t *master_text = (master_sacd_text_t *) p;

        if (strncmp("SACDText", master_text->id, 8) != 0)
        {
            return 0;
        }

        CHECK_ZERO(master_text->reserved);

        SWAP16(master_text->album_title_position);
        SWAP16(master_text->album_title_phonetic_position);
        SWAP16(master_text->album_artist_position);
        SWAP16(master_text->album_artist_phonetic_position);
        SWAP16(master_text->album_publisher_position);
        SWAP16(master_text->album_publisher_phonetic_position);
        SWAP16(master_text->album_copyright_position);
        SWAP16(master_text->album_copyright_phonetic_position);
        SWAP16(master_text->disc_title_position);
        SWAP16(master_text->disc_title_phonetic_position);
        SWAP16(master_text->disc_artist_position);
        SWAP16(master_text->disc_artist_phonetic_position);
        SWAP16(master_text->disc_publisher_position);
        SWAP16(master_text->disc_publisher_phonetic_position);
        SWAP16(master_text->disc_copyright_position);
        SWAP16(master_text->disc_copyright_phonetic_position);

        // we only use the first SACDText entry
        if (i == 0)
        {
            char *current_charset = (char *) character_set[handle->master_toc->locales[i].character_set & 0x07];

            if (master_text->album_title_position)
                handle->master_text.album_title = charset_convert((char *) master_text + master_text->album_title_position, strlen((char *) master_text + master_text->album_title_position), current_charset, "UTF-8");
            if (master_text->album_title_phonetic_position)
                handle->master_text.album_title_phonetic = charset_convert((char *) master_text + master_text->album_title_phonetic_position, strlen((char *) master_text + master_text->album_title_phonetic_position), current_charset, "UTF-8");
            if (master_text->album_artist_position)
                handle->master_text.album_artist = charset_convert((char *) master_text + master_text->album_artist_position, strlen((char *) master_text + master_text->album_artist_position), current_charset, "UTF-8");
            if (master_text->album_artist_phonetic_position)
                handle->master_text.album_artist_phonetic = charset_convert((char *) master_text + master_text->album_artist_phonetic_position, strlen((char *) master_text + master_text->album_artist_phonetic_position), current_charset, "UTF-8");
            if (master_text->album_publisher_position)
                handle->master_text.album_publisher = charset_convert((char *) master_text + master_text->album_publisher_position, strlen((char *) master_text + master_text->album_publisher_position), current_charset, "UTF-8");
            if (master_text->album_publisher_phonetic_position)
                handle->master_text.album_publisher_phonetic = charset_convert((char *) master_text + master_text->album_publisher_phonetic_position, strlen((char *) master_text + master_text->album_publisher_phonetic_position), current_charset, "UTF-8");
            if (master_text->album_copyright_position)
                handle->master_text.album_copyright = charset_convert((char *) master_text + master_text->album_copyright_position, strlen((char *) master_text + master_text->album_copyright_position), current_charset, "UTF-8");
            if (master_text->album_copyright_phonetic_position)
                handle->master_text.album_copyright_phonetic = charset_convert((char *) master_text + master_text->album_copyright_phonetic_position, strlen((char *) master_text + master_text->album_copyright_phonetic_position), current_charset, "UTF-8");

            if (master_text->disc_title_position)
                handle->master_text.disc_title = charset_convert((char *) master_text + master_text->disc_title_position, strlen((char *) master_text + master_text->disc_title_position), current_charset, "UTF-8");
            if (master_text->disc_title_phonetic_position)
                handle->master_text.disc_title_phonetic = charset_convert((char *) master_text + master_text->disc_title_phonetic_position, strlen((char *) master_text + master_text->disc_title_phonetic_position), current_charset, "UTF-8");
            if (master_text->disc_artist_position)
                handle->master_text.disc_artist = charset_convert((char *) master_text + master_text->disc_artist_position, strlen((char *) master_text + master_text->disc_artist_position), current_charset, "UTF-8");
            if (master_text->disc_artist_phonetic_position)
                handle->master_text.disc_artist_phonetic = charset_convert((char *) master_text + master_text->disc_artist_phonetic_position, strlen((char *) master_text + master_text->disc_artist_phonetic_position), current_charset, "UTF-8");
            if (master_text->disc_publisher_position)
                handle->master_text.disc_publisher = charset_convert((char *) master_text + master_text->disc_publisher_position, strlen((char *) master_text + master_text->disc_publisher_position), current_charset, "UTF-8");
            if (master_text->disc_publisher_phonetic_position)
                handle->master_text.disc_publisher_phonetic = charset_convert((char *) master_text + master_text->disc_publisher_phonetic_position, strlen((char *) master_text + master_text->disc_publisher_phonetic_position), current_charset, "UTF-8");
            if (master_text->disc_copyright_position)
                handle->master_text.disc_copyright = charset_convert((char *) master_text + master_text->disc_copyright_position, strlen((char *) master_text + master_text->disc_copyright_position), current_charset, "UTF-8");
            if (master_text->disc_copyright_phonetic_position)
                handle->master_text.disc_copyright_phonetic = charset_convert((char *) master_text + master_text->disc_copyright_phonetic_position, strlen((char *) master_text + master_text->disc_copyright_phonetic_position), current_charset, "UTF-8");
        }

        p += SACD_LSN_SIZE;
    }

    handle->master_man = (master_man_t *) p;
    if (strncmp("SACD_Man", handle->master_man->id, 8) != 0)
    {
        return 0;
    }

    return 1;
}

static int scarletbook_read_area_toc(scarletbook_handle_t *handle, int area_idx)
{
    int                 i, j;
    area_toc_t         *area_toc;
    uint8_t            *area_data;
    uint8_t            *p;
    int                 sacd_text_idx = 0;
    scarletbook_area_t *area = &handle->area[area_idx];
    char *current_charset;

    p = area_data = area->area_data;
    area_toc = area->area_toc = (area_toc_t *) area_data;

    if (strncmp("TWOCHTOC", area_toc->id, 8) != 0 && strncmp("MULCHTOC", area_toc->id, 8) != 0)
    {
        fprintf(stderr, "libsacdread: Not a valid Area TOC!\n");
        return 0;
    }

    SWAP16(area_toc->size);
    SWAP32(area_toc->track_start);
    SWAP32(area_toc->track_end);
    SWAP16(area_toc->area_description_offset);
    SWAP16(area_toc->copyright_offset);
    SWAP16(area_toc->area_description_phonetic_offset);
    SWAP16(area_toc->copyright_phonetic_offset);
    SWAP32(area_toc->max_byte_rate);
    SWAP16(area_toc->track_text_offset);
    SWAP16(area_toc->index_list_offset);
    SWAP16(area_toc->access_list_offset);

    CHECK_ZERO(area_toc->reserved01);
    CHECK_ZERO(area_toc->reserved03);
    CHECK_ZERO(area_toc->reserved04);
    CHECK_ZERO(area_toc->reserved06);
    CHECK_ZERO(area_toc->reserved07);
    CHECK_ZERO(area_toc->reserved08);
    CHECK_ZERO(area_toc->reserved09);
    CHECK_ZERO(area_toc->reserved10);

    current_charset = (char *) character_set[area->area_toc->languages[sacd_text_idx].character_set & 0x07];

    if (area_toc->copyright_offset)
        area->description_phonetic = charset_convert((char *) area_toc + area_toc->copyright_offset, strlen((char *) area_toc + area_toc->copyright_offset), current_charset, "UTF-8");
    if (area_toc->copyright_phonetic_offset)
        area->description_phonetic = charset_convert((char *) area_toc + area_toc->copyright_phonetic_offset, strlen((char *) area_toc + area_toc->copyright_phonetic_offset), current_charset, "UTF-8");
    if (area_toc->area_description_offset)
        area->description_phonetic = charset_convert((char *) area_toc + area_toc->area_description_offset, strlen((char *) area_toc + area_toc->area_description_offset), current_charset, "UTF-8");
    if (area_toc->area_description_phonetic_offset)
        area->description_phonetic = charset_convert((char *) area_toc + area_toc->area_description_phonetic_offset, strlen((char *) area_toc + area_toc->area_description_phonetic_offset), current_charset, "UTF-8");

    if (area_toc->version.major > SUPPORTED_VERSION_MAJOR || area_toc->version.minor > SUPPORTED_VERSION_MINOR)
    {
        fprintf(stderr, "libsacdread: Unsupported version: %2i.%2i\n", area_toc->version.major, area_toc->version.minor);
        return 0;
    }

    // is this the 2 channel?
    if (area_toc->channel_count == 2 && area_toc->loudspeaker_config == 0)
    {
        handle->twoch_area_idx = area_idx;
    }
    else
    {
        handle->mulch_area_idx = area_idx;
    }

    // Area TOC size is SACD_LSN_SIZE
    p += SACD_LSN_SIZE;

    while (p < (area_data + area_toc->size * SACD_LSN_SIZE))
    {
        if (strncmp((char *) p, "SACDTTxt", 8) == 0)
        {
            // we discard all other SACDTTxt entries
            if (sacd_text_idx == 0)
            {
                for (i = 0; i < area_toc->track_count; i++)
                {
                    area_text_t *area_text;
                    uint8_t        track_type, track_amount;
                    char           *track_ptr;
                    area_text = area->area_text = (area_text_t *) p;
                    SWAP16(area_text->track_text_position[i]);
                    if (area_text->track_text_position[i] > 0)
                    {
                        track_ptr = (char *) (p + area_text->track_text_position[i]);
                        track_amount = *track_ptr;
                        track_ptr += 4;
                        for (j = 0; j < track_amount; j++)
                        {
                            track_type = *track_ptr;
                            track_ptr++;
                            track_ptr++;                         // skip unknown 0x20
                            if (*track_ptr != 0)
                            {
                                switch (track_type)
                                {
                                case TRACK_TYPE_TITLE:
                                    area->area_track_text[i].track_type_title = charset_convert(track_ptr, strlen(track_ptr), current_charset, "UTF-8");
                                    break;
                                case TRACK_TYPE_PERFORMER:
                                    area->area_track_text[i].track_type_performer = charset_convert(track_ptr, strlen(track_ptr), current_charset, "UTF-8");
                                    break;
                                case TRACK_TYPE_SONGWRITER:
                                    area->area_track_text[i].track_type_songwriter = charset_convert(track_ptr, strlen(track_ptr), current_charset, "UTF-8");
                                    break;
                                case TRACK_TYPE_COMPOSER:
                                    area->area_track_text[i].track_type_composer = charset_convert(track_ptr, strlen(track_ptr), current_charset, "UTF-8");
                                    break;
                                case TRACK_TYPE_ARRANGER:
                                    area->area_track_text[i].track_type_arranger = charset_convert(track_ptr, strlen(track_ptr), current_charset, "UTF-8");
                                    break;
                                case TRACK_TYPE_MESSAGE:
                                    area->area_track_text[i].track_type_message = charset_convert(track_ptr, strlen(track_ptr), current_charset, "UTF-8");
                                    break;
                                case TRACK_TYPE_EXTRA_MESSAGE:
                                    area->area_track_text[i].track_type_extra_message = charset_convert(track_ptr, strlen(track_ptr), current_charset, "UTF-8");
                                    break;
                                case TRACK_TYPE_TITLE_PHONETIC:
                                    area->area_track_text[i].track_type_title_phonetic = charset_convert(track_ptr, strlen(track_ptr), current_charset, "UTF-8");
                                    break;
                                case TRACK_TYPE_PERFORMER_PHONETIC:
                                    area->area_track_text[i].track_type_performer_phonetic = charset_convert(track_ptr, strlen(track_ptr), current_charset, "UTF-8");
                                    break;
                                case TRACK_TYPE_SONGWRITER_PHONETIC:
                                    area->area_track_text[i].track_type_songwriter_phonetic = charset_convert(track_ptr, strlen(track_ptr), current_charset, "UTF-8");
                                    break;
                                case TRACK_TYPE_COMPOSER_PHONETIC:
                                    area->area_track_text[i].track_type_composer_phonetic = charset_convert(track_ptr, strlen(track_ptr), current_charset, "UTF-8");
                                    break;
                                case TRACK_TYPE_ARRANGER_PHONETIC:
                                    area->area_track_text[i].track_type_arranger_phonetic = charset_convert(track_ptr, strlen(track_ptr), current_charset, "UTF-8");
                                    break;
                                case TRACK_TYPE_MESSAGE_PHONETIC:
                                    area->area_track_text[i].track_type_message_phonetic = charset_convert(track_ptr, strlen(track_ptr), current_charset, "UTF-8");
                                    break;
                                case TRACK_TYPE_EXTRA_MESSAGE_PHONETIC:
                                    area->area_track_text[i].track_type_extra_message_phonetic = charset_convert(track_ptr, strlen(track_ptr), current_charset, "UTF-8");
                                    break;
                                }
                            }
                            if (j < track_amount - 1)
                            {
                                while (*track_ptr != 0)
                                    track_ptr++;

                                while (*track_ptr == 0)
                                    track_ptr++;
                            }
                        }
                    }
                }
            }
            sacd_text_idx++;
            p += SACD_LSN_SIZE;
        }
        else if (strncmp((char *) p, "SACD_IGL", 8) == 0)
        {
            area->area_isrc_genre = (area_isrc_genre_t *) p;
            p += SACD_LSN_SIZE * 2;
        }
        else if (strncmp((char *) p, "SACD_ACC", 8) == 0)
        {
            // skip
            p += SACD_LSN_SIZE * 32;
        }
        else if (strncmp((char *) p, "SACDTRL1", 8) == 0)
        {
            area_tracklist_offset_t *tracklist;
            tracklist = area->area_tracklist_offset = (area_tracklist_offset_t *) p;
            for (i = 0; i < area_toc->track_count; i++)
            {
                SWAP32(tracklist->track_start_lsn[i]);
                SWAP32(tracklist->track_length_lsn[i]);
            }
            p += SACD_LSN_SIZE;
        }
        else if (strncmp((char *) p, "SACDTRL2", 8) == 0)
        {
            area_tracklist_t *tracklist;
            tracklist = area->area_tracklist_time = (area_tracklist_t *) p;
            p += SACD_LSN_SIZE;
        }
        else
        {
            break;
        }
    }

    return 1;
}

void scarletbook_frame_init(scarletbook_handle_t *handle)
{
    handle->packet_info_idx = 0;
    handle->frame.size = 0;
    handle->frame.started = 0;
    memset(&handle->audio_sector, 0, sizeof(audio_sector_t));
}

static inline int get_channel_count(audio_frame_info_t *frame_info)
{
    if (frame_info->channel_bit_2 == 1 && frame_info->channel_bit_3 == 0)
    {
        return 6;
    }
    else if (frame_info->channel_bit_2 == 0 && frame_info->channel_bit_3 == 1)
    {
        return 5;
    }
    else
    {
        return 2;
    }
}

static inline void exec_read_callback(scarletbook_handle_t *handle, frame_read_callback_t frame_read_callback, void *userdata)
{
    if (handle->frame.started && handle->frame.size > 0 && 
        ((handle->frame.dst_encoded && handle->frame.sector_count == 0) ||
        (!handle->frame.dst_encoded && handle->frame.size % FRAME_SIZE_64 == 0))
        )
    {
        handle->frame.started = 0;
        frame_read_callback(handle, handle->frame.data, handle->frame.size, userdata);
    }
}

void scarletbook_process_frames(scarletbook_handle_t *handle, uint8_t *read_buffer, int blocks_read, int last_block, frame_read_callback_t frame_read_callback, void *userdata)
{
    int i, frame_info_counter;

    while(blocks_read--)
    {
        uint8_t *read_buffer_ptr = read_buffer;

        if (handle->packet_info_idx == handle->audio_sector.header.packet_info_count) 
        {
            handle->packet_info_idx = 0;

            memcpy(&handle->audio_sector.header, read_buffer_ptr, AUDIO_SECTOR_HEADER_SIZE);
            read_buffer_ptr += AUDIO_SECTOR_HEADER_SIZE;
#if defined(__BIG_ENDIAN__)
            memcpy(&handle->audio_sector.packet, read_buffer_ptr, AUDIO_PACKET_INFO_SIZE * handle->audio_sector.header.packet_info_count);
            read_buffer_ptr += AUDIO_PACKET_INFO_SIZE * handle->audio_sector.header.packet_info_count;
#else
            // Little Endian systems cannot properly deal with audio_packet_info_t
            {
                for (i = 0; i < handle->audio_sector.header.packet_info_count; i++)
                {
                    handle->audio_sector.packet[i].frame_start = (read_buffer_ptr[0] >> 7) & 1;
                    handle->audio_sector.packet[i].data_type = (read_buffer_ptr[0] >> 3) & 7;
                    handle->audio_sector.packet[i].packet_length = (read_buffer_ptr[0] & 7) << 8 | read_buffer_ptr[1];
                    read_buffer_ptr += AUDIO_PACKET_INFO_SIZE;
                }
            }
#endif
            if (handle->audio_sector.header.dst_encoded)
            {
                memcpy(&handle->audio_sector.frame, read_buffer_ptr, AUDIO_FRAME_INFO_SIZE * handle->audio_sector.header.frame_info_count);
                read_buffer_ptr += AUDIO_FRAME_INFO_SIZE * handle->audio_sector.header.frame_info_count;
            }
            else
            {
                for (i = 0; i < handle->audio_sector.header.frame_info_count; i++)
                {
                    memcpy(&handle->audio_sector.frame[i], read_buffer_ptr, AUDIO_FRAME_INFO_SIZE - 1);
                    read_buffer_ptr += AUDIO_FRAME_INFO_SIZE - 1;
                }
            }
        }

        frame_info_counter = 0;
        while (handle->packet_info_idx < handle->audio_sector.header.packet_info_count) 
        {
            audio_packet_info_t* packet = &handle->audio_sector.packet[handle->packet_info_idx];
            switch (packet->data_type) 
            {
            case DATA_TYPE_AUDIO:
                if (packet->frame_start)
                {
                    exec_read_callback(handle, frame_read_callback, userdata);

                    handle->frame.size = 0;
                    handle->frame.dst_encoded = handle->audio_sector.header.dst_encoded;
                    handle->frame.sector_count = handle->audio_sector.frame[frame_info_counter].sector_count;
                    handle->frame.channel_count = get_channel_count(&handle->audio_sector.frame[frame_info_counter]);
                    handle->frame.started = 1;

                    // advance frame_info_counter
                    frame_info_counter++;
                }
                if (handle->frame.started)
                {
                    if (handle->frame.size + packet->packet_length < MAX_DST_SIZE)
                    {
                        memcpy(handle->frame.data + handle->frame.size, read_buffer_ptr, packet->packet_length);
                        handle->frame.size += packet->packet_length;
                        if (handle->frame.dst_encoded)
                        {
                            handle->frame.sector_count--;
                        }
                    }
                    else
                    {
                        // buffer overflow error, try next frame..
                        handle->frame.started = 0;
                    }
                }
                break;
            case DATA_TYPE_SUPPLEMENTARY:
            case DATA_TYPE_PADDING:
                break;
            default:
                break;
            }
            // advance the source pointer
            read_buffer_ptr += packet->packet_length;

            handle->packet_info_idx++;
        }
        read_buffer += SACD_LSN_SIZE;
    }

    if (last_block) 
    {
        exec_read_callback(handle, frame_read_callback, userdata);
    }

}
