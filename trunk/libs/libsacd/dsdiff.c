/**
 * SACD Ripper - http://code.google.com/write_ptr/sacd-ripper/
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

#include "config.h"

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
#include "dsdiff.h"
#include "scarletbook.h"
#include "version.h"

#define DSDFIFF_BUFFER_SIZE    8192

typedef struct
{
    uint8_t            *header;
    size_t              header_size;
    uint8_t            *footer;
    size_t              footer_size;

    size_t              frame_count;
    ssize_t             audio_data_size;
} 
dsdiff_handle_t;

static char *get_mtoc_title_text(scarletbook_handle_t *handle)
{
    int          i;
    master_toc_t *master_toc = handle->master_toc;

    for (i = 0; i < master_toc->text_area_count; i++)
    {
        master_text_t *master_text = handle->master_text[i];

        if (master_text->album_title_position)
            return (char *) master_text + master_text->album_title_position;
        if (master_text->disc_title_position)
            return (char *) master_text + master_text->disc_title_position;
    }
    return 0;
}

int dsdiff_create_header(scarletbook_output_format_t *ft)
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
        if (ft->dst_encoded)
        {
            compression_type_chunk->compression_type = DST_MARKER;
            compression_type_chunk->count = 11;
            memcpy(compression_type_chunk->compression_name, "DST Encoded", 11);
        }
        else
        {
            compression_type_chunk->compression_type = DSD_MARKER;
            compression_type_chunk->count = 14;
            memcpy(compression_type_chunk->compression_name, "not compressed", 14);
        }

        compression_type_chunk->chunk_data_size = CALC_CHUNK_SIZE(COMPRESSION_TYPE_CHUNK_SIZE - CHUNK_HEADER_SIZE + compression_type_chunk->count);
        write_ptr += CEIL_ODD_NUMBER(COMPRESSION_TYPE_CHUNK_SIZE + compression_type_chunk->count);
    }

    // The Absolute Start Time Chunk is optional but if used it may appear only once in the
    // Property Chunk.
    {
        absolute_start_time_chunk_t *absolute_start_time_chunk = (absolute_start_time_chunk_t *) write_ptr;
        area_tracklist_time_start_t *area_tracklist_time_start = sb_handle->area[ft->area].area_tracklist_time->start;
        absolute_start_time_chunk->chunk_id = ABSS_MARKER;
        absolute_start_time_chunk->chunk_data_size = CALC_CHUNK_SIZE(ABSOLUTE_START_TIME_CHUNK_SIZE - CHUNK_HEADER_SIZE);
        absolute_start_time_chunk->hours = hton16(area_tracklist_time_start->minutes / 60);
        absolute_start_time_chunk->minutes = area_tracklist_time_start->minutes % 60;
        absolute_start_time_chunk->seconds = area_tracklist_time_start->seconds;
        absolute_start_time_chunk->samples = hton32(area_tracklist_time_start->frames);
        write_ptr += ABSOLUTE_START_TIME_CHUNK_SIZE;
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

    // we add a custom (unsupported) ID3 chunk to the PROP chunk to maintain all track information
    // within one file
    {
        chunk_header_t *id3_chunk;
        int            id3_chunk_size;
        id3_chunk                  = (chunk_header_t *) write_ptr;
        id3_chunk->chunk_id        = MAKE_MARKER('I', 'D', '3', ' ');
        id3_chunk_size             = scarletbook_id3_tag_render(sb_handle, write_ptr + CHUNK_HEADER_SIZE, ft->area, ft->track);
        id3_chunk->chunk_data_size = CALC_CHUNK_SIZE(id3_chunk_size);

        write_ptr += CEIL_ODD_NUMBER(CHUNK_HEADER_SIZE + id3_chunk_size);
    }

    // all properties have been written, now set the property chunk size
    property_chunk->chunk_data_size = CALC_CHUNK_SIZE(write_ptr - prop_ptr - CHUNK_HEADER_SIZE);

    // Either the DSD or DST Sound Data chunk is required and may appear
    // only once in the Form DSD Chunk. The chunk must be placed after the Property Chunk.
    if (ft->dst_encoded)
    {
        dst_sound_data_chunk_t *dst_sound_data_chunk;
        dst_frame_information_chunk_t *dst_frame_information_chunk;

        dst_sound_data_chunk                  = (dst_sound_data_chunk_t *) write_ptr;
        dst_sound_data_chunk->chunk_id        = DST_MARKER;
        dst_sound_data_chunk->chunk_data_size = CALC_CHUNK_SIZE(handle->audio_data_size + DST_FRAME_INFORMATION_CHUNK_SIZE);

        write_ptr += DST_SOUND_DATA_CHUNK_SIZE;

        dst_frame_information_chunk           = (dst_frame_information_chunk_t *) write_ptr;
        dst_frame_information_chunk->chunk_id = FRTE_MARKER;
        dst_frame_information_chunk->chunk_data_size = CALC_CHUNK_SIZE(DST_FRAME_INFORMATION_CHUNK_SIZE - CHUNK_HEADER_SIZE);
        dst_frame_information_chunk->frame_rate = hton16(75);
        dst_frame_information_chunk->num_frames = hton32(handle->frame_count);

        write_ptr += DST_FRAME_INFORMATION_CHUNK_SIZE;
    }
    else
    {
        dsd_sound_data_chunk_t * dsd_sound_data_chunk;
        dsd_sound_data_chunk                  = (dsd_sound_data_chunk_t *) write_ptr;
        dsd_sound_data_chunk->chunk_id        = DSD_MARKER;
        dsd_sound_data_chunk->chunk_data_size = CALC_CHUNK_SIZE(handle->audio_data_size);

        write_ptr += CHUNK_HEADER_SIZE;
    }

    // start with a new footer
    handle->footer_size = 0;

    // Audiogate supports, Title and Artist information through an EM Chunk
    {
        uint8_t * em_ptr  = handle->footer + handle->footer_size;
        edited_master_information_chunk_t *edited_master_information_chunk = (edited_master_information_chunk_t *) em_ptr;
        edited_master_information_chunk->chunk_id = DIIN_MARKER;
        em_ptr += EDITED_MASTER_INFORMATION_CHUNK_SIZE;

        {
            marker_chunk_t *marker_chunk = (marker_chunk_t *) em_ptr;
            area_tracklist_time_duration_t *area_tracklist_time_duration = sb_handle->area[ft->area].area_tracklist_time->duration;
            marker_chunk->chunk_id = MARK_MARKER;
            marker_chunk->chunk_data_size = CALC_CHUNK_SIZE(EDITED_MASTER_MARKER_CHUNK_SIZE - CHUNK_HEADER_SIZE);
            marker_chunk->hours = hton16(area_tracklist_time_duration->minutes / 60);
            marker_chunk->minutes = area_tracklist_time_duration->minutes % 60;
            marker_chunk->seconds = area_tracklist_time_duration->seconds;
            marker_chunk->samples = hton32(area_tracklist_time_duration->frames);
            marker_chunk->offset = 0;
            marker_chunk->mark_type = hton16(MARK_MARKER_TYPE_INDEX_ENTRY);
            marker_chunk->mark_channel = 0;
            marker_chunk->track_flags = 0;
            marker_chunk->count = 0;
            em_ptr += EDITED_MASTER_MARKER_CHUNK_SIZE;
        }

        {
            artist_chunk_t   *artist_chunk = (artist_chunk_t *) em_ptr;
            char *c;

            c = sb_handle->area[ft->area].area_track_text[ft->track].track_type_performer;
            if (c)
            {
                char track_artist[512];
                int len;
                memset(track_artist, 0, sizeof(track_artist));
                strncpy(track_artist, c, 511);

                len = strlen(track_artist);
                artist_chunk->chunk_id = DIAR_MARKER;
                artist_chunk->chunk_data_size = CALC_CHUNK_SIZE(EDITED_MASTER_ARTIST_CHUNK_SIZE + len - CHUNK_HEADER_SIZE);
                artist_chunk->count = hton32(len);
                em_ptr += EDITED_MASTER_ARTIST_CHUNK_SIZE;
                memcpy(em_ptr, track_artist, len);
                em_ptr += CEIL_ODD_NUMBER(len);
            }
        }

        {
            title_chunk_t   *title_chunk = (title_chunk_t *) em_ptr;
            char *c;

            c = sb_handle->area[ft->area].area_track_text[ft->track].track_type_title;
            if (c)
            {
                int len;
                char track_title[512];
                memset(track_title, 0, sizeof(track_title));
                strncpy(track_title, c, 511);

                len = strlen(track_title);

                title_chunk->chunk_id = DITI_MARKER;
                title_chunk->chunk_data_size = CALC_CHUNK_SIZE(EDITED_MASTER_TITLE_CHUNK_SIZE + len - CHUNK_HEADER_SIZE);
                title_chunk->count = hton32(len);
                em_ptr += EDITED_MASTER_TITLE_CHUNK_SIZE;
                memcpy(em_ptr, track_title, len);
                em_ptr += CEIL_ODD_NUMBER(len);
            }
        }

        edited_master_information_chunk->chunk_data_size = CALC_CHUNK_SIZE(em_ptr - handle->footer - handle->footer_size - CHUNK_HEADER_SIZE);
        handle->footer_size = CEIL_ODD_NUMBER(em_ptr - handle->footer);
    }

    // Now we write the COMT comment chunk to the footer buffer
    {
        time_t           rawtime;
        struct tm        * timeinfo;
        comment_t        * comment;
        char             data[512];
        uint8_t          * comment_ptr  = handle->footer + handle->footer_size;
        comments_chunk_t *comment_chunk = (comments_chunk_t *) comment_ptr;
        comment_chunk->chunk_id    = COMT_MARKER;
        comment_chunk->numcomments = hton16(2);

        comment_ptr += COMMENTS_CHUNK_SIZE;

        time(&rawtime);
        timeinfo = localtime(&rawtime);

        comment                    = (comment_t *) comment_ptr;
        comment->timestamp_year    = hton16(timeinfo->tm_year + 1900);
        comment->timestamp_month   = timeinfo->tm_mon;
        comment->timestamp_day     = timeinfo->tm_mday;
        comment->timestamp_hour    = timeinfo->tm_hour;
        comment->timestamp_minutes = timeinfo->tm_min;
        comment->comment_type      = hton16(COMT_TYPE_FILE_HISTORY);
        comment->comment_reference = hton16(COMT_TYPE_CHANNEL_FILE_HISTORY_GENERAL);
        sprintf(data, "Material ripped from SACD: %s", (char *) get_mtoc_title_text(sb_handle));
        comment->count = hton32(strlen(data));
        memcpy(comment->comment_text, data, strlen(data));

        comment_ptr += CEIL_ODD_NUMBER(COMMENT_SIZE + strlen(data));

        comment                    = (comment_t *) comment_ptr;
        comment->timestamp_year    = hton16(sb_handle->master_toc->disc_date_year);
        comment->timestamp_month   = sb_handle->master_toc->disc_date_month;
        comment->timestamp_day     = sb_handle->master_toc->disc_date_day;
        comment->timestamp_hour    = 0;
        comment->timestamp_minutes = 0;
        comment->comment_type      = hton16(COMT_TYPE_FILE_HISTORY);
        comment->comment_reference = hton16(COMT_TYPE_CHANNEL_FILE_HISTORY_CREATING_MACHINE);
        sprintf(data, SACD_RIPPER_VERSION);
        comment->count = hton32(strlen(data));
        memcpy(comment->comment_text, data, strlen(data));

        comment_ptr += CEIL_ODD_NUMBER(COMMENT_SIZE + strlen(data));

        handle->footer_size            = CEIL_ODD_NUMBER(comment_ptr - handle->footer);
        comment_chunk->chunk_data_size = CALC_CHUNK_SIZE(comment_ptr - handle->footer - handle->footer_size - COMMENTS_CHUNK_SIZE);
    }

    handle->header_size = CEIL_ODD_NUMBER(write_ptr - handle->header);
    form_dsd_chunk->chunk_data_size = CALC_CHUNK_SIZE(handle->header_size + handle->footer_size - COMMENTS_CHUNK_SIZE + handle->audio_data_size);

    fwrite(handle->header, 1, handle->header_size, ft->fd);

    return 0;
}


int dsdiff_create(scarletbook_output_format_t *ft)
{
    return dsdiff_create_header(ft);
}

int dsdiff_close(scarletbook_output_format_t *ft)
{
    dsdiff_handle_t *handle = (dsdiff_handle_t *) ft->priv;

    if (handle->audio_data_size % 2)
    {
        uint8_t dummy = 0;
        fwrite(&dummy, 1, 1, ft->fd);
        handle->audio_data_size += 1;
    }
    fwrite(handle->footer, 1, handle->footer_size, ft->fd);

    fseek(ft->fd, 0, SEEK_SET);
    
    // write the final header
    dsdiff_create_header(ft);

    if (handle->header)
        free(handle->header);
    if (handle->footer)
        free(handle->footer);

    return 0;
}

size_t dsdiff_write_frame(scarletbook_output_format_t *ft, const uint8_t *buf, size_t len, int last_frame)
{
    dsdiff_handle_t *handle = (dsdiff_handle_t *) ft->priv;

    handle->frame_count++;

    if (ft->dst_encoded)
    {
        dst_frame_data_chunk_t dst_frame_data_chunk;
        dst_frame_data_chunk.chunk_id = DSTF_MARKER;
        dst_frame_data_chunk.chunk_data_size = CALC_CHUNK_SIZE(len);
        {
            size_t nrw;
            nrw = fwrite(&dst_frame_data_chunk, 1, DST_FRAME_DATA_CHUNK_SIZE, ft->fd);
            nrw += fwrite(buf, 1, CEIL_ODD_NUMBER(len), ft->fd);
            handle->audio_data_size += nrw;
            return nrw;
        }
    }
    else
    {
        size_t nrw;
        nrw = fwrite(buf, 1, len, ft->fd);
        handle->audio_data_size += nrw;
        return nrw;
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
