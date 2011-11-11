/*
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Mark Adler
  madler@alumni.caltech.edu
 */

#ifndef BUFFER_POOL_H_INCLUDED
#define BUFFER_POOL_H_INCLUDED

/* -- pool of spaces for buffer management -- */

/* These routines manage a pool of spaces.  Each pool specifies a fixed size
   buffer to be contained in each space.  Each space has a use count, which
   when decremented to zero returns the space to the pool.  If a space is
   requested from the pool and the pool is empty, a space is immediately
   created unless a specified limit on the number of spaces has been reached.
   Only if the limit is reached will it wait for a space to be returned to the
   pool.  Each space knows what pool it belongs to, so that it can be returned.
 */

#include "yarn.h"

/* a space (one buffer for each space) */
typedef struct buffer_pool_space_t 
{
    lock *use;              /* use count -- return to pool when zero */
    void *buf;              /* buffer of size pool->size */
    size_t len;             /* for application usage */
    struct buffer_pool_t *pool;      /* pool to return to */
    struct buffer_pool_space_t *next;     /* for pool linked list */
} buffer_pool_space_t;

/* pool of spaces (one pool for each size needed) */
typedef struct buffer_pool_t 
{
    lock *have;             /* unused spaces available, lock for list */
    buffer_pool_space_t *head;     /* linked list of available buffers */
    size_t size;            /* size of all buffers in this pool */
    int limit;              /* number of new spaces allowed, or -1 */
    int made;               /* number of buffers made */
} buffer_pool_t;

/* initialize a pool (pool structure itself provided, not allocated) -- the
   limit is the maximum number of spaces in the pool, or -1 to indicate no
   limit, i.e., to never wait for a buffer to return to the pool */
void buffer_pool_create(buffer_pool_t *pool, size_t size, int limit);

/* get a space from a pool -- the use count is initially set to one, so there
   is no need to call use_space() for the first use */
buffer_pool_space_t *buffer_pool_get_space(buffer_pool_t *pool);

/* increment the use count to require one more drop before returning this space
   to the pool */
void buffer_pool_use_space(buffer_pool_space_t *space);

/* drop a space, returning it to the pool if the use count is zero */
void buffer_pool_drop_space(buffer_pool_space_t *space);

/* free the memory and lock resources of a pool -- return number of spaces for
   debugging and resource usage measurement */
int buffer_pool_free(buffer_pool_t *pool);

#endif  /* BUFFER_POOL_H_INCLUDED */
