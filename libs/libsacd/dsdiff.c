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

#include <charset.h>

#include "sacd_reader.h"
#include "scarletbook_id3.h"
#include "scarletbook_output.h"
#include "endianess.h"
#include "dsdiff.h"
#include "scarletbook.h"
#include "version.h"

#define DSDFIFF_BUFFER_SIZE    1024 * 16

typedef struct
{
    uint8_t            *header;
    size_t              header_size;
    uint8_t            *footer;
    size_t              footer_size;

    size_t              frame_count;
    uint64_t            audio_data_size;

    dst_frame_index_t  *frame_indexes;
    size_t              frame_indexes_allocated;

    int                 edit_master;
} 
dsdiff_handle_t;

static char *get_mtoc_title_text(scarletbook_handle_t *handle)
{
    master_text_t *master_text = &handle->master_text;

    if (master_text->album_title)
        return master_text->album_title;
    if (master_text->disc_title)
        return master_text->disc_title;

    return "Unknown";
}

static uint8_t *add_marker_chunk(uint8_t *em_ptr, scarletbook_output_format_t *ft, uint64_t frame_count, uint16_t mark_type, uint16_t track_flags)
{
    marker_chunk_t *marker_chunk = (marker_chunk_t *) em_ptr;
    int seconds = (int) (frame_count / SACD_FRAME_RATE);
    int remainder = seconds % 3600;

    marker_chunk->chunk_id = MARK_MARKER;
    marker_chunk->offset = 0;
    marker_chunk->hours = hton16(seconds / 3600);
    marker_chunk->minutes = remainder / 60;
    marker_chunk->seconds = remainder % 60;
    marker_chunk->samples = hton32((frame_count % SACD_FRAME_RATE) * SAMPLES_PER_FRAME * 64);
    marker_chunk->mark_type = hton16(mark_type);
    marker_chunk->mark_channel = hton16(COMT_TYPE_CHANNEL_ALL);
    marker_chunk->track_flags = hton16(track_flags);
    marker_chunk->count = 0;

    marker_chunk->chunk_data_size = CALC_CHUNK_SIZE(EDITED_MASTER_MARKER_CHUNK_SIZE - CHUNK_HEADER_SIZE);
    em_ptr += CEIL_ODD_NUMBER(EDITED_MASTER_MARKER_CHUNK_SIZE);

    return em_ptr;
}

