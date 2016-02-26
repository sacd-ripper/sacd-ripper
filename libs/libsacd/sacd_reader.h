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

#ifndef SACD_READER_H_INCLUDED
#define SACD_READER_H_INCLUDED

#ifdef _MSC_VER

#include <stdio.h>
#include <stdlib.h>
#endif

#include <sys/types.h>
#include <inttypes.h>

#include "sacd_input.h"

/**
 * The SACD access interface.
 *
 * This file contains the functions that form the interface to audio tracks located on a SACD.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque type that is used as a handle for one instance of an opened SACD.
 */
typedef struct sacd_reader_s   sacd_reader_t;

/**
 * Opens a block device of a SACD-ROM file, or an image file.
 *
 * @param path Specifies the the device or file to be used.
 * @return If successful a a read handle is returned. Otherwise 0 is returned.
 *
 * sacd = sacd_open(path);
 */
sacd_reader_t *sacd_open(const char *);

/**
 * Closes and cleans up the SACD reader object.
 *
 * You must close all open files before calling this function.
 *
 * @param sacd A read handle that should be closed.
 *
 * sacd_close(sacd);
 */
void sacd_close(sacd_reader_t *);

/**
 * Seeks and reads a block from sacd.
 *
 * @param sacd A read handle that should be closed.
 * @param lb_number The block number to seek to.
 * @param block_count The amount of blocks to read.
 * @param data The data pointer to read the block into.
 *
 * sacd_read_block_raw(sacd, lb_number, block_count, data);
 */
ssize_t sacd_read_block_raw(sacd_reader_t *, uint32_t, size_t, unsigned char *);

/**
 * Decrypts audio sectors, only available on PS3
 */
int sacd_decrypt(sacd_reader_t *, uint8_t *, int);

/**
 * Authenticates disc, only available on PS3
 */
int sacd_authenticate(sacd_reader_t *);

/**
 * returns the total sector size of the image / disc
 */
uint32_t sacd_get_total_sectors(sacd_reader_t *);

#ifdef __cplusplus
};
#endif
#endif /* SACD_READER_H_INCLUDED */
