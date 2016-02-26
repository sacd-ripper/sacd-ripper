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

#include <stdio.h>
#include <stdlib.h>

#ifdef __lv2ppu__
#include <sys/file.h>
#elif defined(WIN32)
#include <io.h>
#endif

#include "scarletbook_output.h"

static size_t iso_write_frame(scarletbook_output_format_t *ft, const uint8_t *buf, size_t len)
{
    return fwrite(buf, 1, len * SACD_LSN_SIZE, ft->fd);
}

scarletbook_format_handler_t const * iso_format_fn(void) 
{
    static scarletbook_format_handler_t handler = 
    {
        "ISO Image", 
        "iso", 
        0, 
        iso_write_frame,
        0, 
        OUTPUT_FLAG_RAW,
        0
    };
    return &handler;
}