static int calculate_header_and_footer(scarletbook_output_format_t *ft)
{
    form_dsd_chunk_t *form_dsd_chunk;
    property_chunk_t *property_chunk;
    uint8_t          *write_ptr, *prop_ptr;
    scarletbook_handle_t *sb_handle = ft->sb_handle;
    dsdiff_handle_t  *handle = (dsdiff_handle_t *) ft->priv;

    if (!handle->header)
        handle->header = (uint8_t *) calloc(DSDFIFF_BUFFER_SIZE, 1);
    if (!handle->footer)
        handle->footer = (uint8_t *) calloc(DSDFIFF_BUFFER_SIZE, 1);

    write_ptr = handle->header;

    // The Form DSD Chunk is required. It may appear only once in the file.
    form_dsd_chunk            = (form_dsd_chunk_t *) handle->header;
    form_dsd_chunk->chunk_id  = FRM8_MARKER;
    form_dsd_chunk->form_type = DSD_MARKER;
    write_ptr                 = (uint8_t *) handle->header + FORM_DSD_CHUNK_SIZE;

    // The Format Version Chunk is required and must be the first chunk in the Form DSD
    // Chunk. It may appear only once in the Form DSD Chunk.
    {
        format_version_chunk_t *format_version_chunk = (format_version_chunk_t *) write_ptr;
        format_version_chunk->chunk_id        = FVER_MARKER;
        format_version_chunk->chunk_data_size = CALC_CHUNK_SIZE(FORMAT_VERSION_CHUNK_SIZE - CHUNK_HEADER_SIZE);
        format_version_chunk->version         = hton32(DSDIFF_VERSION);
        write_ptr                            += FORMAT_VERSION_CHUNK_SIZE;
    }

    // The Property Chunk is required and must precede the Sound Data Chunk. It may appear
    // only once in the Form DSD Chunk.
    {
        prop_ptr                      = write_ptr;
        property_chunk                = (property_chunk_t *) write_ptr;
        property_chunk->chunk_id      = PROP_MARKER;
        property_chunk->property_type = SND_MARKER;
        write_ptr                    += PROPERTY_CHUNK_SIZE;
    }

    // The Sample Rate Chunk is required and may appear only once in the Property Chunk.
    {
        sample_rate_chunk_t *sample_rate_chunk = (sample_rate_chunk_t *) write_ptr;
        sample_rate_chunk->chunk_id        = FS_MARKER;
        sample_rate_chunk->chunk_data_size = CALC_CHUNK_SIZE(SAMPLE_RATE_CHUNK_SIZE - CHUNK_HEADER_SIZE);
        sample_rate_chunk->sample_rate     = hton32(SACD_SAMPLING_FREQUENCY);
        write_ptr                         += SAMPLE_RATE_CHUNK_SIZE;
    }

    // The Channels Chunk is required and may appear only once in the Property Chunk.
    {
        int              i;
        uint8_t          channel_count    = sb_handle->area[ft->area].area_toc->channel_count;
        channels_chunk_t * channels_chunk = (channels_chunk_t *) write_ptr;
        channels_chunk->chunk_id        = CHNL_MARKER;
        channels_chunk->chunk_data_size = CALC_CHUNK_SIZE(CHANNELS_CHUNK_SIZE - CHUNK_HEADER_SIZE + channel_count * sizeof(uint32_t));
        channels_chunk->channel_count   = hton16(channel_count);

        switch (channel_count)
        {
        case 2:
            channels_chunk->channel_ids[0] = SLFT_MARKER;
            channels_chunk->channel_ids[1] = SRGT_MARKER;
            break;
        case 5:
            channels_chunk->channel_ids[0] = MLFT_MARKER;
            channels_chunk->channel_ids[1] = MRGT_MARKER;
            channels_chunk->channel_ids[2] = C_MARKER;
            channels_chunk->channel_ids[3] = LS_MARKER;
            channels_chunk->channel_ids[4] = RS_MARKER;
            break;
        case 6:
            channels_chunk->channel_ids[0] = MLFT_MARKER;
            channels_chunk->channel_ids[1] = MRGT_MARKER;
            channels_chunk->channel_ids[2] = C_MARKER;
            channels_chunk->channel_ids[3] = LFE_MARKER;
            channels_chunk->channel_ids[4] = LS_MARKER;
            channels_chunk->channel_ids[5] = RS_MARKER;
            break;
        default:
            for (i = 0; i < channel_count; i++)
            {
                sprintf((char *) &channels_chunk->channel_ids[i], "C%03i", i);
            }
            break;
        }

        write_ptr += CHANNELS_CHUNK_SIZE + sizeof(uint32_t) * channel_count;
    }

    // The Compression Type Chunk is required and may appear only once in the Property
    // Chunk.
    {
        compression_type_chunk_t *compression_type_chunk = (compression_type_chunk_t *) write_ptr;
        compression_type_chunk->chunk_id         = CMPR_MARKER;
        if (ft->dsd_encoded_export)
        {
            compression_type_chunk->compression_type = DSD_MARKER;
            compression_type_chunk->count = 14;
            memcpy(compression_type_chunk->compression_name, "not compressed", 14);
        }
        else
        {
            compression_type_chunk->compression_type = DST_MARKER;
            compression_type_chunk->count = 11;
            memcpy(compression_type_chunk->compression_name, "DST Encoded", 11);
        }

        compression_type_chunk->chunk_data_size = CALC_CHUNK_SIZE(COMPRESSION_TYPE_CHUNK_SIZE - CHUNK_HEADER_SIZE + compression_type_chunk->count);
        write_ptr += CEIL_ODD_NUMBER(COMPRESSION_TYPE_CHUNK_SIZE + compression_type_chunk->count);
    }

    // The Loudspeaker Configuration Chunk is optional but if used it may appear only once in
    // the Property Chunk.
    {
        uint8_t channel_count = sb_handle->area[ft->area].area_toc->channel_count;
        loudspeaker_config_chunk_t *loudspeaker_config_chunk = (loudspeaker_config_chunk_t *) write_ptr;
        loudspeaker_config_chunk->chunk_id        = LSCO_MARKER;
        loudspeaker_config_chunk->chunk_data_size = CALC_CHUNK_SIZE(LOADSPEAKER_CONFIG_CHUNK_SIZE - CHUNK_HEADER_SIZE);

        switch (channel_count)
        {
        case 2:
            loudspeaker_config_chunk->loudspeaker_config = hton16(LS_CONFIG_2_CHNL);
            break;
        case 5:
            loudspeaker_config_chunk->loudspeaker_config = hton16(LS_CONFIG_5_CHNL);
            break;
        case 6:
            loudspeaker_config_chunk->loudspeaker_config = hton16(LS_CONFIG_6_CHNL);
            break;
        default:
            loudspeaker_config_chunk->loudspeaker_config = hton16(LS_CONFIG_UNDEFINED);
            break;
        }

        write_ptr += LOADSPEAKER_CONFIG_CHUNK_SIZE;
    }

    // all properties have been written, now set the property chunk size
    property_chunk->chunk_data_size = CALC_CHUNK_SIZE(write_ptr - prop_ptr - CHUNK_HEADER_SIZE);

    // Either the DSD or DST Sound Data chunk is required and may appear
    // only once in the Form DSD Chunk. The chunk must be placed after the Property Chunk.
    if (ft->dsd_encoded_export)
    {
        dsd_sound_data_chunk_t * dsd_sound_data_chunk;
        dsd_sound_data_chunk                  = (dsd_sound_data_chunk_t *) write_ptr;
        dsd_sound_data_chunk->chunk_id        = DSD_MARKER;
        dsd_sound_data_chunk->chunk_data_size = CALC_CHUNK_SIZE(handle->audio_data_size);

        write_ptr += CHUNK_HEADER_SIZE;
    }
    else
    {
        dst_sound_data_chunk_t *dst_sound_data_chunk;
        dst_frame_information_chunk_t *dst_frame_information_chunk;

        dst_sound_data_chunk                  = (dst_sound_data_chunk_t *) write_ptr;
        dst_sound_data_chunk->chunk_id        = DST_MARKER;

        write_ptr += DST_SOUND_DATA_CHUNK_SIZE;

        dst_frame_information_chunk           = (dst_frame_information_chunk_t *) write_ptr;
        dst_frame_information_chunk->chunk_id = FRTE_MARKER;
        dst_frame_information_chunk->frame_rate = hton16(SACD_FRAME_RATE);
        dst_frame_information_chunk->num_frames = hton32(handle->frame_count);
        dst_frame_information_chunk->chunk_data_size = CALC_CHUNK_SIZE(DST_FRAME_INFORMATION_CHUNK_SIZE - CHUNK_HEADER_SIZE);

        dst_sound_data_chunk->chunk_data_size = CALC_CHUNK_SIZE(handle->audio_data_size + DST_FRAME_INFORMATION_CHUNK_SIZE);
        write_ptr += DST_FRAME_INFORMATION_CHUNK_SIZE;
    }

    // start with a new footer
    handle->footer_size = 0;

    // DST Sound Index Chunk
    if (!ft->dsd_encoded_export && handle->frame_count > 0)
    {
        size_t frame;
        dst_sound_index_chunk_t *dst_sound_index_chunk;
        uint8_t *dsti_ptr;

        // resize the footer buffer
        handle->footer = realloc(handle->footer, DSDFIFF_BUFFER_SIZE + handle->frame_indexes_allocated * DST_FRAME_INDEX_SIZE);

        dsti_ptr = handle->footer + handle->footer_size;

        dst_sound_index_chunk                 = (dst_sound_index_chunk_t *) dsti_ptr;
        dst_sound_index_chunk->chunk_id       = DSTI_MARKER;

        dsti_ptr += DST_SOUND_INDEX_CHUNK_SIZE;

        for (frame = 0; frame < handle->frame_count; frame++)
        {
            dst_frame_index_t *dst_frame_index = (dst_frame_index_t *) dsti_ptr;
            dst_frame_index->length = hton32(handle->frame_indexes[frame].length);
            dst_frame_index->offset = hton64(handle->frame_indexes[frame].offset);
            dsti_ptr += DST_FRAME_INDEX_SIZE;
        }

        dst_sound_index_chunk->chunk_data_size = CALC_CHUNK_SIZE(handle->frame_count * DST_FRAME_INDEX_SIZE + DST_SOUND_INDEX_CHUNK_SIZE - CHUNK_HEADER_SIZE);
        handle->footer_size += CEIL_ODD_NUMBER(dsti_ptr - handle->footer - handle->footer_size);
    }

    // edit master information
    {
        uint8_t * em_ptr  = handle->footer + handle->footer_size;
        edited_master_information_chunk_t *edited_master_information_chunk = (edited_master_information_chunk_t *) em_ptr;
        edited_master_information_chunk->chunk_id = DIIN_MARKER;
        em_ptr += EDITED_MASTER_INFORMATION_CHUNK_SIZE;

        if (handle->edit_master)
        {
            int track;
            uint64_t abs_frames_start = 0, abs_frames_stop = 0;

            {
                // id is optional, but SADiE seems to require it
                edited_master_id_chunk_t *emid_chunk = (edited_master_id_chunk_t *) em_ptr;
                emid_chunk->chunk_id = EMID_MARKER;
                emid_chunk->chunk_data_size = CALC_CHUNK_SIZE(EDITED_MASTER_ID_CHUNK_SIZE - CHUNK_HEADER_SIZE);
                //strcpy(emid_chunk->emid, guid()); // TODO?, add guid functionality
                em_ptr += EDITED_MASTER_ID_CHUNK_SIZE;
            }

            for (track = 0; track < sb_handle->area[ft->area].area_toc->track_count; track++)
            {
                area_tracklist_time_t *time;
                uint16_t track_flags_stop, track_flags_start;

                time = &sb_handle->area[ft->area].area_tracklist_time->start[track];
                abs_frames_start = TIME_FRAMECOUNT(time);
                track_flags_start = time->track_flags_tmf4 << 0 
                    | time->track_flags_tmf1 << 1
                    | time->track_flags_tmf2 << 2
                    | time->track_flags_tmf3 << 3;

                time = &sb_handle->area[ft->area].area_tracklist_time->duration[track];
                abs_frames_stop = abs_frames_start + TIME_FRAMECOUNT(time);
                track_flags_stop = time->track_flags_tmf4 << 0 
                    | time->track_flags_tmf1 << 1
                    | time->track_flags_tmf2 << 2
                    | time->track_flags_tmf3 << 3;

                if (track == 0)
                {
                    // setting the programstart to 0 always seems incorrect, but produces correct results for SADiE
                    em_ptr = add_marker_chunk(em_ptr, ft, 0, MARK_MARKER_TYPE_PROGRAMSTART, track_flags_start);
                }

                em_ptr = add_marker_chunk(em_ptr, ft, abs_frames_start, MARK_MARKER_TYPE_TRACKSTART, track_flags_start);

                if (track == sb_handle->area[ft->area].area_toc->track_count - 1 
                 || (uint64_t) TIME_FRAMECOUNT(&sb_handle->area[ft->area].area_tracklist_time->start[track + 1]) > abs_frames_stop)
                {
                    em_ptr = add_marker_chunk(em_ptr, ft, abs_frames_stop, MARK_MARKER_TYPE_TRACKSTOP, track_flags_stop);
                }
            }
        }
        // Audiogate supports, Title and Artist information through an EM Chunk
        else
        {
            marker_chunk_t *marker_chunk = (marker_chunk_t *) em_ptr;
            area_tracklist_time_t *area_tracklist_time_duration = &sb_handle->area[ft->area].area_tracklist_time->duration[ft->track];
            marker_chunk->chunk_id = MARK_MARKER;
            marker_chunk->chunk_data_size = CALC_CHUNK_SIZE(EDITED_MASTER_MARKER_CHUNK_SIZE - CHUNK_HEADER_SIZE);
            marker_chunk->hours = hton16(area_tracklist_time_duration->minutes / 60);
            marker_chunk->minutes = area_tracklist_time_duration->minutes % 60;
            marker_chunk->seconds = area_tracklist_time_duration->seconds;
            marker_chunk->samples = hton32(area_tracklist_time_duration->frames * SAMPLES_PER_FRAME * 64);
            marker_chunk->offset = 0;
            marker_chunk->mark_type = hton16(MARK_MARKER_TYPE_INDEX_ENTRY);
            marker_chunk->mark_channel = hton16(COMT_TYPE_CHANNEL_ALL);
            marker_chunk->track_flags = 0;
            marker_chunk->count = 0;
            em_ptr += EDITED_MASTER_MARKER_CHUNK_SIZE;
        }

        {
            artist_chunk_t *artist_chunk = (artist_chunk_t *) em_ptr;
            char *c = 0;

            if (!handle->edit_master)
                c = sb_handle->area[ft->area].area_track_text[ft->track].track_type_performer;

            if (!c)
            {
                master_text_t *master_text = &sb_handle->master_text;

                if (master_text->album_artist)
                    c = master_text->album_artist;
                else if (master_text->album_artist_phonetic)
                    c = master_text->album_artist_phonetic;
                else if (master_text->disc_artist)
                    c = master_text->disc_artist;
                else if (master_text->disc_artist_phonetic)
                    c = master_text->disc_artist_phonetic;
                else if (master_text->album_title)
                    c = master_text->album_title; 
                else if (master_text->album_title_phonetic)
                    c = master_text->album_title_phonetic;
                else if (master_text->disc_title)
                    c = master_text->disc_title; 
                else if (master_text->disc_title_phonetic)
                    c = master_text->disc_title_phonetic;
            }

            if (c)
            {
                char *track_artist;
                int len;

                track_artist = charset_convert(c, strlen(c), "UTF-8", "ISO-8859-1");

                len = strlen(track_artist);
                artist_chunk->chunk_id = DIAR_MARKER;
                artist_chunk->chunk_data_size = CALC_CHUNK_SIZE(EDITED_MASTER_ARTIST_CHUNK_SIZE + len - CHUNK_HEADER_SIZE);
                artist_chunk->count = hton32(len);
                em_ptr += EDITED_MASTER_ARTIST_CHUNK_SIZE;
                memcpy(em_ptr, track_artist, len);
                em_ptr += CEIL_ODD_NUMBER(len);
                free(track_artist);
            }
        }

        {
            title_chunk_t   *title_chunk = (title_chunk_t *) em_ptr;
            char *c = 0;

            if (!handle->edit_master)
                c = sb_handle->area[ft->area].area_track_text[ft->track].track_type_title;

            if (!c)
            {
                master_text_t *master_text = &sb_handle->master_text;

                if (master_text->album_title)
                    c = master_text->album_title; 
                else if (master_text->album_title_phonetic)
                    c = master_text->album_title_phonetic;
                else if (master_text->disc_title)
                    c = master_text->disc_title; 
                else if (master_text->disc_title_phonetic)
                    c = master_text->disc_title_phonetic;
            }

            if (c)
            {
                int len;
                char *track_title;

                track_title = charset_convert(c, strlen(c), "UTF-8", "ISO-8859-1");
                len = strlen(track_title);

                title_chunk->chunk_id = DITI_MARKER;
                title_chunk->chunk_data_size = CALC_CHUNK_SIZE(EDITED_MASTER_TITLE_CHUNK_SIZE + len - CHUNK_HEADER_SIZE);
                title_chunk->count = hton32(len);
                em_ptr += EDITED_MASTER_TITLE_CHUNK_SIZE;
                memcpy(em_ptr, track_title, len);
                em_ptr += CEIL_ODD_NUMBER(len);
                free(track_title);
            }
        }

        edited_master_information_chunk->chunk_data_size = CALC_CHUNK_SIZE(em_ptr - handle->footer - handle->footer_size - CHUNK_HEADER_SIZE);
        handle->footer_size += CEIL_ODD_NUMBER(em_ptr - handle->footer - handle->footer_size);
    }

    // Now we write the COMT comment chunk to the footer buffer
    {
        time_t           rawtime;
        struct tm        * timeinfo;
        comment_t        * comment;
        char             data[512];
        char            *title;
        uint8_t          * comment_ptr  = handle->footer + handle->footer_size;
        comments_chunk_t *comment_chunk = (comments_chunk_t *) comment_ptr;
        comment_chunk->chunk_id    = COMT_MARKER;
        comment_chunk->numcomments = hton16(2);

        comment_ptr += COMMENTS_CHUNK_SIZE;

        time(&rawtime);
        timeinfo = localtime(&rawtime);

        comment                    = (comment_t *) comment_ptr;
        comment->timestamp_year    = hton16(sb_handle->master_toc->disc_date_year);
        comment->timestamp_month   = sb_handle->master_toc->disc_date_month;
        comment->timestamp_day     = sb_handle->master_toc->disc_date_day;
        comment->timestamp_hour    = 0;
        comment->timestamp_minutes = 0;
        comment->comment_type      = hton16(COMT_TYPE_FILE_HISTORY);
        comment->comment_reference = hton16(COMT_TYPE_CHANNEL_FILE_HISTORY_GENERAL);
        title = (char *) get_mtoc_title_text(sb_handle);
        title = charset_convert(title, strlen(title), "UTF-8", "ISO-8859-1");
        sprintf(data, "Material ripped from SACD: %s", title);
        free(title);
        comment->count = hton32(strlen(data));
        memcpy(comment->comment_text, data, strlen(data));

        comment_ptr += CEIL_ODD_NUMBER(COMMENT_SIZE + strlen(data));

        comment                    = (comment_t *) comment_ptr;
        comment->timestamp_year    = hton16(timeinfo->tm_year + 1900);
        comment->timestamp_month   = timeinfo->tm_mon;
        comment->timestamp_day     = timeinfo->tm_mday;
        comment->timestamp_hour    = timeinfo->tm_hour;
        comment->timestamp_minutes = timeinfo->tm_min;
        comment->comment_type      = hton16(COMT_TYPE_FILE_HISTORY);
        comment->comment_reference = hton16(COMT_TYPE_CHANNEL_FILE_HISTORY_CREATING_MACHINE);
        sprintf(data, SACD_RIPPER_VERSION_INFO);
        comment->count = hton32(strlen(data));
        memcpy(comment->comment_text, data, strlen(data));

        comment_ptr += CEIL_ODD_NUMBER(COMMENT_SIZE + strlen(data));

        comment_chunk->chunk_data_size = CALC_CHUNK_SIZE(comment_ptr - handle->footer - handle->footer_size - CHUNK_HEADER_SIZE);
        handle->footer_size           += CEIL_ODD_NUMBER(comment_ptr - handle->footer - handle->footer_size);
    }

    // we add a custom (unsupported) ID3 chunk to maintain all track information
    // within one file
    if (handle->edit_master)
    {
        int track;

        for (track = 0; track < sb_handle->area[ft->area].area_toc->track_count; track++)
        {
            chunk_header_t *id3_chunk;
            int            id3_chunk_size;
            uint8_t          * id3_ptr  = handle->footer + handle->footer_size;
            id3_chunk                  = (chunk_header_t *) id3_ptr;
            id3_chunk->chunk_id        = MAKE_MARKER('I', 'D', '3', ' ');
            id3_chunk_size             = scarletbook_id3_tag_render(sb_handle, id3_ptr + CHUNK_HEADER_SIZE, ft->area, track);
            id3_chunk->chunk_data_size = CALC_CHUNK_SIZE(id3_chunk_size);

            id3_ptr += CEIL_ODD_NUMBER(CHUNK_HEADER_SIZE + id3_chunk_size);

            handle->footer_size += CEIL_ODD_NUMBER(id3_ptr - handle->footer - handle->footer_size);
        }
    }
    else
    {
        chunk_header_t *id3_chunk;
        int            id3_chunk_size;
        uint8_t          * id3_ptr  = handle->footer + handle->footer_size;
        id3_chunk                  = (chunk_header_t *) id3_ptr;
        id3_chunk->chunk_id        = MAKE_MARKER('I', 'D', '3', ' ');
        id3_chunk_size             = scarletbook_id3_tag_render(sb_handle, id3_ptr + CHUNK_HEADER_SIZE, ft->area, ft->track);
        id3_chunk->chunk_data_size = CALC_CHUNK_SIZE(id3_chunk_size);

        id3_ptr += CEIL_ODD_NUMBER(CHUNK_HEADER_SIZE + id3_chunk_size);

        handle->footer_size += CEIL_ODD_NUMBER(id3_ptr - handle->footer - handle->footer_size);
    }

    handle->header_size = CEIL_ODD_NUMBER(write_ptr - handle->header);
    form_dsd_chunk->chunk_data_size = CALC_CHUNK_SIZE(handle->header_size + handle->footer_size + handle->audio_data_size - CHUNK_HEADER_SIZE);

    return 0;
}

