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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <utils.h>

#include "scarletbook.h"
#include "scarletbook_print.h"

void scarletbook_print_album_text(scarletbook_handle_t *handle)
{
    int          i;
    master_toc_t *master_toc = handle->master_toc;

    for (i = 0; i < master_toc->text_area_count; i++)
    {
        master_text_t *master_text = handle->master_text[i];
        fprintf(stdout, "\tLocale: %c%c\n", master_toc->locales[i].language_code[0], master_toc->locales[i].language_code[1]);

        if (master_text->album_title_position)
            fprintf(stdout, "\tTitle: %s\n", substr((char *) master_text + master_text->album_title_position, 0, 60));
        if (master_text->album_title_phonetic_position)
            fprintf(stdout, "\tTitle Phonetic: %s\n", substr((char *) master_text + master_text->album_title_phonetic_position, 0, 60));
        if (master_text->album_artist_position)
            fprintf(stdout, "\tArtist: %s\n", substr((char *) master_text + master_text->album_artist_position, 0, 60));
        if (master_text->album_artist_phonetic_position)
            fprintf(stdout, "\tArtist Phonetic: %s\n", substr((char *) master_text + master_text->album_artist_phonetic_position, 0, 60));
        if (master_text->album_publisher_position)
            fprintf(stdout, "\tPublisher: %s\n", substr((char *) master_text + master_text->album_publisher_position, 0, 60));
        if (master_text->album_publisher_phonetic_position)
            fprintf(stdout, "\tPublisher Phonetic: %s\n", substr((char *) master_text + master_text->album_publisher_phonetic_position, 0, 60));
        if (master_text->album_copyright_position)
            fprintf(stdout, "\tCopyright: %s\n", substr((char *) master_text + master_text->album_copyright_position, 0, 60));
        if (master_text->album_copyright_phonetic_position)
            fprintf(stdout, "\tCopyright Phonetic: %s\n", substr((char *) master_text + master_text->album_copyright_phonetic_position, 0, 60));
    }
}

void scarletbook_print_disc_text(scarletbook_handle_t *handle)
{
    int          i;
    master_toc_t *master_toc = handle->master_toc;

    for (i = 0; i < master_toc->text_area_count; i++)
    {
        master_text_t *master_text = handle->master_text[i];
        fprintf(stdout, "\tLocale: %c%c\n", master_toc->locales[i].language_code[0], master_toc->locales[i].language_code[1]);

        if (master_text->disc_title_position)
            fprintf(stdout, "\tTitle: %s\n", substr((char *) master_text + master_text->disc_title_position, 0, 60));
        if (master_text->disc_title_phonetic_position)
            fprintf(stdout, "\tTitle Phonetic: %s\n", substr((char *) master_text + master_text->disc_title_phonetic_position, 0, 60));
        if (master_text->disc_artist_position)
            fprintf(stdout, "\tArtist: %s\n", substr((char *) master_text + master_text->disc_artist_position, 0, 60));
        if (master_text->disc_artist_phonetic_position)
            fprintf(stdout, "\tArtist Phonetic: %s\n", substr((char *) master_text + master_text->disc_artist_phonetic_position, 0, 60));
        if (master_text->disc_publisher_position)
            fprintf(stdout, "\tPublisher: %s\n", substr((char *) master_text + master_text->disc_publisher_position, 0, 60));
        if (master_text->disc_publisher_phonetic_position)
            fprintf(stdout, "\tPublisher Phonetic: %s\n", substr((char *) master_text + master_text->disc_publisher_phonetic_position, 0, 60));
        if (master_text->disc_copyright_position)
            fprintf(stdout, "\tCopyright: %s\n", substr((char *) master_text + master_text->disc_copyright_position, 0, 60));
        if (master_text->disc_copyright_phonetic_position)
            fprintf(stdout, "\tCopyright Phonetic: %s\n", substr((char *) master_text + master_text->disc_copyright_phonetic_position, 0, 60));
    }
}

