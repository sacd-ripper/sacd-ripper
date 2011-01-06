/**
 * Copyright (c) 2011 Mr Wicked, http://code.google.com/p/sacd-ripper/
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
 
#ifndef SACD_READER_H_INCLUDED
#define SACD_READER_H_INCLUDED

#ifdef _MSC_VER
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#endif

#include <sys/types.h>
#include <inttypes.h>

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
typedef struct sacd_reader_s sacd_reader_t;

/**
 * Opens a block device of a SACD-ROM file, or an image file.
 *
 * @param path Specifies the the device or file to be used.
 * @return If successful a a read handle is returned. Otherwise 0 is returned.
 *
 * sacd = sacd_open(path);
 */
sacd_reader_t *sacd_open( const char * );

/**
 * Closes and cleans up the SACD reader object.
 *
 * You must close all open files before calling this function.
 *
 * @param sacd A read handle that should be closed.
 *
 * sacd_close(sacd);
 */
void sacd_close( sacd_reader_t * );

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
ssize_t sacd_read_block_raw( sacd_reader_t *, uint32_t, size_t, unsigned char * );

#ifdef __cplusplus
};
#endif
#endif /* SACD_READER_H_INCLUDED */
