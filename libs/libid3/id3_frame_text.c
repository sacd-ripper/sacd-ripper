/*
 *    Copyright (C) 1999, 2001, 2002,  Espen Skoglund
 *    Copyright (C) 2000-2004  Haavard Kvaalen
 *
 * $Id: id3_frame_text.c,v 1.14 2004/04/04 22:12:01 havard Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

#include "id3.h"
#include "id3_header.h"

#include "charset.h"


/* Get size of string in bytes including null. */
unsigned int id3_string_size(uint8_t encoding, const char* text)
{
	int length = 0;

	switch (encoding)
	{
		case ID3_ENCODING_ISO_8859_1:
		case ID3_ENCODING_UTF8:
			length = strlen(text) + 1;
			break;
		case ID3_ENCODING_UTF16:
		case ID3_ENCODING_UTF16BE:
			while (*text != 0 || *(text + 1) != 0)
			{
				text += 2;
				length += 2;
			}
			length += 2;
			break;
	}
	return length;
}

/* Returns a newly-allocated string in the locale's encoding. */
char* id3_string_decode(uint8_t encoding, const char* text)
{
	switch (encoding)
	{
		case ID3_ENCODING_ISO_8859_1:
			return strdup(text);
		case ID3_ENCODING_UTF8:
			return charset_from_utf8((char *) text);
		case ID3_ENCODING_UTF16:
			return convert_from_utf16((const uint8_t *) text);
		case ID3_ENCODING_UTF16BE:
			return convert_from_utf16be((const uint8_t *) text);
		default:
			return NULL;
	}
}


/*
 * Function id3_get_encoding (frame)
 *
 *    Return text encoding for frame, or -1 if frame does not have any
 *    text encoding.
 *
 */
int8_t id3_get_encoding(struct id3_frame *frame)
{
	/* Type check */
	if (!id3_frame_is_text(frame) &&
	    frame->fr_desc->fd_id != ID3_WXXX &&
	    frame->fr_desc->fd_id != ID3_IPLS &&
	    frame->fr_desc->fd_id != ID3_USLT &&
	    frame->fr_desc->fd_id != ID3_SYLT &&
	    frame->fr_desc->fd_id != ID3_COMM &&
	    frame->fr_desc->fd_id != ID3_APIC &&
	    frame->fr_desc->fd_id != ID3_GEOB &&
	    frame->fr_desc->fd_id != ID3_USER &&
	    frame->fr_desc->fd_id != ID3_OWNE &&
	    frame->fr_desc->fd_id != ID3_COMR)
		return -1;

	/* Check if frame is compressed */
	if (id3_decompress_frame(frame) == -1)
		return -1;

	return *(int8_t *) frame->fr_data;
}


/*
 * Function id3_set_encoding (frame, encoding)
 *
 *    Set text encoding for frame.  Return 0 upon success, or -1 if an
 *    error occured.
 *
 */
int id3_set_encoding(struct id3_frame *frame, int8_t encoding)
{
	/* Type check */
	if (frame->fr_desc->fd_idstr[0] != 'T' &&
	    frame->fr_desc->fd_id != ID3_WXXX &&
	    frame->fr_desc->fd_id != ID3_IPLS &&
	    frame->fr_desc->fd_id != ID3_USLT &&
	    frame->fr_desc->fd_id != ID3_SYLT &&
	    frame->fr_desc->fd_id != ID3_COMM &&
	    frame->fr_desc->fd_id != ID3_APIC &&
	    frame->fr_desc->fd_id != ID3_GEOB &&
	    frame->fr_desc->fd_id != ID3_USER &&
	    frame->fr_desc->fd_id != ID3_OWNE &&
	    frame->fr_desc->fd_id != ID3_COMR)
		return -1;

	/* Check if frame is compressed */
	if (id3_decompress_frame(frame) == -1)
		return -1;

	/* Changing the encoding of frames is not supported yet */
	if (*(int8_t *) frame->fr_data != encoding)
		return -1;

	/* Set encoding */
	*(int8_t *) frame->fr_data = encoding;
	return 0;
}


/*
 * Function id3_get_text (frame)
 *
 *    Return string contents of frame.
 *
 */
char *id3_get_text(struct id3_frame *frame)
{
	int offset = 0;
	/* Type check */
	if (frame->fr_desc->fd_idstr[0] != 'T')
		return NULL;

	/* Check if frame is compressed */
	if (id3_decompress_frame(frame) == -1)
		return NULL;

	if (frame->fr_desc->fd_id == ID3_TXXX)
	{
		/*
		 * This is a user defined text frame.  Skip the description.
		 */
		offset = id3_string_size(ID3_TEXT_FRAME_ENCODING(frame),
					 ID3_TEXT_FRAME_PTR(frame));
		if (offset >= frame->fr_size)
			return NULL;
	}

	return id3_string_decode(ID3_TEXT_FRAME_ENCODING(frame),
				 ID3_TEXT_FRAME_PTR(frame) + offset);
}


/*
 * Function id3_get_text_desc (frame)
 *
 *    Get description part of a text frame.
 *
 */
char *id3_get_text_desc(struct id3_frame *frame)
{
	/* Type check */
	if (frame->fr_desc->fd_idstr[0] != 'T')
		return NULL;

	/* If predefined text frame, return description. */
	if (frame->fr_desc->fd_id != ID3_TXXX)
		return frame->fr_desc->fd_description;

	/* Check if frame is compressed */
	if (id3_decompress_frame(frame) == -1)
		return NULL;

	return id3_string_decode(ID3_TEXT_FRAME_ENCODING(frame),
				 ID3_TEXT_FRAME_PTR(frame));
}


