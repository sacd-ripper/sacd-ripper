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

#define _FILE_OFFSET_BITS 64    /* large file support - linux */
#ifndef SACD_INPUT_H_INCLUDED
#define SACD_INPUT_H_INCLUDED

#include <inttypes.h>


//  #if defined(__MINGW32__) || defined(_WIN32)
//  #   undef  lseek
//  #   define lseek            _lseeki64
//  #   undef  fseeko
//  #   define fseeko           fseeko64
//  #   undef  ftello
//  #   define ftello           ftello64
//  #   define flockfile(...)
//  #   define funlockfile(...)
//  #   define getc_unlocked    getc
//  #   undef  off_t
//  #   define off_t            __int64
//  //#   define off_t            uint64_t
//  #   undef  stat
//  #   define stat             _stati64
//  #   define fstat            _fstati64
//  #   define wstat            _wstati64
//  #endif


//64 bit file access is the natural file access type for Cygwin. off_t is 8 bytes. 
//There are no foo64 functions for that reason. 
//Just use fopen and friends and you get 64 bit file access for free.

#if defined(__lv2ppu__)
#include <sys/io_buffer.h>
#endif

// for linux it is already added preprocessor coomand in cmake text 
//   -D_FILE_OFFSET_BITS=64
// #define _FILE_OFFSET_BITS 64

typedef struct sacd_input_s * sacd_input_t;

extern sacd_input_t (*sacd_input_open)         (const char *);
extern int          (*sacd_input_close)        (sacd_input_t);
extern uint32_t     (*sacd_input_read)         (sacd_input_t, uint32_t, uint32_t, void *);
extern char *       (*sacd_input_error)        (sacd_input_t);
extern int          (*sacd_input_authenticate) (sacd_input_t);
extern int          (*sacd_input_decrypt)      (sacd_input_t, uint8_t *, uint32_t);
extern uint32_t     (*sacd_input_total_sectors)(sacd_input_t);

int sacd_input_setup(const char *); 

#endif /* SACD_INPUT_H_INCLUDED */
