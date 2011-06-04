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
#include "version.h"

#ifndef NO_ID3

#include "id3tag.h"

static void update_id3_frame(struct id3_tag *tag, const char *frame_name, const char *data)
{
    int              res;
    struct id3_frame *frame;
    union id3_field  *field;
    id3_ucs4_t       *ucs4;

    if (data == NULL)
        return;

    /*
     * An empty string removes the frame altogether.
     */
    if (strlen(data) == 0)
    {
        while ((frame = id3_tag_findframe(tag, frame_name, 0)))
            id3_tag_detachframe(tag, frame);
        return;
    }

    frame = id3_tag_findframe(tag, frame_name, 0);
    if (!frame)
    {
        frame = id3_frame_new(frame_name);
        id3_tag_attachframe(tag, frame);
    }

    if (strcmp(frame_name, ID3_FRAME_COMMENT) == 0)
    {
        field       = id3_frame_field(frame, 3);
        field->type = ID3_FIELD_TYPE_STRINGFULL;
    }
    else
    {
        field       = id3_frame_field(frame, 1);
        field->type = ID3_FIELD_TYPE_STRINGLIST;
    }

    ucs4 = id3_latin1_ucs4duplicate((const id3_latin1_t *) data);

    if (strcmp(frame_name, ID3_FRAME_COMMENT) == 0)
    {
        res = id3_field_setfullstring(field, ucs4);
    }
    else
    {
        res = id3_field_setstrings(field, 1, &ucs4);
    }

    if (res != 0)
    {
        fprintf(stderr, "error setting id3 field: %s\n", frame_name);
    }
}
#endif

int scarletbook_id3_tag_render(scarletbook_handle_t *handle, uint8_t *buffer, int area, int track)
{
#ifndef NO_ID3
    const int      sacd_id3_genres[] = {
        12,  12,  40, 12, 32, 140,  2,  3,
        98,  12,  80, 38,  7,   8, 86, 77,
        10, 103, 104, 13, 15,  16, 17, 14,
        37,  24, 101, 12,  0,  12, 12, 12
    };
    struct id3_tag *id3tag;
    char           tmp[200];
    int            len;

    id3tag = id3_tag_new();
    id3_tag_clearframes(id3tag);
    id3tag->options |= ID3_TAG_OPTION_COMPRESSION;

    memset(tmp, 0, sizeof(tmp));

    if (handle->area[area].area_track_text->track_type_title)
        update_id3_frame(id3tag, ID3_FRAME_TITLE, handle->area[area].area_track_text->track_type_title);
    if (handle->area[area].area_track_text->track_type_performer)
        update_id3_frame(id3tag, ID3_FRAME_ARTIST, handle->area[area].area_track_text->track_type_performer);
    if (handle->master_text[0]->album_title_position)
    {
        snprintf(tmp, 200, "%s", (char *) handle->master_text[0] + handle->master_text[0]->album_title_position);
        update_id3_frame(id3tag, ID3_FRAME_ALBUM, tmp);
    }
    update_id3_frame(id3tag, ID3_FRAME_COMMENT, SACD_RIPPER_VERSION);

    snprintf(tmp, 200, "%04d", handle->master_toc->disc_date_year);
    update_id3_frame(id3tag, ID3_FRAME_YEAR, tmp);

    snprintf(tmp, 200, "%d", track + 1);     // internally tracks start from 0
    update_id3_frame(id3tag, ID3_FRAME_TRACK, tmp);

    snprintf(tmp, 200, "%d", sacd_id3_genres[handle->area[area].area_isrc_genre->track_genre[track].genre & 0x1f]);
    update_id3_frame(id3tag, ID3_FRAME_GENRE, tmp);

    len = id3_tag_render(id3tag, buffer);

    id3_tag_delete(id3tag);

    return len;
#else
    return 0;
#endif
}
