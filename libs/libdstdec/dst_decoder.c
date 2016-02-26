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

/*
  Although mostly unrecognizable; the parallelization code has been borrowed 
  from "pigz" (parallel zlib) made by Mark Adler, credits for the usage of 
  "yarn" and this threading code go to him!
*/

#include <stdlib.h>
#include <assert.h>
#ifndef __APPLE__
#include <malloc.h>
#endif
#include <pthread.h>
#include <string.h>
#ifdef __linux__
#include <sys/sysinfo.h>
#endif

#include <logging.h>

#include "dst_decoder.h"
#include "yarn.h"
#include "buffer_pool.h"
#include "dst_fram.h"
#include "dst_init.h"

#ifdef __APPLE__
#include <sys/sysctl.h>
#endif

/* -- parallel decoding -- */

/* decode or write job (passed from decode list to write list) -- if seq is
   equal to -1, decode_thread is instructed to return; if more is false then
   this is the last chunk, which after writing tells write_thread to return */
typedef struct job_t
{
    long seq;                                 /* sequence number */
    int error;                                /* an error code (eg. DST decoding error) */
    int more;                                 /* true if this is not the last chunk */
    buffer_pool_space_t *in;                  /* input DST data to decode */
    buffer_pool_space_t *out;                 /* resulting DSD decoded data */
    struct job_t *next;                       /* next job in the list (either list) */
} 
job_t;

struct dst_decoder_s
{
    int procs;            /* maximum number of compression threads (>= 1) */
    int channel_count;

    int sequence;       /* each job get's a unique sequence number */

    /* input and output buffer pools */
    buffer_pool_t in_pool;
    buffer_pool_t out_pool;

    /* list of decode jobs (with tail for appending to list) */
    lock *decode_have;   /* number of decode jobs waiting */
    job_t *decode_head, **decode_tail;

    /* list of write jobs */
    lock *write_first;    /* lowest sequence number in list */
    job_t *write_head;

    /* number of decoding threads running */
    int cthreads;

    /* write thread if running */
    thread *writeth;

    frame_decoded_callback_t frame_decoded_callback;
    frame_error_callback_t frame_error_callback;
    void *userdata;
};

static unsigned processor_count(void)
{
#if defined(_WIN32)
    return pthread_num_processors_np();
#elif defined(__linux__)
    return get_nprocs();
#elif defined(__APPLE__) || defined(__FreeBSD__)
    int count;
    size_t size=sizeof(count);
    return sysctlbyname("hw.ncpu",&count,&size,NULL,0) ? 1 : count;
#endif
}

/* setup job lists (call from main thread) */
static void setup_decoding_jobs(dst_decoder_t *dst_decoder)
{
    /* set up only if not already set up*/
    if (dst_decoder->decode_have != NULL)
        return;

    /* allocate locks and initialize lists */
    dst_decoder->decode_have = new_lock(0);
    dst_decoder->decode_head = NULL;
    dst_decoder->decode_tail = &dst_decoder->decode_head;
    dst_decoder->write_first = new_lock(-1);
    dst_decoder->write_head = NULL;

    /* initialize buffer pools */
    buffer_pool_create(&dst_decoder->in_pool, 64 * 1024, (dst_decoder->procs << 1) + 2);
    buffer_pool_create(&dst_decoder->out_pool, 64 * 1024, -1);
}

/* command the decode threads to all return, then join them all (call from
   main thread), free all the thread-related resources */
static void finish_decoding_jobs(dst_decoder_t *dst_decoder)
{
    job_t job;
    int caught;

    /* only do this once */
    if (dst_decoder->decode_have == NULL)
        return;

    /* command all of the extant decode threads to return */
    possess(dst_decoder->decode_have);
    job.error = 0;
    job.seq = -1;
    job.next = NULL;
    dst_decoder->decode_head = &job;
    dst_decoder->decode_tail = &(job.next);
    twist(dst_decoder->decode_have, BY, +1);       /* will wake them all up */

    /* join all of the decode threads, verify they all came back */
    caught = join_all();
    LOG(lm_main, LOG_NOTICE, ("-- joined %d decode threads", caught));
    assert(caught == dst_decoder->cthreads);
    dst_decoder->cthreads = 0;

    /* free the resources */
    caught = buffer_pool_free(&dst_decoder->out_pool);
    LOG(lm_main, LOG_NOTICE, ("-- freed %d output buffers", caught));
    caught = buffer_pool_free(&dst_decoder->in_pool);
    LOG(lm_main, LOG_NOTICE, ("-- freed %d input buffers", caught));
    free_lock(dst_decoder->write_first);
    free_lock(dst_decoder->decode_have);
    dst_decoder->decode_have = NULL;
}