void scarletbook_print_master_toc(scarletbook_handle_t *handle)
{
    int          i;
    char         tmp_str[20];
    master_toc_t *mtoc = handle->master_toc;

    fprintf(stdout, "Disc Information:\n\n");
    fprintf(stdout, "\tVersion: %2i.%02i\n", mtoc->version.major, mtoc->version.minor);
    fprintf(stdout, "\tCreation date: %4i-%02i-%02i\n"
            , mtoc->disc_date_year, mtoc->disc_date_month, mtoc->disc_date_day);

    if (mtoc->disc_catalog_number)
    {
        strncpy(tmp_str, mtoc->disc_catalog_number, 16);
        tmp_str[16] = '\0';
        fprintf(stdout, "\tCatalog Number: %s\n", tmp_str);
    }

    for (i = 0; i < 4; i++)
    {
        genre_table_t *t = &mtoc->disc_genre[i];
        if (t->category)
        {
            fprintf(stdout, "\tCategory: %s\n", album_category[t->category]);
            fprintf(stdout, "\tGenre: %s\n", album_genre[t->genre]);
        }
    }
    scarletbook_print_disc_text(handle);

    fprintf(stdout, "\nAlbum Information:\n\n");
    if (mtoc->disc_catalog_number)
    {
        strncpy(tmp_str, mtoc->album_catalog_number, 16);
        tmp_str[16] = '\0';
        fprintf(stdout, "\tCatalog Number: %s\n", tmp_str);
    }
    fprintf(stdout, "\tSequence Number: %i\n", mtoc->album_sequence_number);
    fprintf(stdout, "\tSet Size: %i\n", mtoc->album_set_size);

    for (i = 0; i < 4; i++)
    {
        genre_table_t *t = &mtoc->album_genre[i];
        if (t->category)
        {
            fprintf(stdout, "\tCategory: %s\n", album_category[t->category]);
            fprintf(stdout, "\tGenre: %s\n", album_genre[t->genre]);
        }
    }

    scarletbook_print_album_text(handle);
}

void scarletbook_print_area_text(scarletbook_handle_t *handle, int area_idx)
{
    int i;
    fprintf(stdout, "\tTrack list [%d]:\n", area_idx);
    for (i = 0; i < handle->area[area_idx].area_toc->track_count; i++)
    {
        area_track_text_t *track_text = &handle->area[area_idx].area_track_text[i];
        if (track_text->track_type_title)
            fprintf(stdout, "\t\tTitle[%d]: %s\n", i, substr(track_text->track_type_title, 0, 48));
        if (track_text->track_type_title_phonetic)
            fprintf(stdout, "\t\tTitle Phonetic[%d]: %s\n", i, substr(track_text->track_type_title_phonetic, 0, 48));
        if (track_text->track_type_performer)
            fprintf(stdout, "\t\tPerformer[%d]: %s\n", i, substr(track_text->track_type_performer, 0, 48));
        if (track_text->track_type_performer_phonetic)
            fprintf(stdout, "\t\tPerformer Phonetic[%d]: %s\n", i, substr(track_text->track_type_performer_phonetic, 0, 48));
        if (track_text->track_type_songwriter)
            fprintf(stdout, "\t\tSongwriter[%d]: %s\n", i, substr(track_text->track_type_songwriter, 0, 48));
        if (track_text->track_type_songwriter_phonetic)
            fprintf(stdout, "\t\tSongwriter Phonetic[%d]: %s\n", i, substr(track_text->track_type_songwriter_phonetic, 0, 48));
        if (track_text->track_type_composer)
            fprintf(stdout, "\t\tComposer[%d]: %s\n", i, substr(track_text->track_type_composer, 0, 48));
        if (track_text->track_type_composer_phonetic)
            fprintf(stdout, "\t\tComposer Phonetic[%d]: %s\n", i, substr(track_text->track_type_composer_phonetic, 0, 48));
        if (track_text->track_type_arranger)
            fprintf(stdout, "\t\tArranger[%d]: %s\n", i, substr(track_text->track_type_arranger, 0, 48));
        if (track_text->track_type_composer_phonetic)
            fprintf(stdout, "\t\tArranger Phonetic[%d]: %s\n", i, substr(track_text->track_type_arranger_phonetic, 0, 48));
        if (track_text->track_type_message)
            fprintf(stdout, "\t\tMessage[%d]: %s\n", i, substr(track_text->track_type_message, 0, 48));
        if (track_text->track_type_message_phonetic)
            fprintf(stdout, "\t\tMessage Phonetic[%d]: %s\n", i, substr(track_text->track_type_message_phonetic, 0, 48));
        if (track_text->track_type_extra_message)
            fprintf(stdout, "\t\tExtra Message[%d]: %s\n", i, substr(track_text->track_type_extra_message, 0, 48));
        if (track_text->track_type_extra_message_phonetic)
            fprintf(stdout, "\t\tExtra Message Phonetic[%d]: %s\n", i, substr(track_text->track_type_extra_message_phonetic, 0, 48));
    }
}

