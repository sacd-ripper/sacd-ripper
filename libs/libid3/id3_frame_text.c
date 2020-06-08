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

uint8_t *id3_encodeUTF8_to_UTF16_text(const char *str)
{
	// UTF-16 has BOM included, but is machine dependent BE(Windows) or LE(Linux); finally we use UTF-16LE which does not have BOM included, so we must add BOM 'FE FF'
	uint8_t *text_encoded = (uint8_t *)charset_convert(str, strlen(str), "UTF-8", "UTF-16LE"); // or UCS-2LE

	return text_encoded;
}

uint8_t *id3_encodeUTF8_to_ASCII_text(const char *str)
{
	uint8_t *text_encoded = (uint8_t *)charset_convert(str, strlen(str), "UTF-8", "ISO-8859-1");

	return text_encoded;
}

/*
 * Function id3_set_text (frame, text)
 *
 *    input text is allways UTF-8 encoded
 *    Set text for the indicated frame in ISO-8859-1 encoding
 *    Return 0 upon success, or -1 if an error occured.
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

	// convert text to ISO-8859-1
	uint8_t *text_ascii = id3_encodeUTF8_to_ASCII_text(text);
	/*
	 * Allocate memory for new data.
	 */
	frame->fr_raw_size = strlen(text) + 2; // BUG - must add 1 for encoding byte and 1 byte for ending zero of string (copied in fr_data)
	frame->fr_raw_data = calloc(frame->fr_raw_size + 1,1);

	/*
	 * Copy contents.
	 */
	*(int8_t *) frame->fr_raw_data = ID3_ENCODING_ISO_8859_1;
	memcpy((char *)frame->fr_raw_data + 1, text_ascii, frame->fr_raw_size);

	free(text_ascii);

	frame->fr_altered = 1;
	frame->fr_owner->id3_altered = 1;

	frame->fr_data = frame->fr_raw_data;
	frame->fr_size = frame->fr_raw_size;

	return 0;
}

/*
 * Function id3_set_text_utf8 (frame, text)
 * 
 *    input text is allways UTF-8 encoded
 *    Set text for the indicated frame in  UTF-8 encoding.
 *    Return 0 upon success, or -1 if an error occured.
 *
 */
int id3_set_text_utf8(struct id3_frame *frame, char *text)
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
	frame->fr_raw_size = strlen(text) + 2; // BUG - must add 1 for encoding byte and 1 byte for ending zero of string (copied in fr_raw_data)
	frame->fr_raw_data = calloc(frame->fr_raw_size + 1, 1);

	/*
	 * Copy contents.
	 */
	*(uint8_t *)frame->fr_raw_data = ID3_ENCODING_UTF8;
	memcpy((uint8_t *)frame->fr_raw_data + 1, text, frame->fr_raw_size);

	frame->fr_altered = 1;
	frame->fr_owner->id3_altered = 1;

	frame->fr_data = frame->fr_raw_data;
	frame->fr_size = frame->fr_raw_size;

	return 0;
}

/*
 * Function id3_set_text_utf16 (frame, text)
 *
 *    input text is allways UTF-8 encoded
 *    Set text for the indicated frame inf UTF-16 encoding
 *    Return 0 upon success, or -1 if an error occured.
 *
 */
int id3_set_text_utf16(struct id3_frame *frame, char *text)
{
	/* Type check */
	if (frame->fr_desc->fd_idstr[0] != 'T')
		return -1;

	/*
	 * Release memory occupied by previous data.
	 */
	id3_frame_clear_data(frame);

	// convert text to UTF16
	uint8_t *text_utf16 = id3_encodeUTF8_to_UTF16_text(text);

	/*
	 * Allocate memory for new data.
	 */
	frame->fr_raw_size = 2 * strlen(text) + 5; // BUG - must add 1 for encoding byte 2 bytes for BOM, 2 byte for ending Unicode NULL (00 00) of string (copied in fr_raw_data)
	frame->fr_raw_data = calloc(frame->fr_raw_size +1,1);

	/*
	 * Copy contents.
	 */
	*(uint8_t *)frame->fr_raw_data = ID3_ENCODING_UTF16;
	uint8_t BOM_array[] = {0xff, 0xfe}; // UTF-16LE the BOM is 'FF FE'  ; UTF-16BE the BOM is 'FE FF'

	memcpy((uint8_t *)frame->fr_raw_data + 1, BOM_array, 2); 
	memcpy((uint8_t *)frame->fr_raw_data + 3, (uint8_t *)text_utf16, frame->fr_raw_size-2);

	free(text_utf16);

	frame->fr_altered = 1;
	frame->fr_owner->id3_altered = 1;

	frame->fr_data = frame->fr_raw_data;
	frame->fr_size = frame->fr_raw_size;

	return 0;
}

