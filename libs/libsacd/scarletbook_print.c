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
#include <ctype.h>
#include <assert.h>
#include <wchar.h>
#include <locale.h>

#include <charset.h>
#include <utils.h>

#include "scarletbook.h"
#include "scarletbook_print.h"

static const wchar_t *ucs(const char* str) 
{
    static wchar_t buf[2048];
#ifdef _WIN32
    char *wchar_type = (sizeof(wchar_t) == 2) ? "UCS-2-INTERNAL" : "UCS-4-INTERNAL";
#else
    char *wchar_type = "WCHAR_T";
#endif
    wchar_t *wc = (wchar_t *) charset_convert((char *) str, strlen(str), "UTF-8", wchar_type);
    wcscpy(buf, wc);
    free(wc);
    return buf;
}

static void scarletbook_print_album_text(scarletbook_handle_t *handle)
{
    master_toc_t *master_toc = handle->master_toc;
    master_text_t *master_text = &handle->master_text;

    fwprintf(stdout, L"\tLocale: %c%c\n", master_toc->locales[0].language_code[0], master_toc->locales[0].language_code[1]);

    if (master_text->album_title)
        fwprintf(stdout, L"\tTitle: %ls\n", ucs(master_text->album_title));
    if (master_text->album_title_phonetic)
        fwprintf(stdout, L"\tTitle Phonetic: %ls\n", ucs(master_text->album_title_phonetic));
    if (master_text->album_artist)
        fwprintf(stdout, L"\tArtist: %ls\n", ucs(master_text->album_artist));
    if (master_text->album_artist_phonetic)
        fwprintf(stdout, L"\tArtist Phonetic: %ls\n", ucs(master_text->album_artist_phonetic));
    if (master_text->album_publisher)
        fwprintf(stdout, L"\tPublisher: %ls\n", ucs(master_text->album_publisher));
    if (master_text->album_publisher_phonetic)
        fwprintf(stdout, L"\tPublisher Phonetic: %ls\n", ucs(master_text->album_publisher_phonetic));
    if (master_text->album_copyright)
        fwprintf(stdout, L"\tCopyright: %ls\n", ucs(master_text->album_copyright));
    if (master_text->album_copyright_phonetic)
        fwprintf(stdout, L"\tCopyright Phonetic: %ls\n", ucs(master_text->album_copyright_phonetic));
}

static void scarletbook_print_disc_text(scarletbook_handle_t *handle)
{
    master_toc_t *master_toc = handle->master_toc;

    master_text_t *master_text = &handle->master_text;
    fwprintf(stdout, L"\tLocale: %c%c\n", master_toc->locales[0].language_code[0], master_toc->locales[0].language_code[1]);

    if (master_text->disc_title)
        fwprintf(stdout, L"\tTitle: %ls\n", ucs(master_text->disc_title));
    if (master_text->disc_title_phonetic)
        fwprintf(stdout, L"\tTitle Phonetic: %ls\n", ucs(master_text->disc_title_phonetic));
    if (master_text->disc_artist)
        fwprintf(stdout, L"\tArtist: %ls\n", ucs(master_text->disc_artist));
    if (master_text->disc_artist_phonetic)
        fwprintf(stdout, L"\tArtist Phonetic: %ls\n", ucs(master_text->disc_artist_phonetic));
    if (master_text->disc_publisher)
        fwprintf(stdout, L"\tPublisher: %ls\n", ucs(master_text->disc_publisher));
    if (master_text->disc_publisher_phonetic)
        fwprintf(stdout, L"\tPublisher Phonetic: %ls\n", ucs(master_text->disc_publisher_phonetic));
    if (master_text->disc_copyright)
        fwprintf(stdout, L"\tCopyright: %ls\n", ucs(master_text->disc_copyright));
    if (master_text->disc_copyright_phonetic)
        fwprintf(stdout, L"\tCopyright Phonetic: %ls\n", ucs(master_text->disc_copyright_phonetic));
}