void scarletbook_print_area_toc(scarletbook_handle_t *handle, int area_idx)
{
    int                        i;
    area_isrc_genre_t             *area_isrc_genre;
    area_tracklist_offset_t *area_tracklist_offset;
    area_tracklist_time_t   *area_tracklist_time;
    area_toc_t              *area_toc = handle->area[area_idx].area_toc;
    area_isrc_genre   = handle->area[area_idx].area_isrc_genre;
    area_tracklist_offset = handle->area[area_idx].area_tracklist_offset;
    area_tracklist_time   = handle->area[area_idx].area_tracklist_time;

    fprintf(stdout, "\tArea Information [%i]:\n\n", area_idx);
    fprintf(stdout, "\tVersion: %2i.%02i\n", area_toc->version.major, area_toc->version.minor);

    if (area_toc->copyright_offset)
        fprintf(stdout, "\tCopyright: %s\n", substr((char *) area_toc + area_toc->copyright_offset, 0, 60));
    if (area_toc->copyright_phonetic_offset)
        fprintf(stdout, "\tCopyright Phonetic: %s\n", substr((char *) area_toc + area_toc->copyright_phonetic_offset, 0, 60));
    if (area_toc->area_description_offset)
        fprintf(stdout, "\tArea Description: %s\n", substr((char *) area_toc + area_toc->area_description_offset, 0, 60));
    if (area_toc->area_description_phonetic_offset)
        fprintf(stdout, "\tArea Description Phonetic: %s\n", substr((char *) area_toc + area_toc->area_description_phonetic_offset, 0, 50));

    fprintf(stdout, "\tTrack Count: %i\n", area_toc->track_count);
    fprintf(stdout, "\tSpeaker config: ");
    if (area_toc->channel_count == 2 && area_toc->extra_settings == 0)
    {
        fprintf(stdout, "2 Channel\n");
    }
    else if (area_toc->channel_count == 5 && area_toc->extra_settings == 3)
    {
        fprintf(stdout, "5 Channel\n");
    }
    else if (area_toc->channel_count == 6 && area_toc->extra_settings == 4)
    {
        fprintf(stdout, "6 Channel\n");
    }
    else
    {
        fprintf(stdout, "Unknown\n");
    }

    scarletbook_print_area_text(handle, area_idx);

    for (i = 0; i < area_toc->track_count; i++)
    {
        isrc_t *isrc = &area_isrc_genre->isrc[i];
        if (*isrc->country_code)
        {
            fprintf(stdout, "\tISRC Track [%d]:\n\t  ", i);
            fprintf(stdout, "Country: %s, ", substr(isrc->country_code, 0, 2));
            fprintf(stdout, "Owner: %s, ", substr(isrc->owner_code, 0, 3));
            fprintf(stdout, "Year: %s, ", substr(isrc->recording_year, 0, 2));
            fprintf(stdout, "Designation: %s\n", substr(isrc->designation_code, 0, 5));
        }
    }
}

void scarletbook_print(scarletbook_handle_t *handle)
{
    int i;

    if (!handle)
    {
        fprintf(stderr, "No valid ScarletBook handle available\n");
        return;
    }

    if (handle->master_toc)
    {
        scarletbook_print_master_toc(handle);
    }

    fprintf(stdout, "\nArea count: %i\n", handle->area_count);
    if (handle->area_count > 0)
    {
        for (i = 0; i < handle->area_count; i++)
        {
            scarletbook_print_area_toc(handle, i);
        }
    }
}
