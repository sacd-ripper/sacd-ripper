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

    for (i = 0; i < master_toc->text_channel_count; i++)
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

    for (i = 0; i < master_toc->text_channel_count; i++)
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
    fprintf(stdout, "\tVersion: %2i.%02i\n", (mtoc->disc_version >> 8) & 0xff, mtoc->disc_version & 0xff);
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

void scarletbook_print_track_text(scarletbook_handle_t *handle, int channel_nr)
{
    int i;
    fprintf(stdout, "\tTrack list [%d]:\n", channel_nr);
    for (i = 0; i < handle->channel_toc[channel_nr]->track_count; i++)
    {
        channel_track_text_t *track_text = &handle->channel_track_text[channel_nr][i];
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

void scarletbook_print_channel_toc(scarletbook_handle_t *handle, int channel_nr)
{
    int                        i;
    channel_isrc_t             *channel_isrc;
    channel_tracklist_offset_t *channel_offset;
    channel_tracklist_abs_t    *channel_time;
    channel_toc_t              *channel = handle->channel_toc[channel_nr];
    channel_isrc   = handle->channel_isrc[channel_nr];
    channel_offset = handle->channel_tracklist_offset[channel_nr];
    channel_time   = handle->channel_tracklist_time[channel_nr];

    fprintf(stdout, "\nChannel Information [%i]:\n\n", channel_nr);
    fprintf(stdout, "\tVersion: %2i.%02i\n", (channel->version >> 8) & 0xff, channel->version & 0xff);

    if (channel->copyright_offset)
        fprintf(stdout, "\tCopyright: %s\n", substr((char *) channel + channel->copyright_offset, 0, 60));
    if (channel->copyright_phonetic_offset)
        fprintf(stdout, "\tCopyright Phonetic: %s\n", substr((char *) channel + channel->copyright_phonetic_offset, 0, 60));
    if (channel->area_description_offset)
        fprintf(stdout, "\tArea Description: %s\n", substr((char *) channel + channel->area_description_offset, 0, 60));
    if (channel->area_description_phonetic_offset)
        fprintf(stdout, "\tArea Description Phonetic: %s\n", substr((char *) channel + channel->area_description_phonetic_offset, 0, 50));

    fprintf(stdout, "\tTrack Count: %i\n", channel->track_count);
    fprintf(stdout, "\tSpeaker config: ");
    if (channel->channel_count == 2 && channel->loudspeaker_config == 0)
    {
        fprintf(stdout, "2 Channel\n");
    }
    else if (channel->channel_count == 5 && channel->loudspeaker_config == 3)
    {
        fprintf(stdout, "5 Channel\n");
    }
    else if (channel->channel_count == 6 && channel->loudspeaker_config == 4)
    {
        fprintf(stdout, "6 Channel\n");
    }
    else
    {
        fprintf(stdout, "Unknown\n");
    }

    scarletbook_print_track_text(handle, channel_nr);

    for (i = 0; i < channel->track_count; i++)
    {
        isrc_t *isrc = &channel_isrc->isrc[i];
        fprintf(stdout, "\tISRC Track [%d]:\n\t  ", i);
        fprintf(stdout, "Country: %s, ", substr(isrc->country_code, 0, 2));
        fprintf(stdout, "Owner: %s, ", substr(isrc->owner_code, 0, 3));
        fprintf(stdout, "Year: %s, ", substr(isrc->recording_year, 0, 2));
        fprintf(stdout, "Designation: %s\n", substr(isrc->designation_code, 0, 5));
    }

    //fprintf(stdout, "Track %i, %02:%02i:%02i\n", i, channel_time->track_start_time[i], channel_time->track_stop_time[i], i);
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

    fprintf(stdout, "\nChannel count: %i\n", handle->channel_count);
    if (handle->channel_count > 0)
    {
        for (i = 0; i < handle->channel_count; i++)
        {
            scarletbook_print_channel_toc(handle, i);
        }
    }
}
