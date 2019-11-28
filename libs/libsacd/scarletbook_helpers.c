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

#include <string.h>
#include <stdlib.h>
#include <fileutils.h>
#include <utils.h>

#include "scarletbook_helpers.h"

#define MAX_DISC_ARTIST_LEN 40
#define MAX_ALBUM_TITLE_LEN 60
#define MAX_TRACK_TITLE_LEN 100
#define MAX_TRACK_ARTIST_LEN 40

int utf8cpy(char *dst, char *src, int n)
{
    // n is the size of dst (including the last byte for null
    int i = 0;

    while (i < n)
    {
        int c;
        if (!(src[i] & 0x80))
        {
            // ASCII code
            if (src[i] == '\0')
            {
                break;
            }
            c = 1;
        }
        else if ((src[i] & 0xe0) == 0xc0)
        {
            // 2-byte code
            c = 2;
        }
        else if ((src[i] & 0xf0) == 0xe0)
        {
            // 3-byte code
            c = 3;
        }
        else if ((src[i] & 0xf8) == 0xf0)
        {
            // 4-byte code
            c = 4;
        }
        else if (src[i] == '\0')
        {
            break;
        }
        else
        {
            break;
        }
        
        if (i + c <= n)
        {
            memcpy(dst + i, src + i, c * sizeof(char));
            i += c;
        }
        else
        {
            break;
        }
    }

    dst[i] = '\0';
    return i;
}


char * get_album_dir(scarletbook_handle_t *handle)
{
    char disc_artist[MAX_DISC_ARTIST_LEN + 1];
    char disc_album_title[MAX_ALBUM_TITLE_LEN + 1];
    char disc_album_year[5];
    char *albumdir;
    master_text_t *master_text = &handle->master_text;
    char *p_artist = 0;
    char *p_album_title = 0;

    if (master_text->disc_artist)
        p_artist = master_text->disc_artist;
    else if (master_text->disc_artist_phonetic)
        p_artist = master_text->disc_artist_phonetic;
    else if (master_text->album_artist)
        p_artist = master_text->album_artist;
    else if (master_text->album_artist_phonetic)
        p_artist = master_text->album_artist_phonetic;

    if (master_text->disc_title)
        p_album_title = master_text->disc_title;
    else if (master_text->disc_title_phonetic)
        p_album_title = master_text->disc_title_phonetic;
    else if (master_text->album_title)
        p_album_title = master_text->album_title;
    else if (master_text->album_title_phonetic)
        p_album_title = master_text->album_title_phonetic;

    memset(disc_artist, 0, sizeof(disc_artist));
    if (p_artist)
    {
        char *pos = strchr(p_artist, ';');
        if (!pos)
            pos = p_artist + strlen(p_artist);
        char *pos1 = strstr(p_artist, " -");
        if (pos1 != NULL && pos1 < pos)			
            pos = pos1;
        pos1 = strchr(p_artist, ',');
        if (pos1 != NULL && pos1 < pos)			
            pos = pos1;
        pos1 = strchr(p_artist, '/');  // standard artist separator
        if (pos1 != NULL && pos1 < pos)			
            pos = pos1;
		
        utf8cpy(disc_artist, p_artist, min(pos - p_artist, MAX_DISC_ARTIST_LEN));
    }

    memset(disc_album_title, 0, sizeof(disc_album_title));
    if (p_album_title)
    {
        char *pos = strchr(p_album_title, ';');
        if (!pos)
            pos = p_album_title + strlen(p_album_title);
        utf8cpy(disc_album_title, p_album_title, min(pos - p_album_title, MAX_ALBUM_TITLE_LEN));
    }

    char multiset_s[20] = "";
    char *disc_album_title_final;

    if (handle->master_toc->album_set_size > 1) // Set of discs
    {       
        snprintf(multiset_s, sizeof(multiset_s), " [disc %d-%d]", handle->master_toc->album_sequence_number, handle->master_toc->album_set_size);
    }
    disc_album_title_final = (char *)malloc(strlen(disc_album_title) + strlen(multiset_s) + 1);
    sprintf(disc_album_title_final, "%s%s", disc_album_title, multiset_s);

    snprintf(disc_album_year, sizeof(disc_album_year), "%04d", handle->master_toc->disc_date_year);

    sanitize_filename(disc_artist);
    sanitize_filename(disc_album_title_final);

    if (strlen(disc_artist) > 0 && strlen(disc_album_title) > 0)
        albumdir = parse_format("%A - %L", 0, disc_album_year, disc_artist, disc_album_title_final, NULL);
    else if (strlen(disc_artist) > 0)
        albumdir = parse_format("%A", 0, disc_album_year, disc_artist, disc_album_title_final, NULL);
    else if (strlen(disc_album_title) > 0)
        albumdir = parse_format("%L", 0, disc_album_year, disc_artist, disc_album_title_final, NULL);
    else
        albumdir = parse_format("Unknown Album", 0, disc_album_year, disc_artist, disc_album_title_final, NULL);

    sanitize_filepath(albumdir);

    free(disc_album_title_final);
    
    return albumdir;
}

char *get_music_filename(scarletbook_handle_t *handle, int area, int track, const char *override_title)
{
    char *c;
    char track_artist[MAX_TRACK_ARTIST_LEN + 1];
    char track_title[MAX_TRACK_TITLE_LEN + 1];
    char disc_album_title[MAX_ALBUM_TITLE_LEN + 1];
    char disc_album_year[5];
    master_text_t *master_text = &handle->master_text;
    char * p_album_title = 0;

    if (master_text->disc_title)
        p_album_title = master_text->disc_title;
    else if (master_text->disc_title_phonetic)
        p_album_title = master_text->disc_title_phonetic;
    else if (master_text->album_title)
        p_album_title = master_text->album_title;
    else if (master_text->album_title_phonetic)
        p_album_title = master_text->album_title_phonetic;

    memset(track_artist, 0, sizeof(track_artist));
    c = handle->area[area].area_track_text[track].track_type_performer;
    if (c)
    {
        char *pos = strchr(c, ';') ;
        if (!pos)
            pos=c + strlen(c);
        char *pos1 = strstr(c, " -");
        if (pos1 != NULL && pos1 < pos)
            pos = pos1;
        pos1 = strchr(c, ',');
        if (pos1 != NULL && pos1 < pos)
            pos = pos1;
        pos1 = strchr(c, '/');  // standard artist separator
        if (pos1 != NULL && pos1 < pos)			
            pos = pos1;
		
        utf8cpy(track_artist, c, min(pos - c, MAX_TRACK_ARTIST_LEN));
    }

    memset(track_title, 0, sizeof(track_title));
    c = handle->area[area].area_track_text[track].track_type_title;
    if (c)
    {
        utf8cpy(track_title, c, MAX_TRACK_TITLE_LEN);
    }

    memset(disc_album_title, 0, sizeof(disc_album_title));
    if (p_album_title)
    {
        char *pos = strchr(p_album_title, ';');
        if (!pos)
            pos = p_album_title + strlen(p_album_title);
        utf8cpy(disc_album_title, p_album_title, min(pos - p_album_title, MAX_ALBUM_TITLE_LEN));
    }

    snprintf(disc_album_year, sizeof(disc_album_year), "%04d", handle->master_toc->disc_date_year);

    sanitize_filename(track_artist);
    sanitize_filename(disc_album_title);
    sanitize_filename(track_title);

    if (override_title && strlen(override_title) > 0)
        return parse_format("%N - %T", track + 1, disc_album_year, track_artist, disc_album_title, override_title);
    else if (strlen(track_artist) > 0 && strlen(track_title) > 0)
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