/*
 * Function id3_get_text_number (frame)
 *
 *    Return string contents of frame translated to a positive
 *    integer, or -1 if an error occured.
 *
 */
int id3_get_text_number(struct id3_frame *frame)
{
	int number = 0;
	char* number_str;

	/* Check if frame is compressed */
	if (id3_decompress_frame(frame) == -1)
		return -1;

	number_str = id3_string_decode(ID3_TEXT_FRAME_ENCODING(frame),
				       ID3_TEXT_FRAME_PTR(frame));

	if (number_str != NULL)
	{
		sscanf(number_str, "%d", &number);
		free(number_str);
	}

	return number;
}


/*
 * Function id3_set_text (frame, text)
 *
 *    Set text for the indicated frame (only ISO-8859-1 is currently
 *    supported).  Return 0 upon success, or -1 if an error occured.
 *
 */
int id3_set_text(struct id3_frame *frame, char *text)
{
	/* Type check */
	if (frame->fr_desc->fd_idstr[0] != 'T')
		return -1;

	/*
	 * Release memory occupied by previous data.
	 */
	id3_frame_clear_data(frame);

	/*
	 * Allocate memory for new data.
	 */
	frame->fr_raw_size = strlen(text) + 1;
	frame->fr_raw_data = malloc(frame->fr_raw_size + 1);

	/*
	 * Copy contents.
	 */
	*(int8_t *) frame->fr_raw_data = ID3_ENCODING_ISO_8859_1;
	memcpy((char *) frame->fr_raw_data + 1, text, frame->fr_raw_size);

	frame->fr_altered = 1;
	frame->fr_owner->id3_altered = 1;

	frame->fr_data = frame->fr_raw_data;
	frame->fr_size = frame->fr_raw_size;

	return 0;
}


/*
 * Function id3_set_text_number (frame, number)
 *
 *    Set number for the indicated frame (only ISO-8859-1 is currently
 *    supported).  Return 0 upon success, or -1 if an error occured.
 *
 */
int id3_set_text_number(struct id3_frame *frame, int number)
{
	char buf[64];
	int pos;
	char *text;

	/* Type check */
	if (frame->fr_desc->fd_idstr[0] != 'T')
		return -1;

	/*
	 * Release memory occupied by previous data.
	 */
	id3_frame_clear_data(frame);

	/*
	 * Create a string with a reversed number.
	 */
	pos = 0;
	while (number > 0 && pos < 64)
	{
		buf[pos++] = (number % 10) + '0';
		number /= 10;
	}
	if (pos == 64)
		return -1;
	if (pos == 0)
		buf[pos++] = '0';

	/*
	 * Allocate memory for new data.
	 */
	frame->fr_raw_size = pos + 1;
	frame->fr_raw_data = malloc(frame->fr_raw_size + 1);

	/*
	 * Insert contents.
	 */
	*(int8_t *) frame->fr_raw_data = ID3_ENCODING_ISO_8859_1;
	text = (char *) frame->fr_raw_data + 1;
	while (--pos >= 0)
		*text++ = buf[pos];
	*text = '\0';

	frame->fr_altered = 1;
	frame->fr_owner->id3_altered = 1;

	frame->fr_data = frame->fr_raw_data;
	frame->fr_size = frame->fr_raw_size;

	return 0;
}

int id3_frame_is_text(struct id3_frame *frame)
{
	if (frame && frame->fr_desc &&
	    (frame->fr_desc->fd_idstr[0] == 'T' ||
	     frame->fr_desc->fd_idstr[0] == 'W'))
		return 1;
	return 0;
}

/*
 * Function id3_get_comment(frame)
 *
 *    Return string contents of a comment frame.
 *
 */
char *id3_get_comment(struct id3_frame *frame)
{
	int offset;
	/* Type check */
	if (frame->fr_desc->fd_id != ID3_COMM)
		return NULL;

	/* Check if frame is compressed */
	if (id3_decompress_frame(frame) == -1)
		return NULL;

	if (frame->fr_size < 5)
		return NULL;

	/* Skip language id */
	offset = 3;
	
	/* Skip the description */
	offset += id3_string_size(ID3_TEXT_FRAME_ENCODING(frame),
				  ID3_TEXT_FRAME_PTR(frame) + offset);
	if (offset >= frame->fr_size)
		return NULL;

	return id3_string_decode(ID3_TEXT_FRAME_ENCODING(frame),
				 ID3_TEXT_FRAME_PTR(frame) + offset);
}

/*
 * Function id3_set_comment (frame, comment)
 *
 *    Set comment for the indicated frame (only ISO-8859-1 is currently
 *    supported).  Return 0 upon success, or -1 if an error occured.
 *
 */
int id3_set_comment(struct id3_frame *frame, char* description, char *comment)
{
	/* Type check */
	if (frame->fr_desc->fd_idstr[0] != 'C')
		return -1;

	/*
	 * Release memory occupied by previous data.
	 */
	id3_frame_clear_data(frame);

	/*
	 * Allocate memory for new data.
	 */
	frame->fr_raw_size = strlen(description) + 1 + strlen(comment) + 1;
	frame->fr_raw_data = malloc(frame->fr_raw_size + 1);

	/*
	 * Copy contents.
	 */
	*(int8_t *) frame->fr_raw_data = ID3_ENCODING_ISO_8859_1;
    memcpy((char *) frame->fr_raw_data + 1, description, strlen(description) + 1);
	memcpy((char *) frame->fr_raw_data + 1 + strlen(description) + 1, comment, strlen(comment) + 1);

	frame->fr_altered = 1;
	frame->fr_owner->id3_altered = 1;

	frame->fr_data = frame->fr_raw_data;
	frame->fr_size = frame->fr_raw_size;

	return 0;
}
