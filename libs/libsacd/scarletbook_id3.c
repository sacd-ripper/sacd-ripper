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

    // TIT2 track title
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
    // TPE2 is widely used as album artist
    if (handle->master_text.album_artist)
    {
        char *album_artist = handle->master_text.album_artist;
        frame = id3_add_frame(tag, ID3_TPE2);
        album_artist = charset_convert(album_artist, strlen(album_artist), "UTF-8", "ISO-8859-1");
        id3_set_text(frame, album_artist);
        free(album_artist);
    }
    if (&handle->area[area].area_isrc_genre->isrc[track])
    {
        char isrc[12];
        char *isrc_conv;
        memcpy(isrc, handle->area[area].area_isrc_genre->isrc[track].country_code, 2);
        memcpy(isrc + 2, handle->area[area].area_isrc_genre->isrc[track].owner_code, 3);
        memcpy(isrc + 5, handle->area[area].area_isrc_genre->isrc[track].recording_year, 2);
        memcpy(isrc + 7, handle->area[area].area_isrc_genre->isrc[track].designation_code, 5);

        frame = id3_add_frame(tag, ID3_TSRC);
        isrc_conv = charset_convert(isrc, 12, "UTF-8", "ISO-8859-1");
        id3_set_text(frame, isrc_conv);
        free(isrc_conv);
    }
    if (handle->master_text.album_publisher){
        char *publisher = handle->master_text.album_publisher;
        frame = id3_add_frame(tag, ID3_TPUB);
        publisher = charset_convert(publisher, strlen(publisher), "UTF-8", "ISO-8859-1");
        id3_set_text(frame, publisher);
        free(publisher);
    }
    if (handle->master_text.album_copyright){
        char *copyright = handle->master_text.album_copyright;
        frame = id3_add_frame(tag, ID3_TCOP);
        copyright = charset_convert(copyright, strlen(copyright), "UTF-8", "ISO-8859-1");
        id3_set_text(frame, copyright);
        free(copyright);
    }
    if (handle->master_toc){
        master_toc_t *mtoc = handle->master_toc;
        char str[64];
        char *str_conv;
        sprintf(str, "%d/%d", mtoc->album_sequence_number, mtoc->album_set_size);
        frame = id3_add_frame(tag, ID3_TPOS);
        str_conv = charset_convert(str, strlen(str), "UTF-8", "ISO-8859-1");
        id3_set_text(frame,str_conv);
        free(str_conv);
    }
    if (handle->area[area].area_track_text[track].track_type_composer)
    {
        char *composer = handle->area[area].area_track_text[track].track_type_composer; 
        frame = id3_add_frame(tag, ID3_TCOM);
        composer = charset_convert(composer, strlen(composer), "UTF-8", "ISO-8859-1");
        id3_set_text(frame, composer);
        free(composer);
    }
    // Artist
    if (handle->area[area].area_track_text[track].track_type_performer)
    {
        char *performer = handle->area[area].area_track_text[track].track_type_performer;        
        performer = charset_convert(performer, strlen(performer), "UTF-8", "ISO-8859-1");
		       
		frame = id3_add_frame(tag, ID3_TPE1);  // Artist, soloist
		id3_set_text(frame, performer);

        //frame = id3_add_frame(tag, ID3_TPE3);  // TPE3=Conductor/performer refinement;  TOPE='Original artist(s)/performer(s)' IPLS -Involved people(performer?)
        //id3_set_text(frame, performer);
        //frame = id3_add_frame(tag, ID3_IPLS);  // IPLS -Involved people(performer?)
        //id3_set_text(frame, performer);

        frame = id3_add_frame(tag, ID3_TXXX);  // ID3_TXXX, Performer
        id3_set_text__performer(frame, performer);
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
    // Title of album
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
    // Genre
    frame = id3_add_frame(tag, ID3_TCON);
    id3_set_text(frame, (char *) genre_table[sacd_id3_genres[handle->area[area].area_isrc_genre->track_genre[track].genre & 0x1f]]);
    // YEAR
    snprintf(tmp, 200, "%04d", handle->master_toc->disc_date_year);
    frame = id3_add_frame(tag, ID3_TYER);
    id3_set_text(frame, tmp);
    // Month, day
    snprintf(tmp, 200, "%02d%02d", handle->master_toc->disc_date_month, handle->master_toc->disc_date_day);
    frame = id3_add_frame(tag, ID3_TDAT);
    id3_set_text(frame, tmp);
    // Track number/total tracks
    snprintf(tmp, 200, "%d/%d", track + 1,handle->area[area].area_toc->track_count);     // internally tracks start from 0
    frame = id3_add_frame(tag, ID3_TRCK);
    id3_set_text(frame, tmp);

    len = id3_write_tag(tag, buffer);
    id3_close(tag);

    return len;
}