static int dsdiff_create_edit_master(scarletbook_output_format_t *ft)
{
    int ret;
    dsdiff_handle_t *handle = (dsdiff_handle_t *) ft->priv;
    handle->edit_master = 1;
    ret = calculate_header_and_footer(ft);
    fwrite(handle->header, 1, handle->header_size, ft->fd);
    return ret;
}

static int dsdiff_create(scarletbook_output_format_t *ft)
{
    int ret = calculate_header_and_footer(ft);
    dsdiff_handle_t  *handle = (dsdiff_handle_t *) ft->priv;
    fwrite(handle->header, 1, handle->header_size, ft->fd); 
    return ret;
}

static int dsdiff_close(scarletbook_output_format_t *ft)
{
    dsdiff_handle_t *handle = (dsdiff_handle_t *) ft->priv;

    if (!handle)
        return 0;
    
    if (handle->audio_data_size % 2)
    {
        uint8_t dummy = 0;
        fwrite(&dummy, 1, 1, ft->fd);
        handle->audio_data_size += 1;
    }

    // re-calculate the header & footer
    calculate_header_and_footer(ft);

    // append the footer
    fwrite(handle->footer, 1, handle->footer_size, ft->fd);

    // write the final header
    fseek(ft->fd, 0, SEEK_SET);
    fwrite(handle->header, 1, handle->header_size, ft->fd);

    if (handle->frame_indexes)
        free(handle->frame_indexes);
    if (handle->header)
        free(handle->header);
    if (handle->footer)
        free(handle->footer);

    return 0;
}

