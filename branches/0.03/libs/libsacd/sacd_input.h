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

#ifndef SACD_INPUT_H_INCLUDED
#define SACD_INPUT_H_INCLUDED

#if defined(__MINGW32__)
#   undef  lseek
#   define lseek            _lseeki64
#   undef  fseeko
#   define fseeko           fseeko64
#   undef  ftello
#   define ftello           ftello64
#   define flockfile(...)
#   define funlockfile(...)
#   define getc_unlocked    getc
#   undef  off_t
#   define off_t            off64_t
#   undef  stat
#   define stat             _stati64
#   define fstat            _fstati64
#   define wstat            _wstati64
#endif

#if defined(__lv2ppu__)
#include <sys/io_buffer.h>
#endif

typedef struct sacd_input_s * sacd_input_t;

extern sacd_input_t sacd_input_open(const char *);
extern int     sacd_input_close(sacd_input_t);
extern ssize_t sacd_input_read(sacd_input_t, int, int, void *);
extern char    * sacd_input_error(sacd_input_t);
#if defined(__lv2ppu__)
inline int sacd_input_async_read(sacd_input_t, int, int, sys_io_block_t, uint64_t);
#endif
inline int sacd_input_get_fd(sacd_input_t);

#endif /* SACD_INPUT_H_INCLUDED */