/* get the next decoding job from the head of the list, decode and compute
   the check value on the input, and put a job in the write list with the
   results -- keep looking for more jobs, returning when a job is found with a
   sequence number of -1 (leave that job in the list for other incarnations to
   find) */
static void decode_thread(void *userdata)
{
    job_t *job;                /* job pulled and working on */ 
    job_t *here, **prior;      /* pointers for inserting in write list */ 
    ebunch      D;
    dst_decoder_t *dst_decoder = (dst_decoder_t *) userdata;

    if (DST_InitDecoder(&D, dst_decoder->channel_count, 64) != 0)
    {
        pthread_exit(0);
    }

    /* keep looking for work */
    for(;;)
    {
        /* get a job */
        possess(dst_decoder->decode_have);
        wait_for(dst_decoder->decode_have, NOT_TO_BE, 0);
        job = dst_decoder->decode_head;
        assert(job != NULL);
        if (job->seq == -1)
            break;
        dst_decoder->decode_head = job->next;
        if (job->next == NULL)
            dst_decoder->decode_tail = &dst_decoder->decode_head;
        twist(dst_decoder->decode_have, BY, -1);

        /* got a job */
        LOG(lm_main, LOG_NOTICE, ("-- decoding #%ld", job->seq));

        if (job->more)
        {
            job->out = buffer_pool_get_space(&dst_decoder->out_pool);

            /* Save the error for later, so that the write_thread can output them in DST frame order */
            job->error = DST_FramDSTDecode(job->in->buf, job->out->buf, job->in->len, job->seq, &D); 
            if (job->error != DSTErr_NoError)
                LOG(lm_main, LOG_ERROR, ("ERROR: %s on frame: %d", DST_GetErrorMessage(job->error), D.FrameHdr.FrameNr));

            job->out->len = (size_t)(MAX_DSDBITS_INFRAME / 8 * dst_decoder->channel_count);
            buffer_pool_drop_space(job->in);

            LOG(lm_main, LOG_NOTICE, ("-- decoded #%ld%s", job->seq, job->more ? "" : " (last)"));
        }

        /* insert write job in list in sorted order, alert write thread */
        possess(dst_decoder->write_first);
        prior = &dst_decoder->write_head;
        while ((here = *prior) != NULL) 
        {
            if (here->seq > job->seq)
                break;
            prior = &(here->next);
        }
        job->next = here;
        *prior = job;
        twist(dst_decoder->write_first, TO, dst_decoder->write_head->seq);

        /* done with that one -- go find another job */
    } 

    /* found job with seq == -1 -- free deflate memory and return to join */
    release(dst_decoder->decode_have);

    if (DST_CloseDecoder(&D) != 0)
    {
        pthread_exit(0);
    }
}

/* collect the write jobs off of the list in sequence order and write out the
   decoded data until the last chunk is written -- also write the header and
   trailer and combine the individual check values of the input buffers */
