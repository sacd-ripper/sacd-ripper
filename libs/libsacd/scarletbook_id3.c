#if 0
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

#include "config.h"
#include "scarletbook.h"

#include "id3tag.h"

static void update_id3_frame (struct id3_tag *tag, const char *frame_name, const char *data) {
	int res;
	struct id3_frame *frame;
	union id3_field *field;
	id3_ucs4_t *ucs4;

	if (data == NULL) 
		return;

	/*
	* An empty string removes the frame altogether.
	*/
	if (strlen(data) == 0) {
		while ((frame = id3_tag_findframe (tag, frame_name, 0)))
			id3_tag_detachframe (tag, frame);
		return;
	}

	frame = id3_tag_findframe (tag, frame_name, 0);
	if (!frame) {
		frame = id3_frame_new (frame_name);
		id3_tag_attachframe (tag, frame);
	}

	if (frame_name == ID3_FRAME_COMMENT) {
		field = id3_frame_field (frame, 3);
		field->type = ID3_FIELD_TYPE_STRINGFULL;
	} else {
		field = id3_frame_field (frame, 1);
		field->type = ID3_FIELD_TYPE_STRINGLIST;
	}

	ucs4 = id3_latin1_ucs4duplicate (data);

	if (frame_name == ID3_FRAME_GENRE) {
		char tmp[10];
		int index = id3_genre_number (ucs4);
		free (ucs4);
		sprintf(tmp, "%d", index);
		ucs4 = id3_latin1_ucs4duplicate (tmp);
	}

	if (frame_name == ID3_FRAME_COMMENT) {
		res = id3_field_setfullstring (field, ucs4);
	} else {
		res = id3_field_setstrings (field, 1, &ucs4);
	}

	if (res != 0) {
		fprintf(stderr, "error setting id3 field: %s\n", frame_name);
	}
} 

int scarletbook_id3_tag_render(scarletbook_handle_t *handle, uint8_t *buffer, int channel, int track) {

	struct id3_tag *id3tag; 
	int len;

	id3tag = id3_tag_new();
	id3_tag_clearframes(id3tag);
	id3tag->options |= ID3_TAG_OPTION_COMPRESSION; 

	// TODO
	update_id3_frame (id3tag, ID3_FRAME_TITLE, "Titles");
	update_id3_frame (id3tag, ID3_FRAME_ARTIST, "Artist");
	update_id3_frame (id3tag, ID3_FRAME_ALBUM, "Album"); 
	update_id3_frame (id3tag, ID3_FRAME_YEAR, "");
	update_id3_frame (id3tag, ID3_FRAME_COMMENT, "");
	update_id3_frame (id3tag, ID3_FRAME_TRACK, "");
	update_id3_frame (id3tag, ID3_FRAME_GENRE, ""); 

	len = id3_tag_render(id3tag, buffer);

	id3_tag_delete(id3tag);

	return len;
}
#endif