/*
 * Function id3_set_text_wraper (frame, text, id3_tag_mode)
 *
 *    input text is allways UTF-8 encoded;
 *    Set text for the indicated frame:
 * 		- ISO-8859-1 or UTF-16 encodings for ID3v2.3 tags;
 * 		- UTF-8 encoding for ID3v2.4 tags
 *    Return 0 upon success, or -1 if an error occured.
 *    This wraper is based on handle->id3_tag_mode.
 *
 */
int id3_set_text_wraper(struct id3_frame *frame, char *text, int id3_tag_mode)
{
	int result = -1;

	switch (id3_tag_mode)
	{
		case 1:
		case 2:
			/*id3v2.3 UTF-16 */
			result = id3_set_text_utf16(frame, text);
			break;
		case 3:
			/* id3v2.3 ISO_8859_1 (ASCII)*/
			result = id3_set_text(frame, text);
			break;
		case 4:
		case 5:
			/* id3v2.4 UTF8 */
			result = id3_set_text_utf8(frame, text);
			break;
		default:
			/* 0 no id3 tag */
			break;
	}

	return result;
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
 *    Set comment for the indicated frame (only UTF-8 is currently
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
	frame->fr_raw_size = 1 + strlen(description) + 1 + strlen(comment) + 1; // BUG - must add 1 for ending zero of string (copied in fr_data)
	frame->fr_raw_data = calloc(frame->fr_raw_size + 1, 1);

	/*
	 * Copy contents.
	 */
	*(uint8_t *)frame->fr_raw_data =ID3_ENCODING_UTF8; // ID3_ENCODING_ISO_8859_1;
	memcpy((uint8_t *)frame->fr_raw_data + 1, description, strlen(description));
	memcpy((uint8_t *)frame->fr_raw_data + 1 + strlen(description) + 1, comment, strlen(comment));

	frame->fr_altered = 1;
	frame->fr_owner->id3_altered = 1;

	frame->fr_data = frame->fr_raw_data;
	frame->fr_size = frame->fr_raw_size;

	return 0;
}

/*
 * Function id3_set_text__performer (frame, text)
 *	 Input text is allways UTF-8 encoded
 *   Set text for the description:PREFORMER of indicated frame 
 *   Return 0 upon success, or -1 if an error occured.
 *         ISO_8859_1 encoding
 */
int id3_set_text__performer(struct id3_frame *frame, char *text)
{
	// Type check
	if (frame->fr_desc->fd_idstr[0] != 'T')
		return -1;

	// Release memory occupied by previous data.

	id3_frame_clear_data(frame);

	// convert text to ISO-8859-1
	uint8_t *text_ascii = id3_encodeUTF8_to_ASCII_text(text);

	// Allocate memory for new data.

	frame->fr_raw_size = strlen("PERFORMER") + strlen(text) + 3; // 1 for encoding, 1 for null description and 1 for null terminator of text
	frame->fr_raw_data = calloc(frame->fr_raw_size + 1, 1);

	// Copy contents.

	*(uint8_t *)frame->fr_raw_data = ID3_ENCODING_ISO_8859_1;

	memcpy((uint8_t *)frame->fr_raw_data + 1, "PERFORMER", strlen("PERFORMER"));
	memcpy((uint8_t *)frame->fr_raw_data + 1 + strlen("PERFORMER") + 1, text_ascii, strlen(text));
	free(text_ascii);

	frame->fr_altered = 1;
	frame->fr_owner->id3_altered = 1;

	frame->fr_data = frame->fr_raw_data;
	frame->fr_size = frame->fr_raw_size;

	return 0;
}

/*
 * Function id3_set_text__performer (frame, text)
 * 	 Input text is allways UTF-8 encoded
 *   Set text for the description:PREFORMER of indicated frame 
 *   Return 0 upon success, or -1 if an error occured.
 *         UTF-8 encoding
 */
int id3_set_text__performer_utf8(struct id3_frame *frame, char *text)
{
	// Type check
	if (frame->fr_desc->fd_idstr[0] != 'T')
		return -1;

	// Release memory occupied by previous data.

	id3_frame_clear_data(frame);

	// Allocate memory for new data.

	frame->fr_raw_size = strlen("PERFORMER") + strlen(text) + 3; // 1 for encoding, 1 for null description and 1 for null terminator of text
	frame->fr_raw_data = calloc(frame->fr_raw_size + 1, sizeof(uint8_t));

	// Copy contents.

	*(uint8_t *)frame->fr_raw_data = ID3_ENCODING_UTF8;

	memcpy((uint8_t *)frame->fr_raw_data + 1, "PERFORMER", strlen("PERFORMER"));
	memcpy((uint8_t *)frame->fr_raw_data + 1 + strlen("PERFORMER") + 1, text, strlen(text));

	frame->fr_altered = 1;
	frame->fr_owner->id3_altered = 1;

	frame->fr_data = frame->fr_raw_data;
	frame->fr_size = frame->fr_raw_size;

	return 0;
}

/*
 * Function id3_set_text__performer_UTF16 (frame, text)
 *    input text is allways UTF-8 encoded
 *    Set text for the description:PREFORMER of indicated frame 
 *   Return 0 upon success, or -1 if an error occured.
 *         UTF-16 encoding
 */
int id3_set_text__performer_UTF16(struct id3_frame *frame, char *text)
{
	// Type check
	if (frame->fr_desc->fd_idstr[0] != 'T')
		return -1;

	// Release memory occupied by previous data.

	id3_frame_clear_data(frame);

	// Allocate memory for new data.

	uint8_t *performer_utf16 = id3_encodeUTF8_to_UTF16_text("PERFORMER");
	uint8_t *text_utf16 = id3_encodeUTF8_to_UTF16_text(text);

	frame->fr_raw_size = 2 * strlen("PERFORMER") + 2 * strlen(text) + 9; // 1 for encoding, 2 for BOM, 2 for UNICODE null  description, 2 BOM, 2 for UNICODE null terminator of text
	frame->fr_raw_data = calloc(frame->fr_raw_size + 1, sizeof(uint8_t));

	// Copy contents.

    int offset=0;
	*(uint8_t *)frame->fr_raw_data = ID3_ENCODING_UTF16;
	offset += 1;
	uint8_t BOM_array[] = {0xff, 0xfe}; // UTF-16LE the BOM is 'FF FE'

	memcpy((uint8_t *)frame->fr_raw_data + offset, BOM_array, 2);
	offset += 2;
	memcpy((uint8_t *)frame->fr_raw_data + offset, (uint8_t *)performer_utf16, 2 * strlen("PERFORMER")+2);

	offset += (2 * strlen("PERFORMER"))+2;
	memcpy((uint8_t *)frame->fr_raw_data + offset, BOM_array, 2);
	offset +=2;
	memcpy((uint8_t *)frame->fr_raw_data + offset, (uint8_t *)text_utf16, 2 * strlen(text)+2);

	free(performer_utf16);
	free(text_utf16);

	frame->fr_altered = 1;
	frame->fr_owner->id3_altered = 1;

	frame->fr_data = frame->fr_raw_data;
	frame->fr_size = frame->fr_raw_size;

	return 0;
}

/*
 * Function id3_set_text__performer_wraper (frame, text, id3_tag_mode)
 *
 *    input text is allways UTF-8 encoded;
 *    Set text for the indicated frame:
 * 		- ISO-8859-1 or UTF-16 encodings for ID3v2.3 tags;
 * 		- UTF-8 encoding for ID3v2.4 tags
 *    Return 0 upon success, or -1 if an error occured.
 *    This wraper is based on handle->id3_tag_mode.
 *
 */
int id3_set_text__performer_wraper(struct id3_frame *frame, char *text, int id3_tag_mode)
{
	int result = -1;

	switch (id3_tag_mode)
	{
	case 1:
	case 2:
		/*id3v2.3 UTF-16 */
		result = id3_set_text__performer_UTF16(frame, text);
		break;
	case 3:
		/* id3v2.3 ISO_8859_1 (ASCII)*/
		result = id3_set_text__performer(frame, text);
		break;
	case 4:
	case 5:
		/* id3v2.4 UTF8 */
		result = id3_set_text__performer_utf8(frame, text);
		break;
	default:
		/* 0 no id3 tag */
		break;
	}

	return result;
}