static void write_thread(void *userdata)
{
    long seq;                       /* next sequence number looking for */
    job_t *job;                     /* job pulled and working on */
    int more;                       /* true if more chunks to write */
    dst_decoder_t *dst_decoder = (dst_decoder_t *) userdata;

    /* build and write header */
    LOG(lm_main, LOG_NOTICE, ("-- write thread running"));

    /* process output of decode threads until end of input */
    seq = 0;
    do 
    {
        /* get next write job in order */
        possess(dst_decoder->write_first);
        wait_for(dst_decoder->write_first, TO_BE, seq);
        job = dst_decoder->write_head;
        dst_decoder->write_head = job->next;
        twist(dst_decoder->write_first, TO, dst_decoder->write_head == NULL ? -1 : dst_decoder->write_head->seq);

        /* report any error */
        if (job->error != 0 && dst_decoder->frame_error_callback)
            dst_decoder->frame_error_callback(job->seq, job->error, DST_GetErrorMessage(job->error), dst_decoder->userdata);

        more = job->more;

        if (more)
        {
            /* write the decoded data and drop the output buffer */
            dst_decoder->frame_decoded_callback(job->out->buf, job->out->len, dst_decoder->userdata);
            buffer_pool_drop_space(job->out);
        }

        free(job);

        /* get the next buffer in sequence */
        seq++;
    } 
    while (more);

    /* verify no more jobs, prepare for next use */
    possess(dst_decoder->decode_have);
    assert(dst_decoder->decode_head == NULL && peek_lock(dst_decoder->decode_have) == 0);
    release(dst_decoder->decode_have);
    possess(dst_decoder->write_first);
    assert(dst_decoder->write_head == NULL);
    twist(dst_decoder->write_first, TO, -1);
}

static void finish_write_job(dst_decoder_t *dst_decoder)
{
    job_t *job;                /* job for decode, then write */

    /* create a new job, use next input chunk, previous as dictionary */
    job = malloc(sizeof(job_t));
    if (job == NULL)
        exit(1);
    job->error = 0;
    job->seq = dst_decoder->sequence;
    job->in = 0;
    job->out = 0;
    job->more = 0;

    ++dst_decoder->sequence;

    /* start another decode thread if needed */
    if (dst_decoder->cthreads < dst_decoder->procs) 
    {
        (void)launch(decode_thread, dst_decoder);
        dst_decoder->cthreads++;
    }

    /* put job at end of decode list, let all the decoders know */
    possess(dst_decoder->decode_have);
    job->next = NULL;
    *dst_decoder->decode_tail = job;
    dst_decoder->decode_tail = &(job->next);
    twist(dst_decoder->decode_have, BY, +1);

    join(dst_decoder->writeth);
    dst_decoder->writeth = NULL;
}

dst_decoder_t* dst_decoder_create(int channel_count, frame_decoded_callback_t frame_decoded_callback, frame_error_callback_t frame_error_callback, void *userdata)
{
    dst_decoder_t *dst_decoder = (dst_decoder_t*) calloc(sizeof(dst_decoder_t), 1);

    if (!dst_decoder)
        exit(1);

    assert(frame_decoded_callback);

    dst_decoder->channel_count = channel_count;
    dst_decoder->userdata = userdata;
    dst_decoder->frame_decoded_callback = frame_decoded_callback;
    dst_decoder->frame_error_callback = frame_error_callback;
    dst_decoder->procs = processor_count();

    /* if first time or after an option change, setup the job lists */
    setup_decoding_jobs(dst_decoder);

    /* start write thread */
    dst_decoder->writeth = launch(write_thread, dst_decoder);

    return dst_decoder;
}

void dst_decoder_destroy(dst_decoder_t *dst_decoder)
{
    finish_write_job(dst_decoder);
    finish_decoding_jobs(dst_decoder);

    free(dst_decoder);
}

void dst_decoder_decode(dst_decoder_t *dst_decoder, uint8_t* frame_data, size_t frame_size)
{
    job_t *job;                /* job for decode, then write */

    /* create a new job, use next input chunk */
    job = malloc(sizeof(job_t));
    if (job == NULL)
        exit(1);
    job->error = 0;
    job->seq = dst_decoder->sequence;
    job->in = buffer_pool_get_space(&dst_decoder->in_pool);
    memcpy(job->in->buf, frame_data, frame_size);
    job->in->len = frame_size;
    job->out = NULL;
    job->more = 1;

    ++dst_decoder->sequence;

    /* start another decode thread if needed */
    if (dst_decoder->cthreads < dst_decoder->procs) 
    {
        (void)launch(decode_thread, dst_decoder);
        dst_decoder->cthreads++;
    }

    /* put job at end of decode list, let all the decoders know */
    possess(dst_decoder->decode_have);
    job->next = NULL;
    *dst_decoder->decode_tail = job;
    dst_decoder->decode_tail = &(job->next);
    twist(dst_decoder->decode_have, BY, +1);
}
