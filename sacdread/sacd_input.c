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
 
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

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
ssize_t sacd_input_read(sacd_input_t dev, void *buffer, int blocks)
{
  ssize_t ret, len;

  len = (size_t)blocks * SACD_LSN_SIZE;

  while(len > 0) {

    ret = read(dev->fd, buffer, len);

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
