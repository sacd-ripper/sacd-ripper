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

#include <string.h>

#include <fileutils.h>
#include <utils.h>

#include "scarletbook_helpers.h"

char *get_album_dir(scarletbook_handle_t *handle)
{
    char disc_artist[60];
    char disc_album_title[60];
    char disc_album_year[5];
    char *albumdir;
    master_text_t *master_text = handle->master_text[0];
    int disc_artist_position = (master_text->disc_artist_position ? master_text->disc_artist_position : master_text->disc_artist_phonetic_position);
    int disc_album_title_position = (master_text->disc_title_position ? master_text->disc_title_position : master_text->disc_title_phonetic_position);

    memset(disc_artist, 0, sizeof(disc_artist));
    if (disc_artist_position)
    {
        char *c = (char *) master_text + disc_artist_position;
        char *pos = strchr(c, ';');
        if (!pos)
            pos = c + strlen(c);
        strncpy(disc_artist, c, min(pos - c, 59));
    }

    memset(disc_album_title, 0, sizeof(disc_album_title));
    if (disc_album_title_position)
    {
        char *c = (char *) master_text + disc_album_title_position;
        char *pos = strchr(c, ';');
        if (!pos)
            pos = c + strlen(c);
        strncpy(disc_album_title, c, min(pos - c, 59));
    }

    snprintf(disc_album_year, sizeof(disc_album_year), "%04d", handle->master_toc->disc_date_year);
    
    sanitize_filename(disc_artist);
    sanitize_filename(disc_album_title);

    if (strlen(disc_artist) > 0 && strlen(disc_album_title) > 0)
        albumdir = parse_format("%A - %L", 0, disc_album_year, disc_artist, disc_album_title, NULL);
    else if (strlen(disc_artist) > 0)
        albumdir = parse_format("%A", 0, disc_album_year, disc_artist, disc_album_title, NULL);
    else if (strlen(disc_album_title) > 0)
        albumdir = parse_format("%L", 0, disc_album_year, disc_artist, disc_album_title, NULL);
    else
        albumdir = parse_format("Unknown Album", 0, disc_album_year, disc_artist, disc_album_title, NULL);

    return albumdir;
}

char *get_music_filename(scarletbook_handle_t *handle, int area, int track)
{
    char *c;
    char track_artist[60];
    char track_title[60];
    char disc_album_title[60];
    char disc_album_year[5];
    master_text_t *master_text = handle->master_text[0];
    int disc_album_title_position = (master_text->disc_title_position ? master_text->disc_title_position : master_text->disc_title_phonetic_position);

    memset(track_artist, 0, sizeof(track_artist));
    c = handle->area[area].area_track_text[track].track_type_performer;
    if (c)
    {
        strncpy(track_artist, c, 59);
    }

    memset(track_title, 0, sizeof(track_title));
    c = handle->area[area].area_track_text[track].track_type_title;
    if (c)
    {
        strncpy(track_title, c, 59);
    }

    memset(disc_album_title, 0, sizeof(disc_album_title));
    if (disc_album_title_position)
    {
        char *c = (char *) master_text + disc_album_title_position;
        char *pos = strchr(c, ';');
        if (!pos)
            pos = c + strlen(c);
        strncpy(disc_album_title, c, min(pos - c, 59));
    }

    snprintf(disc_album_year, sizeof(disc_album_year), "%04d", handle->master_toc->disc_date_year);

    sanitize_filename(track_artist);
    sanitize_filename(disc_album_title);
    sanitize_filename(track_title);

    if (strlen(track_artist) > 0 && strlen(track_title) > 0)
        return parse_format("%N - %A - %T", track + 1, disc_album_year, track_artist, disc_album_title, track_title);
    else if (strlen(track_artist) > 0)
        return parse_format("%N - %A", track + 1, disc_album_year, track_artist, disc_album_title, track_title);
    else if (strlen(track_title) > 0)
        return parse_format("%N - %T", track + 1, disc_album_year, track_artist, disc_album_title, track_title);
    else if (strlen(disc_album_title) > 0)
        return parse_format("%N - %L", track + 1, disc_album_year, track_artist, disc_album_title, track_title);
    else
        return parse_format("%N - Unknown Artist", track + 1, disc_album_year, track_artist, disc_album_title, track_title);
}


char *get_speaker_config_string(area_toc_t *area) 
{
    if (area->channel_count == 2 && area->extra_settings == 0)
    {
        return "2ch.";
    }
    else if (area->channel_count == 5 && area->extra_settings == 3)
    {
        return "5ch.";
    }
    else if (area->channel_count == 6 && area->extra_settings == 4)
    {
        return "5.1ch";
    }
    else
    {
        return "Unknown";
    }
}

char *get_frame_format_string(area_toc_t *area) 
{
    if (area->frame_format == FRAME_FORMAT_DSD_3_IN_14)
    {
        return "DSD 3 in 14";
    }
    else if (area->frame_format == FRAME_FORMAT_DSD_3_IN_16)
    {
        return "DSD 3 in 16";
    }
    else if (area->frame_format == FRAME_FORMAT_DST)
    {
        return "Lossless DST";
    }
    else
    {
        return "Unknown";
    }
}
