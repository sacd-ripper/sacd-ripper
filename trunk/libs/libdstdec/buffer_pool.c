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

#include <assert.h>
#include <stdlib.h>
#ifndef __APPLE__
#include <malloc.h>
#endif

#include "buffer_pool.h"

/* initialize a pool (pool structure itself provided, not allocated) -- the
   limit is the maximum number of spaces in the pool, or -1 to indicate no
   limit, i.e., to never wait for a buffer to return to the pool */
void buffer_pool_create(buffer_pool_t *pool, size_t size, int limit)
{
    pool->have = new_lock(0);
    pool->head = NULL;
    pool->size = size;
    pool->limit = limit;
    pool->made = 0;
}

/* get a space from a pool -- the use count is initially set to one, so there
   is no need to call use_space() for the first use */
buffer_pool_space_t *buffer_pool_get_space(buffer_pool_t *pool)
{
    buffer_pool_space_t *space;

    /* if can't create any more, wait for a space to show up */
    possess(pool->have);
    if (pool->limit == 0)
        wait_for(pool->have, NOT_TO_BE, 0);

    /* if a space is available, pull it from the list and return it */
    if (pool->head != NULL) 
        {
        space = pool->head;
        possess(space->use);
        pool->head = space->next;
        twist(pool->have, BY, -1);      /* one less in pool */
        twist(space->use, TO, 1);       /* initially one user */
        return space;
    }

    /* nothing available, don't want to wait, make a new space */
    assert(pool->limit != 0);
    if (pool->limit > 0)
        pool->limit--;
    pool->made++;
    release(pool->have);
    space = malloc(sizeof(buffer_pool_space_t));
    if (space == NULL)
        return 0;
    space->use = new_lock(1);           /* initially one user */
#ifdef _WIN32
    space->buf = _aligned_malloc(pool->size, 64);
#elif __APPLE__
    posix_memalign(&space->buf, 64, pool->size);
#else
    space->buf = memalign(64, pool->size);
#endif
    if (space->buf == NULL)
        return 0;
    space->pool = pool;                 /* remember the pool this belongs to */
    return space;
}

/* increment the use count to require one more drop before returning this space
   to the pool */
void buffer_pool_use_space(buffer_pool_space_t *space)
{
    possess(space->use);
    twist(space->use, BY, +1);
}

/* drop a space, returning it to the pool if the use count is zero */
void buffer_pool_drop_space(buffer_pool_space_t *space)
{
    int use;
    buffer_pool_t *pool;

    possess(space->use);
    use = peek_lock(space->use);
    assert(use != 0);
    if (use == 1) 
    {
        pool = space->pool;
        possess(pool->have);
        space->next = pool->head;
        pool->head = space;
        twist(pool->have, BY, +1);
    }
    twist(space->use, BY, -1);
}

/* free the memory and lock resources of a pool -- return number of spaces for
   debugging and resource usage measurement */
int buffer_pool_free(buffer_pool_t *pool)
{
    int count;
    buffer_pool_space_t *space;

    possess(pool->have);
    count = 0;
    while ((space = pool->head) != NULL) 
    {
        pool->head = space->next;
#ifdef _WIN32
        _aligned_free(space->buf);
#else
        free(space->buf);
#endif
        free_lock(space->use);
        free(space);
        count++;
    }
    release(pool->have);
    free_lock(pool->have);
    assert(count == pool->made);
    return count;
}
