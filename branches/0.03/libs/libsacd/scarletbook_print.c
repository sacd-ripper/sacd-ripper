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

#include "scarletbook.h"
#include "scarletbook_print.h"

char *substr(const char *pstr, int start, int numchars) {
	static char pnew[255];
	strncpy(pnew, pstr + start, numchars);
	pnew[numchars] = '\0';
	return pnew;
}

void scarletbook_print_album_text(scarletbook_handle_t *handle) {
	int i;
	master_toc_t *master_toc = handle->master_toc;

	for (i = 0; i < master_toc->text_channel_count; i++) {
		master_text_t *master_text = handle->master_text[i];
		printf("\tLocale: %c%c\n", master_toc->locales[i].language_code[0], master_toc->locales[i].language_code[1]);

		if (master_text->album_title_position)
			printf("\tTitle: %s\n", substr((char*) master_text + master_text->album_title_position, 0, 60));
		if (master_text->album_title_phonetic_position)
			printf("\tTitle Phonetic: %s\n", substr((char*) master_text + master_text->album_title_phonetic_position, 0, 60));
		if (master_text->album_artist_position)
			printf("\tArtist: %s\n", substr((char*) master_text + master_text->album_artist_position, 0, 60));
		if (master_text->album_artist_phonetic_position)
			printf("\tArtist Phonetic: %s\n", substr((char*) master_text + master_text->album_artist_phonetic_position, 0, 60));
		if (master_text->album_publisher_position)
			printf("\tPublisher: %s\n", substr((char*) master_text + master_text->album_publisher_position, 0, 60));
		if (master_text->album_publisher_phonetic_position)
			printf("\tPublisher Phonetic: %s\n", substr((char*) master_text + master_text->album_publisher_phonetic_position, 0, 60));
		if (master_text->album_copyright_position)
			printf("\tCopyright: %s\n", substr((char*) master_text + master_text->album_copyright_position, 0, 60));
		if (master_text->album_copyright_phonetic_position)
			printf("\tCopyright Phonetic: %s\n", substr((char*) master_text + master_text->album_copyright_phonetic_position, 0, 60));
	}
}

void scarletbook_print_disc_text(scarletbook_handle_t *handle) {
	int i;
	master_toc_t *master_toc = handle->master_toc;

	for (i = 0; i < master_toc->text_channel_count; i++) {
		master_text_t *master_text = handle->master_text[i];
		printf("\tLocale: %c%c\n", master_toc->locales[i].language_code[0], master_toc->locales[i].language_code[1]);

		if (master_text->disc_title_position)
			printf("\tTitle: %s\n", substr((char*) master_text + master_text->disc_title_position, 0, 60));
		if (master_text->disc_title_phonetic_position)
			printf("\tTitle Phonetic: %s\n", substr((char*) master_text + master_text->disc_title_phonetic_position, 0, 60));
		if (master_text->disc_artist_position)
			printf("\tArtist: %s\n", substr((char*) master_text + master_text->disc_artist_position, 0, 60));
		if (master_text->disc_artist_phonetic_position)
			printf("\tArtist Phonetic: %s\n", substr((char*) master_text + master_text->disc_artist_phonetic_position, 0, 60));
		if (master_text->disc_publisher_position)
			printf("\tPublisher: %s\n", substr((char*) master_text + master_text->disc_publisher_position, 0, 60));
		if (master_text->disc_publisher_phonetic_position)
			printf("\tPublisher Phonetic: %s\n", substr((char*) master_text + master_text->disc_publisher_phonetic_position, 0, 60));
		if (master_text->disc_copyright_position)
			printf("\tCopyright: %s\n", substr((char*) master_text + master_text->disc_copyright_position, 0, 60));
		if (master_text->disc_copyright_phonetic_position)
			printf("\tCopyright Phonetic: %s\n", substr((char*) master_text + master_text->disc_copyright_phonetic_position, 0, 60));
	}
}

