/**
 * SACD Ripper - https://github.com/sacd-ripper/
 *
 * Copyright (c) 2010-2015 by respective authors.
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

#ifndef SCARLETBOOK_HELPERS_H_INCLUDED
#define SCARLETBOOK_HELPERS_H_INCLUDED

#include "scarletbook.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * helper functions..
 */
static inline int has_two_channel(scarletbook_handle_t *handle)
{
    return handle->twoch_area_idx != -1;
}

static inline int has_multi_channel(scarletbook_handle_t *handle)
{
    return handle->mulch_area_idx != -1;
}

static inline int has_both_channels(scarletbook_handle_t *handle)
{
    return handle->twoch_area_idx != -1 && handle->mulch_area_idx != -1;
}

static inline area_toc_t* get_two_channel(scarletbook_handle_t *handle)
{
    return(handle->twoch_area_idx == -1 ? 0 : handle->area[handle->twoch_area_idx].area_toc);
}

static inline area_toc_t* get_multi_channel(scarletbook_handle_t *handle)
{
    return(handle->mulch_area_idx == -1 ? 0 : handle->area[handle->mulch_area_idx].area_toc);
}

char *get_speaker_config_string(area_toc_t *);

char *get_frame_format_string(area_toc_t *);

char *get_album_dir(scarletbook_handle_t *);

char *get_music_filename(scarletbook_handle_t *, int, int, const char *);

#ifdef __cplusplus
};
#endif
#endif /* SCARLETBOOK_HELPERS_H_INCLUDED */
