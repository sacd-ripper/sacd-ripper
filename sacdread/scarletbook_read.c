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
 
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "endianess.h"
#include "scarletbook_types.h"
#include "scarletbook_read.h"
#include "sacd_reader.h"
#include "sacd_read_internal.h"

#ifndef NDEBUG
#define CHECK_ZERO0(arg) \
  if(arg != 0) { \
    fprintf(stderr, "*** Zero check failed in %s:%i\n    for %s = 0x%x\n", \
            __FILE__, __LINE__, # arg, arg); \
  }
#define CHECK_ZERO(arg) \
  if(memcmp(my_friendly_zeros, &arg, sizeof(arg))) { \
    unsigned int i_CZ; \
    fprintf(stderr, "*** Zero check failed in %s:%i\n    for %s = 0x", \
            __FILE__, __LINE__, # arg ); \
    for(i_CZ = 0; i_CZ < sizeof(arg); i_CZ++) \
      fprintf(stderr, "%02x", *((uint8_t *)&arg + i_CZ)); \
    fprintf(stderr, "\n"); \
  }
static const uint8_t my_friendly_zeros[2048];
#else
#define CHECK_ZERO0(arg) (void)(arg)
#define CHECK_ZERO(arg) (void)(arg)
#endif

/* Prototypes for internal functions */
static int scarletbook_read_master_toc(scarletbook_handle_t *);

scarletbook_handle_t *scarletbook_open(sacd_reader_t *sacd, int title) {
  scarletbook_handle_t *sb;

  sb = (scarletbook_handle_t *)malloc(sizeof(scarletbook_handle_t));
  if(!sb)
    return NULL;

  memset(sb, 0, sizeof(scarletbook_handle_t));
  sb->sacd = sacd;

  if (!scarletbook_read_master_toc(sb)) {
    fprintf(stderr, "libsacdread: Can't read Master TOC.\n");
    scarletbook_close(sb);
  }

  return sb;
}

void scarletbook_close(scarletbook_handle_t *handle) {
  if(!handle)
    return;

  if(handle->master_data)
    free(handle->master_data);

  memset(handle, 0, sizeof(scarletbook_handle_t));

  free(handle);
  handle = 0;
}

static int scarletbook_read_master_toc(scarletbook_handle_t *handle) {
  int i; 
  uint8_t* p;
  master_toc_t *master_toc;

  handle->master_data = malloc(MASTER_TOC_LEN * SACD_LSN_SIZE);
  if(!handle->master_data)
	  return 0;

  if (!sacd_read_block_raw(handle->sacd, START_OF_MASTER_TOC, MASTER_TOC_LEN, handle->master_data))
	  return 0;

  master_toc = handle->master_toc = (master_toc_t*) handle->master_data;

  if (strncmp("SACDMTOC", master_toc->id, 8) != 0) {
	  fprintf(stderr, "libsacdread: Not a ScarletBook disc!\n");
    return 0;
  }

  ntoh16(master_toc->disc_version);
  ntoh16(master_toc->album_set_size);
  ntoh16(master_toc->album_sequence_number);
  ntoh32(master_toc->channel_1_toc_area_1_start);
  ntoh32(master_toc->channel_1_toc_area_2_start);
  ntoh32(master_toc->channel_1_toc_area_size);
  ntoh32(master_toc->channel_2_toc_area_1_start);
  ntoh16(master_toc->channel_2_toc_area_2_start);
  ntoh16(master_toc->channel_2_toc_area_size);
  ntoh16(master_toc->disc_date_year);

  if (master_toc->disc_version != SUPPORT_SCARLETBOOK_VERSION) {
	  fprintf(stderr, "libsacdread: Unsupported version: %2i.%2i\n", (master_toc->disc_version >> 8) & 0xff, master_toc->disc_version & 0xff);
	  return 0;
  }

  CHECK_ZERO(master_toc->zero_01);
  CHECK_ZERO(master_toc->zero_02);
  CHECK_ZERO(master_toc->zero_03);
  CHECK_ZERO(master_toc->zero_04);
  CHECK_ZERO(master_toc->zero_05);
  CHECK_ZERO(master_toc->zero_06);
  for (i = 0; i < 4; i++) {
	  CHECK_ZERO(master_toc->album_genre[i].zero_01);
      CHECK_ZERO(master_toc->disc_genre[i].zero_01);
	  CHECK_VALUE(master_toc->album_genre[i].category <= MAX_CATEGORY_COUNT);
	  CHECK_VALUE(master_toc->disc_genre[i].category <= MAX_CATEGORY_COUNT);
	  CHECK_VALUE(master_toc->album_genre[i].genre <= MAX_GENRE_COUNT);
	  CHECK_VALUE(master_toc->disc_genre[i].genre <= MAX_GENRE_COUNT);
  }

  CHECK_VALUE(master_toc->text_channel_count <= MAX_LANGUAGE_COUNT);

  // point to eof master header
  p = handle->master_data + SACD_LSN_SIZE;

  // set pointers to text content
  for (i = 0; i < MAX_LANGUAGE_COUNT; i++) {
     master_text_t *master_text;
	 handle->master_text[i] = master_text = (master_text_t*) p;

	 if(strncmp("SACDText", master_text->id, 8) != 0) {
       return 0;
     }

	 CHECK_ZERO(master_text->zero_01);

     ntoh16(master_text->album_title_position);
     ntoh16(master_text->album_title_phonetic_position);
     ntoh16(master_text->album_artist_position);
     ntoh16(master_text->album_artist_phonetic_position);
     ntoh16(master_text->album_publisher_position);
     ntoh16(master_text->album_publisher_phonetic_position);
     ntoh16(master_text->album_copyright_position);
     ntoh16(master_text->album_copyright_phonetic_position);
     ntoh16(master_text->disc_title_position);
     ntoh16(master_text->disc_title_phonetic_position);
     ntoh16(master_text->disc_artist_position);
     ntoh16(master_text->disc_artist_phonetic_position);
     ntoh16(master_text->disc_publisher_position);
     ntoh16(master_text->disc_publisher_phonetic_position);
     ntoh16(master_text->disc_copyright_position);
	 ntoh16(master_text->disc_copyright_phonetic_position);

	 p += SACD_LSN_SIZE;

  }

  handle->master_man = (master_man_t*) p; 
  if(strncmp("SACD_Man", handle->master_man->id, 8) != 0) {
     return 0;
  }

  return 1;
}
