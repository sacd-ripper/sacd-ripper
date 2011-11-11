/*
 *    Copyright (C) 1999, 2002,  Espen Skoglund
 *    Copyright (C) 2000 - 2004  Haavard Kvaalen
 *
 * $Id: id3_frame_content.c,v 1.15 2007/07/05 13:59:48 shd Exp $
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
#include <stdlib.h>
#include <string.h>

#include "id3.h"

#if 0

enum {
	CTYPE_NUM,
	CTYPE_STR,
};

struct element {
	int type;
	union {
		int num;
		char *str;
	} c;
};

static GSList* push_element(GSList *l, int num, char *str)
{
	struct element *elem = malloc(sizeof (struct element));

	if (num != -1)
	{
		elem->type = CTYPE_NUM;
		elem->c.num = num;
	}
	else
	{
		elem->type = CTYPE_STR;
		elem->c.str = strdup(str);
	}
	return g_slist_prepend(l, elem);
}

static char *pop_element(GSList **l)
{
	struct element *elem = (*l)->data;
	GSList *tmp;
	char *ret;

	if (elem->type == CTYPE_NUM)
		ret = strdup(mpg123_get_id3_genre(elem->c.num));
	else
		ret = elem->c.str;

	/* Remove link */
	tmp = *l;
	*l = g_slist_remove_link(*l, *l);
	g_slist_free_1(tmp);
	return ret;
}

static GSList* id3_get_content_v23(struct id3_frame *frame)
{
    char *input, *ptr;
	GSList *list = NULL;

	input = id3_string_decode(ID3_TEXT_FRAME_ENCODING(frame),
				  ID3_TEXT_FRAME_PTR(frame));

	if (!input)
		return NULL;

	ptr = input;
	while (ptr[0] == '(' && ptr[1] != '(')
	{
		if (!strchr(ptr, ')'))
			break;

		if (strncmp(ptr, "(RX)", 4) == 0)
		{
			list = push_element(list, -1, _("Remix"));
			ptr += 4;
		}
		else if (strncmp(ptr, "(CR)", 4) == 0)
		{
			list = push_element(list, -1, _("Cover"));
			ptr += 4;
		}
		else
		{
			/* Get ID3v1 genre number */
			int num;
			char *endptr;
			num = strtol(ptr + 1, &endptr, 10);
			if (*endptr == ')' && num >= 0 && num <= 255)
			{
				list = push_element(list, num, NULL);
				ptr = endptr + 1;
			}
			else if (*endptr == ')')
				ptr = endptr + 1;
			else
				break;
		}
	}

	if (*ptr == 0) {
		/* Unexpected end of ID */
		g_slist_free(list);
		free(input);
		return NULL;
	}

	/*
	 * Add plaintext refinement.
	 */
	if (strncmp(ptr, "((", 2))
		ptr++;
	list = push_element(list, -1, ptr);
	free(input);

	return list;
}

static GSList* id3_get_content_v24(struct id3_frame *frame)
{
	GSList *list = NULL;
	int offset = 0;
	
	while (offset < frame->fr_size - 1)
	{
		int num;
		char *input, *endptr;

		input = id3_string_decode(ID3_TEXT_FRAME_ENCODING(frame),
					  ID3_TEXT_FRAME_PTR(frame) + offset);

		if (!input)
			break;

		/* Get ID3v1 genre number */
		num = strtol(input, &endptr, 10);
		if (endptr && endptr != input && *endptr == '\0' &&
		    num >= 0 && num < 256)
			list = push_element(list, num, NULL);
		else if (!strcmp(input, "RX"))
			list = push_element(list, -1, _("Remix"));
		else if (!strcmp(input, "CR"))
			list = push_element(list, -1, _("Cover"));
		else
			list = push_element(list, -1, input);

		offset += id3_string_size(ID3_TEXT_FRAME_ENCODING(frame),
					  ID3_TEXT_FRAME_PTR(frame) + offset);
	}

	return list;
}

/*
 * Function id3_get_content (frame)
 *
 *    Expand content type string of frame and return it.  Return NULL
 *    upon error.
 *
 */
char *id3_get_content(struct id3_frame *frame)
{
	GSList *list;
	char **str_array, *ret;
	int len;

	if (frame->fr_desc->fd_id == ID3_TCON
        return NULL;

	/* Check if frame is compressed */
	if (id3_decompress_frame(frame) == -1)
		return NULL;

	if (frame->fr_owner->id3_version == 3)
		list = id3_get_content_v23(frame);
	else
		list = id3_get_content_v24(frame);

	len = g_slist_length(list);

	if (len == 0)
		return strdup("");
	
	str_array = calloc(sizeof (char *) * (len + 1), 1);

	while (len-- && list)
		str_array[len] = pop_element(&list);

	if (len != -1 || list)
		fprintf(stderr, "len: %d; list: %p", len, list);

	ret = g_strjoinv(", ", str_array);
	g_strfreev(str_array);

	return ret;
}

#endif