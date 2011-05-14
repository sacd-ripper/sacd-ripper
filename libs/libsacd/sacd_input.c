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
 
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <io.h>

#include "sacd_reader.h"
#include "scarletbook_types.h"
#include "sacd_input.h"

struct sacd_input_s {
  int fd;
};

/**
 * initialize and open a SACD device or file.
 */
sacd_input_t sacd_input_open(const char *target)
{
  sacd_input_t dev;

  /* Allocate the library structure */
  dev = (sacd_input_t) malloc(sizeof(*dev));
  if(dev == NULL) {
    fprintf(stderr, "libsacdread: Could not allocate memory.\n");
    return NULL;
  }

  /* Open the device */
#ifndef WIN32
  dev->fd = open(target, O_RDONLY);
#else
  dev->fd = open(target, O_RDONLY | O_BINARY);
#endif
  if(dev->fd < 0) {
    perror("libsacdread: Could not open input");
    free(dev);
    return NULL;
  }

  return dev;
}

/**
 * return the last error message
 */
char *sacd_input_error(sacd_input_t dev)
{
  /* use strerror(errno)? */
  return (char *)"unknown error";
}

/**
 * seek into the device.
 */
int sacd_input_seek(sacd_input_t dev, int blocks)
{
  off_t pos;

  pos = lseek(dev->fd, (off_t)blocks * (off_t)SACD_LSN_SIZE, SEEK_SET);
  if(pos < 0) {
      return pos;
  }
  /* assert pos % SACD_LSN_SIZE == 0 */
  return (int) (pos / SACD_LSN_SIZE);
}

/**
 * read data from the device.
 */
ssize_t sacd_input_read(sacd_input_t dev, int pos, int blocks, void *buffer)
{
  ssize_t ret, len;

  ret = lseek(dev->fd, (off_t)pos * (off_t)SACD_LSN_SIZE, SEEK_SET);
  if(ret < 0) {
      return 0;
  }

  len = (size_t)blocks * SACD_LSN_SIZE;

  while(len > 0) {

    ret = read(dev->fd, buffer, (unsigned int) len);

    if(ret < 0) {
      /* One of the reads failed, too bad.  We won't even bother
       * returning the reads that went OK, and as in the POSIX spec
       * the file position is left unspecified after a failure. */
      return ret;
    }

    if(ret == 0) {
      /* Nothing more to read.  Return all of the whole blocks, if any.
       * Adjust the file position back to the previous block boundary. */
      ssize_t bytes = (ssize_t)blocks * SACD_LSN_SIZE - len;
      off_t over_read = -(bytes % SACD_LSN_SIZE);
      /*off_t pos =*/ lseek(dev->fd, over_read, SEEK_CUR);
      /* should have pos % 2048 == 0 */
      return (int) (bytes / SACD_LSN_SIZE);
    }

    len -= ret;
  }

  return blocks;
}

/**
 * close the SACD device and clean up.
 */
int sacd_input_close(sacd_input_t dev)
{
  int ret;

  ret = close(dev->fd);

  if(ret < 0)
    return ret;

  free(dev);

  return 0;
}
