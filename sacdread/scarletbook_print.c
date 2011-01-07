/**
 * Copyright (c) 2011 Mr Wicked, http://code.google.com/p/sacd-ripper/
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "scarletbook_types.h"
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
	printf("\tVersion: %2i.%2i\n", (mtoc->disc_version >> 8) & 0xff, mtoc->disc_version & 0xff);
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

void scarletbook_print_channel_toc(channel_toc_t *channel) {

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
			scarletbook_print_channel_toc(handle->channel_toc[i]);
		}
	}

}
