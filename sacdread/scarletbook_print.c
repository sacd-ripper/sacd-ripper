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

void scarletbook_print_master_toc(master_toc_t *mtoc) {
	int i;
	char tmp_str[20];

	printf("Disc:\n");
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

	printf("Album:\n");
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
}


void scarletbook_print(scarletbook_handle_t *handle) {

	if(!handle) {
		fprintf(stderr, "No valid ScarletBook handle available\n");
		return;
	}

	if (handle->master_toc) {
		scarletbook_print_master_toc(handle->master_toc);
	}

}