static void scarletbook_print_master_toc(scarletbook_handle_t *handle)
{
    int          i;
    char         tmp_str[20];
    master_toc_t *mtoc = handle->master_toc;

    fwprintf(stdout, L"Disc Information:\n\n");
    fwprintf(stdout, L"\tVersion: %2i.%02i\n", mtoc->version.major, mtoc->version.minor);
    fwprintf(stdout, L"\tCreation date: %4i-%02i-%02i\n"
            , mtoc->disc_date_year, mtoc->disc_date_month, mtoc->disc_date_day);

    if (mtoc->disc_catalog_number)
    {
        strncpy(tmp_str, mtoc->disc_catalog_number, 16);
        tmp_str[16] = '\0';
        fwprintf(stdout, L"\tCatalog Number: %ls\n", ucs(tmp_str));
    }

    for (i = 0; i < 4; i++)
    {
        genre_table_t *t = &mtoc->disc_genre[i];
        if (t->category)
        {
            fwprintf(stdout, L"\tCategory: %ls\n", ucs(album_category[t->category]));
            fwprintf(stdout, L"\tGenre: %ls\n", ucs(album_genre[t->genre]));
        }
    }
    scarletbook_print_disc_text(handle);

    fwprintf(stdout, L"\nAlbum Information:\n\n");
    if (mtoc->disc_catalog_number)
    {
        strncpy(tmp_str, mtoc->album_catalog_number, 16);
        tmp_str[16] = '\0';
        fwprintf(stdout, L"\tCatalog Number: %ls\n", ucs(tmp_str));
    }
    fwprintf(stdout, L"\tSequence Number: %i\n", mtoc->album_sequence_number);
    fwprintf(stdout, L"\tSet Size: %i\n", mtoc->album_set_size);

    for (i = 0; i < 4; i++)
    {
        genre_table_t *t = &mtoc->album_genre[i];
        if (t->category)
        {
            fwprintf(stdout, L"\tCategory: %ls\n", ucs(album_category[t->category]));
            fwprintf(stdout, L"\tGenre: %ls\n", ucs(album_genre[t->genre]));
        }
    }

    scarletbook_print_album_text(handle);
}

static void scarletbook_print_area_text(scarletbook_handle_t *handle, int area_idx)
{
    int i;
    fwprintf(stdout, L"\tTrack list [%d]:\n", area_idx);
    for (i = 0; i < handle->area[area_idx].area_toc->track_count; i++)
    {
        area_track_text_t *track_text = &handle->area[area_idx].area_track_text[i];
        if (track_text->track_type_title)
            fwprintf(stdout, L"\t\tTitle[%d]: %ls\n", i, ucs(track_text->track_type_title));
        if (track_text->track_type_title_phonetic)
            fwprintf(stdout, L"\t\tTitle Phonetic[%d]: %ls\n", i, ucs(track_text->track_type_title_phonetic));
        if (track_text->track_type_performer)
            fwprintf(stdout, L"\t\tPerformer[%d]: %ls\n", i, ucs(track_text->track_type_performer));
        if (track_text->track_type_performer_phonetic)
            fwprintf(stdout, L"\t\tPerformer Phonetic[%d]: %ls\n", i, ucs(track_text->track_type_performer_phonetic));
        if (track_text->track_type_songwriter)
            fwprintf(stdout, L"\t\tSongwriter[%d]: %ls\n", i, ucs(track_text->track_type_songwriter));
        if (track_text->track_type_songwriter_phonetic)
            fwprintf(stdout, L"\t\tSongwriter Phonetic[%d]: %ls\n", i, ucs(track_text->track_type_songwriter_phonetic));
        if (track_text->track_type_composer)
            fwprintf(stdout, L"\t\tComposer[%d]: %ls\n", i, ucs(track_text->track_type_composer));
        if (track_text->track_type_composer_phonetic)
            fwprintf(stdout, L"\t\tComposer Phonetic[%d]: %ls\n", i, ucs(track_text->track_type_composer_phonetic));
        if (track_text->track_type_arranger)
            fwprintf(stdout, L"\t\tArranger[%d]: %ls\n", i, ucs(track_text->track_type_arranger));
        if (track_text->track_type_arranger_phonetic)
            fwprintf(stdout, L"\t\tArranger Phonetic[%d]: %ls\n", i, ucs(track_text->track_type_arranger_phonetic));
        if (track_text->track_type_message)
            fwprintf(stdout, L"\t\tMessage[%d]: %ls\n", i, ucs(track_text->track_type_message));
        if (track_text->track_type_message_phonetic)
            fwprintf(stdout, L"\t\tMessage Phonetic[%d]: %ls\n", i, ucs(track_text->track_type_message_phonetic));
        if (track_text->track_type_extra_message)
            fwprintf(stdout, L"\t\tExtra Message[%d]: %ls\n", i, ucs(track_text->track_type_extra_message));
        if (track_text->track_type_extra_message_phonetic)
            fwprintf(stdout, L"\t\tExtra Message Phonetic[%d]: %ls\n", i, ucs(track_text->track_type_extra_message_phonetic));
    }
}

