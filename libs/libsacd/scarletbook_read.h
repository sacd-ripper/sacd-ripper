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

#ifndef SCARLETBOOK_READ_H_INCLUDED
#define SCARLETBOOK_READ_H_INCLUDED

#include "scarletbook.h"
#include "sacd_reader.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * handle = scarletbook_open(sacd, title);
 *
 * Opens a scarletbook structure and reads in all the data.
 * Returns a handle to a completely parsed structure.
 */
scarletbook_handle_t *scarletbook_open(sacd_reader_t *, int);

/**
 * initialize scarletbook audio frames structs
 */
void scarletbook_frame_init(scarletbook_handle_t *handle);

/**
 * callback when a complete audio frame has been read
 */
typedef void (*frame_read_callback_t)(scarletbook_handle_t *handle, uint8_t* frame_data, size_t frame_size, void *userdata);

/**
 * processes scarletbook audio frames and does a callback in case it found a frame
 */
void scarletbook_process_frames(scarletbook_handle_t *, uint8_t *, int, int, frame_read_callback_t, void *);

/**
 * scarletbook_close(ifofile);
 * Cleans up the scarletbook information. This will free all data allocated for the
 * substructures.
 */
void scarletbook_close(scarletbook_handle_t *);

#ifdef __cplusplus
};
#endif
#endif /* SCARLETBOOK_READ_H_INCLUDED */