static size_t dsdiff_write_frame(scarletbook_output_format_t *ft, const uint8_t *buf, size_t len)
{
    dsdiff_handle_t *handle = (dsdiff_handle_t *) ft->priv;

    handle->frame_count++;

    if (ft->dsd_encoded_export)
    {
        size_t nrw;
        nrw = fwrite(buf, 1, len, ft->fd);
        handle->audio_data_size += nrw;
        return nrw;
    }
    else
    {
        dst_frame_data_chunk_t dst_frame_data_chunk;
        dst_frame_data_chunk.chunk_id = DSTF_MARKER;
        dst_frame_data_chunk.chunk_data_size = hton64(len);
        {
            size_t nrw;

            if (handle->frame_count > handle->frame_indexes_allocated)
            {
                handle->frame_indexes_allocated += 10000;
                handle->frame_indexes = (dst_frame_index_t *) realloc(handle->frame_indexes, handle->frame_indexes_allocated * DST_FRAME_INDEX_SIZE);
            }

            handle->frame_indexes[handle->frame_count - 1].length = len;

#ifdef _WIN32
            handle->frame_indexes[handle->frame_count - 1].offset = _ftelli64(ft->fd) + DST_FRAME_DATA_CHUNK_SIZE;
#elif defined(__lv2ppu__) || defined(__APPLE__)
            handle->frame_indexes[handle->frame_count - 1].offset = ftello(ft->fd) + DST_FRAME_DATA_CHUNK_SIZE;
#else
            handle->frame_indexes[handle->frame_count - 1].offset = ftello64(ft->fd) + DST_FRAME_DATA_CHUNK_SIZE;
#endif

            nrw = fwrite(&dst_frame_data_chunk, 1, DST_FRAME_DATA_CHUNK_SIZE, ft->fd);
            nrw += fwrite(buf, 1, len, ft->fd);
            if (len % 2)
            {
                uint8_t dummy = 0;
                nrw += fwrite(&dummy, 1, 1, ft->fd);
            }
            handle->audio_data_size += nrw;
            return nrw;
        }
    }
}

scarletbook_format_handler_t const * dsdiff_format_fn(void) 
{
    static scarletbook_format_handler_t handler = 
    {
        "Direct Stream Digital Interchange File Format", 
        "dsdiff", 
        dsdiff_create, 
        dsdiff_write_frame,
        dsdiff_close, 
        OUTPUT_FLAG_DSD | OUTPUT_FLAG_DST,
        sizeof(dsdiff_handle_t)
    };
    return &handler;
}

scarletbook_format_handler_t const * dsdiff_edit_master_format_fn(void) 
{
    static scarletbook_format_handler_t handler = 
    {
        "Direct Stream Digital Interchange (Edit Master) File Format", 
        "dsdiff_edit_master", 
        dsdiff_create_edit_master, 
        dsdiff_write_frame,
        dsdiff_close, 
        OUTPUT_FLAG_DSD | OUTPUT_FLAG_DST | OUTPUT_FLAG_EDIT_MASTER,
        sizeof(dsdiff_handle_t)
    };
    return &handler;
}
