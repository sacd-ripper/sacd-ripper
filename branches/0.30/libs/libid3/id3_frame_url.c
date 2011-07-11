/*
 *    Copyright (C) 1999, 2001, 2002  Espen Skoglund
 *    Copyright (C) 2000 - 2004  Haavard Kvaalen
 *
 * $Id: id3_frame_url.c,v 1.9 2004/04/04 16:14:31 havard Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
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

#include "id3.h"
#include "id3_header.h"

/*
 * Function id3_get_url (frame)
 *
 *    Return URL of frame.
 *
 */
char *id3_get_url(struct id3_frame *frame)
{
	int offset = 0;
	/* Type check */
	if (frame->fr_desc->fd_idstr[0] != 'W')
		return NULL;

	/* Check if frame is compressed */
	if (id3_decompress_frame(frame) == -1)
		return NULL;

	if (frame->fr_desc->fd_id == ID3_WXXX)
	{
		/*
		 * This is a user defined link frame.  Skip the description.
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
 * Function id3_get_url_desc (frame)
 *
 *    Get description of a URL.
 *
 */
char *id3_get_url_desc(struct id3_frame *frame)
{
	/* Type check */
	if (frame->fr_desc->fd_idstr[0] != 'W')
		return NULL;

	/* If predefined link frame, return description. */
	if (frame->fr_desc->fd_id != ID3_WXXX)
		return frame->fr_desc->fd_description;

	/* Check if frame is compressed */
	if (id3_decompress_frame(frame) == -1)
		return NULL;

	return id3_string_decode(ID3_TEXT_FRAME_ENCODING(frame),
				 ID3_TEXT_FRAME_PTR(frame));
}