static void scarletbook_print_area_toc(scarletbook_handle_t *handle, int area_idx)
{
    int                        i;
    area_isrc_genre_t       *area_isrc_genre;
    area_tracklist_offset_t *area_tracklist_offset;
    area_tracklist_t        *area_tracklist_time;
    scarletbook_area_t      *area = &handle->area[area_idx];
    area_toc_t              *area_toc = area->area_toc;
    area_isrc_genre   = area->area_isrc_genre;
    area_tracklist_offset = area->area_tracklist_offset;
    area_tracklist_time   = area->area_tracklist_time;

    fwprintf(stdout, L"\tArea Information [%i]:\n\n", area_idx);
    fwprintf(stdout, L"\tVersion: %2i.%02i\n", area_toc->version.major, area_toc->version.minor);

    if (area->copyright)
        fwprintf(stdout, L"\tCopyright: %ls\n", ucs(area->copyright));
    if (area->copyright_phonetic)
        fwprintf(stdout, L"\tCopyright Phonetic: %ls\n", ucs(area->copyright_phonetic));
    if (area->description)
        fwprintf(stdout, L"\tArea Description: %ls\n", ucs(area->description));
    if (area->description_phonetic)
        fwprintf(stdout, L"\tArea Description Phonetic: %ls\n", ucs(area->description_phonetic));

    fwprintf(stdout, L"\tTrack Count: %i\n", area_toc->track_count);
    fwprintf(stdout, L"\tSpeaker config: ");
    if (area_toc->channel_count == 2 && area_toc->extra_settings == 0)
    {
        fwprintf(stdout, L"2 Channel\n");
    }
    else if (area_toc->channel_count == 5 && area_toc->extra_settings == 3)
    {
        fwprintf(stdout, L"5 Channel\n");
    }
    else if (area_toc->channel_count == 6 && area_toc->extra_settings == 4)
    {
        fwprintf(stdout, L"6 Channel\n");
    }
    else
    {
        fwprintf(stdout, L"Unknown\n");
    }

    scarletbook_print_area_text(handle, area_idx);

    for (i = 0; i < area_toc->track_count; i++)
    {
        isrc_t *isrc = &area_isrc_genre->isrc[i];
        if (*isrc->country_code)
        {
            fwprintf(stdout, L"\tISRC Track [%d]:\n\t  ", i);
            fwprintf(stdout, L"Country: %ls, ", ucs(substr(isrc->country_code, 0, 2)));
            fwprintf(stdout, L"Owner: %ls, ", ucs(substr(isrc->owner_code, 0, 3)));
            fwprintf(stdout, L"Year: %ls, ", ucs(substr(isrc->recording_year, 0, 2)));
            fwprintf(stdout, L"Designation: %ls\n", ucs(substr(isrc->designation_code, 0, 5)));
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

    fwprintf(stdout, L"\nArea count: %i\n", handle->area_count);
    if (handle->area_count > 0)
    {
        for (i = 0; i < handle->area_count; i++)
        {
            scarletbook_print_area_toc(handle, i);
        }
    }
}
