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
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

#include <charset.h>

#include "scarletbook.h"

#include <id3.h>
#include <genre.dat>

int scarletbook_id3_tag_render(scarletbook_handle_t *handle, uint8_t *buffer, int area, int track)
{
    const int sacd_id3_genres[] = {
        12,     /* Not used => Other */
        12,     /* Not defined => Other */
        60,     /* Adult Contemporary => Top 40 */
        40,     /* Alternative Rock => AlternRock */
        12,     /* Children's Music => Other */
        32,     /* Classical => Classical */
        140,    /* Contemporary Christian => Contemporary Christian */
        2,      /* Country => Country */
        3,      /* Dance => Dance */
        98,     /* Easy Listening => Easy Listening */
        109,    /* Erotic => Porn Groove */
        80,     /* Folk => Folk */
        38,     /* Gospel => Gospel */
        7,      /* Hip Hop => Hip-Hop */
        8,      /* Jazz => Jazz */
        86,     /* Latin => Latin */
        77,     /* Musical => Musical */
        10,     /* New Age => New Age */
        103,    /* Opera => Opera */
        104,    /* Operetta => Chamber Music */
        13,     /* Pop Music => Pop */
        15,     /* RAP => Rap */
        16,     /* Reggae => Reggae */
        17,     /* Rock Music => Rock */
        14,     /* Rhythm & Blues => R&B */
        37,     /* Sound Effects => Sound Clip */
        24,     /* Sound Track => Soundtrack */
        101,    /* Spoken Word => Speech */
        48,     /* World Music => Ethnic */
        0,      /* Blues => Blues */
        12,     /* Not used => Other */
    }; 
    struct id3_tag *tag;
    struct id3_frame *frame;
    char tmp[200];
    int len;

    tag = id3_open_mem(0, ID3_OPENF_CREATE);

    memset(tmp, 0, sizeof(tmp));

    if (handle->area[area].area_track_text[track].track_type_title)
    {
        char *track_type_title = charset_convert(handle->area[area].area_track_text[track].track_type_title, strlen(handle->area[area].area_track_text[track].track_type_title), "UTF-8", "ISO-8859-1");
        frame = id3_add_frame(tag, ID3_TIT2);
        id3_set_text(frame, track_type_title);
        free(track_type_title);
    }
    else
    {
        master_text_t *master_text = &handle->master_text;
        char *album_title = 0;

        if (master_text->album_title)
            album_title = master_text->album_title; 
        else if (master_text->album_title_phonetic)
            album_title = master_text->album_title_phonetic;
        else if (master_text->disc_title)
            album_title = master_text->disc_title; 
        else if (master_text->disc_title_phonetic)
            album_title = master_text->disc_title_phonetic;

        if (album_title)
        {
            frame = id3_add_frame(tag, ID3_TIT2);
            album_title = charset_convert(album_title, strlen(album_title), "UTF-8", "ISO-8859-1");
            id3_set_text(frame, album_title);
            free(album_title);
        }
    }
    if (handle->area[area].area_track_text[track].track_type_performer)
    {
        char *performer = handle->area[area].area_track_text[track].track_type_performer; 
        frame = id3_add_frame(tag, ID3_TPE1);
        performer = charset_convert(performer, strlen(performer), "UTF-8", "ISO-8859-1");
        id3_set_text(frame, performer);
        free(performer);
    }
    else
    {
        master_text_t *master_text = &handle->master_text;
        char *artist = 0;

        // preferably we use the title as the artist name, as disc/album artist mostly contains garbage..
        if (master_text->album_title)
            artist = master_text->album_title; 
        else if (master_text->album_title_phonetic)
            artist = master_text->album_title_phonetic;
        else if (master_text->disc_title)
            artist = master_text->disc_title; 
        else if (master_text->disc_title_phonetic)
            artist = master_text->disc_title_phonetic;
        else if (master_text->album_artist)
            artist = master_text->album_artist;
        else if (master_text->album_artist_phonetic)
            artist = master_text->album_artist_phonetic;
        else if (master_text->disc_artist)
            artist = master_text->disc_artist;
        else if (master_text->disc_artist_phonetic)
            artist = master_text->disc_artist_phonetic;

        if (artist)
        {
            frame = id3_add_frame(tag, ID3_TPE1);
            artist = charset_convert(artist, strlen(artist), "UTF-8", "ISO-8859-1");
            id3_set_text(frame, artist);
            free(artist);
        }
    }

    {
        master_text_t *master_text = &handle->master_text;
        char *album_title = 0;

        if (master_text->album_title)
            album_title = master_text->album_title; 
        else if (master_text->album_title_phonetic)
            album_title = master_text->album_title_phonetic;
        else if (master_text->disc_title)
            album_title = master_text->disc_title; 
        else if (master_text->disc_title_phonetic)
            album_title = master_text->disc_title_phonetic;

        if (album_title)
        {
            frame = id3_add_frame(tag, ID3_TALB);
            album_title = charset_convert(album_title, strlen(album_title), "UTF-8", "ISO-8859-1");
            id3_set_text(frame, album_title);
            free(album_title);
        }
    }

    frame = id3_add_frame(tag, ID3_TCON);
    id3_set_text(frame, (char *) genre_table[sacd_id3_genres[handle->area[area].area_isrc_genre->track_genre[track].genre & 0x1f]]);

    snprintf(tmp, 200, "%04d", handle->master_toc->disc_date_year);
    frame = id3_add_frame(tag, ID3_TYER);
    id3_set_text(frame, tmp);

    snprintf(tmp, 200, "%02d%02d", handle->master_toc->disc_date_month, handle->master_toc->disc_date_day);
    frame = id3_add_frame(tag, ID3_TDAT);
    id3_set_text(frame, tmp);

    snprintf(tmp, 200, "%d", track + 1);     // internally tracks start from 0
    frame = id3_add_frame(tag, ID3_TRCK);
    id3_set_text(frame, tmp);

    len = id3_write_tag(tag, buffer);
    id3_close(tag);

    return len;
}