void scarletbook_print_master_toc(scarletbook_handle_t *handle) {
	int i;
	char tmp_str[20];
	master_toc_t *mtoc = handle->master_toc;

	printf("Disc Information:\n\n");
	printf("\tVersion: %2i.%02i\n", (mtoc->disc_version >> 8) & 0xff, mtoc->disc_version & 0xff);
	printf("\tCreation date: %4i-%02i-%02i\n"
		, mtoc->disc_date_year, mtoc->disc_date_month, mtoc->disc_date_day);

	if (mtoc->disc_catalog_number) {
		strncpy(tmp_str, mtoc->disc_catalog_number, 16);
		tmp_str[16] = '\0';
		printf("\tCatalog Number: %s\n", tmp_str);
	}

	for (i = 0; i < 4; i++) {
		genre_table_t *t = &mtoc->disc_genre[i];
		if (t->category) {
			printf("\tCategory: %s\n", album_category[t->category]);
			printf("\tGenre: %s\n", album_genre[t->genre]);
		}
	}
	scarletbook_print_disc_text(handle);

	printf("\nAlbum Information:\n\n");
	if (mtoc->disc_catalog_number) {
		strncpy(tmp_str, mtoc->album_catalog_number, 16);
		tmp_str[16] = '\0';
		printf("\tCatalog Number: %s\n", tmp_str);
	}
	printf("\tSequence Number: %i\n", mtoc->album_sequence_number);
	printf("\tSet Size: %i\n", mtoc->album_set_size);

	for (i = 0; i < 4; i++) {
		genre_table_t *t = &mtoc->album_genre[i];
		if (t->category) {
			printf("\tCategory: %s\n", album_category[t->category]);
			printf("\tGenre: %s\n", album_genre[t->genre]);
		}
	}
	scarletbook_print_album_text(handle);
}

void scarletbook_print_channel_toc(scarletbook_handle_t *handle, int channel_nr) {
	int i;
	channel_isrc_t *channel_isrc;
	channel_tracklist_offset_t *channel_offset;
	channel_tracklist_abs_t *channel_time;
	channel_toc_t *channel = handle->channel_toc[channel_nr];
	channel_isrc = handle->channel_isrc[channel_nr];
	channel_offset = handle->channel_tracklist_offset[channel_nr];
	channel_time = handle->channel_tracklist_time[channel_nr];

	printf("\nChannel Information [%i]:\n\n", channel_nr);
	printf("\tVersion: %2i.%02i\n", (channel->version >> 8) & 0xff, channel->version & 0xff);

	if (channel->copyright_offset)
		printf("\tCopyright: %s\n", substr((char*) channel + channel->copyright_offset, 0, 60));
	if (channel->copyright_phonetic_offset)
		printf("\tCopyright Phonetic: %s\n", substr((char*) channel + channel->copyright_phonetic_offset, 0, 60));
	if (channel->area_description_offset)
		printf("\tArea Description: %s\n", substr((char*) channel + channel->area_description_offset, 0, 60));
	if (channel->area_description_phonetic_offset)
		printf("\tArea Description Phonetic: %s\n", substr((char*) channel + channel->area_description_phonetic_offset, 0, 50));
	
	printf("\tTrack Count: %i\n", channel->track_count);
	printf("\tSpeaker config: ");
	if (channel->channel_count == 2 && channel->loudspeaker_config == 0) {
		printf("2 Channel\n");
	} else if (channel->channel_count == 5 && channel->loudspeaker_config == 3) {
		printf("5 Channel\n");
	} else if (channel->channel_count == 6 && channel->loudspeaker_config == 4) {
		printf("6 Channel\n");
	} else {
		printf("Unknown\n");
	}

	for (i = 0; i < channel->track_count; i++) {
		isrc_t *isrc = &channel_isrc->isrc[i];
		printf("\tISRC Track [%d]:\n\t  ", i);
		printf("Country: %s, ", substr(isrc->country_code, 0, 2));
		printf("Owner: %s, ", substr(isrc->owner_code, 0, 3));
		printf("Year: %s, ", substr(isrc->recording_year, 0, 2));
		printf("Designation: %s\n", substr(isrc->designation_code, 0, 5));
	}

	//printf("Track %i, %02:%02i:%02i\n", i, channel_time->track_start_time[i], channel_time->track_stop_time[i], i);

}

void scarletbook_print(scarletbook_handle_t *handle) {
    int i;

	if(!handle) {
		fprintf(stderr, "No valid ScarletBook handle available\n");
		return;
	}

	if (handle->master_toc) {
		scarletbook_print_master_toc(handle);
	}

	printf("\nChannel count: %i\n", handle->channel_count);
	if (handle->channel_count > 0) {
		for (i = 0; i < handle->channel_count; i++) {
			scarletbook_print_channel_toc(handle, i);
		}
	}

}
