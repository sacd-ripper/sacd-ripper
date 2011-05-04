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

#include <time.h>
#include <stdio.h>
#include <stdlib.h> 
#include <errno.h>
#include <string.h>

#include "sac_accessor.h"
#include "utils.h"

static void *spe_thread_proc(void *arg) {
    sac_accessor_t *sa = (sac_accessor_t *) arg;
    spe_context_ptr_t spe = sa->spe;
    unsigned int entry = SPE_DEFAULT_ENTRY;
    spe_stop_info_t stop_info;
    spe_program_handle_t * program;
    int ret;

    fprintf(stdout, "spe_thread_proc::spe_image_open: " SAC_MODULE_LOCATION "\n");
    program = spe_image_open(SAC_MODULE_LOCATION);
    if (!program) {
        fprintf(stderr, "spe_thread_proc::spe_image_open\n");
        exit(-1);
    }

    fprintf(stdout, "spe_thread_proc::spe_program_load\n");
    if (spe_program_load(spe, program)) {
        fprintf(stderr, "spe_thread_proc::spe_program_load: %s\n", strerror(errno));
        exit(-1);
    }

    fprintf(stdout, "spe_thread_proc::spe_context_run\n");
    ret = spe_context_run(spe, &entry, 0, NULL, NULL, &stop_info);
    if (ret == 0) {
        fprintf(stdout, "spe_thread_proc::spe_context_run: stop with exit code: %x\n", stop_info.result.spe_exit_code);
    } else if (ret > 0) {
        fprintf(stderr, "[ERR] spe_thread_proc::spe_context_run: Unexpected stop and signal, code: %d\n", stop_info.stop_reason);
        exit(-1);
    } else {
        fprintf(stderr, "[ERR] spe_thread_proc::spe_context_run: %s, code: %d\n", strerror(errno), stop_info.stop_reason);
    }

    fprintf(stdout, "exiting spe thread..\n");

    pthread_exit(NULL);
}

static void *ppe_thread_proc(void *arg) {
    sac_accessor_t* sa = (sac_accessor_t *) arg;
    int ret;
    int i;

    fprintf(stdout, "ppe_thread_proc::ppe_thread_proc\n");

    /* event loop */
    while (1) {
        /* wait for the next event */
        ret = spe_event_wait(sa->evhandler, &sa->event, 1, -1);
        if (ret == -1) {
            fprintf(stderr, "spe_event_wait: %s\n", strerror(errno));
            exit(-1);
        } else if (ret == 0) {
            fprintf(stderr, "spe_event_wait: Unexpected timeout.\n");
            exit(-1);
        }

        fprintf(stdout, "received event %u: spe[%u]\n", ret, (unsigned int) sa->event.data.u32);
        if (sa->event.events & SPE_EVENT_OUT_INTR_MBOX) {

            fprintf(stdout, "handle_interrupt [%x]\n", (uintptr_t) arg);

            ret = spe_out_intr_mbox_read(sa->spe, &sa->read_count, 1, SPE_MBOX_ANY_NONBLOCKING);
            if (ret == 0) {
                fprintf(stderr, "spe_out_intr_mbox_read failed %d\n", ret);
                pthread_mutex_unlock(&sa->mmio_mutex);
                exit(-1);
            }
            fprintf(stdout, "spe_out_intr_mbox_read returned [%x]\n", sa->read_count);

            fprintf(stdout, "pthread_mutex_lock\n");
            ret = pthread_mutex_lock(&sa->mmio_mutex);
            if (ret != 0) {
                fprintf(stderr, "pthread_mutex_lock(&) failed. (%d)\n", ret);
                exit(-1);
            }

            fprintf(stdout, "pthread_cond_signal\n");
            ret = pthread_cond_signal(&sa->mmio_cond);
            if (ret != 0) {
                pthread_mutex_unlock(&sa->mmio_mutex);
                exit(-1);
            }

            fprintf(stdout, "pthread_mutex_unlock\n");
            ret = pthread_mutex_unlock(&sa->mmio_mutex);
            if (ret != 0) {
                exit(-1);
            }
        } else if (sa->event.events & SPE_EVENT_SPE_STOPPED) {

            fprintf(stdout, "received stopped event, exiting ppe thread..\n");
            break;
        }
    }

    pthread_exit(NULL);
}

sac_accessor_t *create_sac_accessor(void) {
    sac_accessor_t *sa;
    int ret = 0;

    sa = calloc(1, sizeof (sac_accessor_t));
    if (sa == NULL) {
        fprintf(stderr, "sac_accessor_t calloc failed\n");
        return 0;
    }

    sa->buffer = (uint8_t *) memalign(128, DMA_SIZE);
    if (sa->buffer == NULL) {
        fprintf(stderr, "memalign failed\n");
        return 0;
    }

    if (pthread_mutex_init(&sa->mmio_mutex, NULL) != 0) {
        fprintf(stderr, "create mmio_mutex failed.\n");
        return 0;
    }

    if (pthread_cond_init(&sa->mmio_cond, NULL) != 0) {
        fprintf(stderr, "create mmio_cond failed.\n");
        return 0;
    }

    fprintf(stdout, "spe_context_create\n");
    sa->spe = spe_context_create(SPE_EVENTS_ENABLE, NULL);
    if (!sa->spe) {
        fprintf(stderr, "spe_context_create: %s\n", strerror(errno));
        return 0;
    }

    /* register events */
    fprintf(stdout, "spe_event_handler_create\n");
    sa->evhandler = spe_event_handler_create();
    if (!sa->evhandler) {
        fprintf(stderr, "spe_event_handler_create: %s\n", strerror(errno));
        return 0;
    }

    sa->event.events = SPE_EVENT_OUT_INTR_MBOX | SPE_EVENT_SPE_STOPPED;
    sa->event.spe = sa->spe;
    fprintf(stdout, "spe_event_handler_register\n");
    ret = spe_event_handler_register(sa->evhandler, &sa->event);
    if (ret == -1) {
        fprintf(stderr, "spe_event_handler_register: %s\n", strerror(errno));
        return 0;
    }

    ret = pthread_create(&sa->ppe_tid, NULL, ppe_thread_proc, sa);
    if (ret) {
        fprintf(stderr, "ppe pthread_create: %s\n", strerror(ret));
        return 0;
    }

    ret = pthread_create(&sa->spe_tid, NULL, spe_thread_proc, sa);
    if (ret) {
        fprintf(stderr, "spe pthread_create: %s\n", strerror(ret));
        return 0;
    }

    fprintf(stdout, "spe_in_mbox_write\n");
    spe_in_mbox_write(sa->spe, (unsigned int *) &sa->buffer, 1, SPE_MBOX_ALL_BLOCKING);
    EIEIO;

    return sa;
}

int destroy_sac_accessor(sac_accessor_t *sa) {
    int ret = 0;

    fprintf(stdout, "destroy_sac_accessor\n");

    if (sa == NULL) {
        return -1;
    }

    if (sa->spe != NULL) {
        ret = spe_in_mbox_status(sa->spe);
        EIEIO;

        fprintf(stdout, "spe_in_mbox_status 0x%x\n", ret);

        if (ret & 1) {
            unsigned int cmd = EXIT_SAC_CMD;
            fprintf(stdout, "spe_in_mbox_write 0x%x\n", EXIT_SAC_CMD);
            spe_in_mbox_write(sa->spe, &cmd, 1, SPE_MBOX_ALL_BLOCKING);
        }
    }

    fprintf(stdout, "pthread_join(sa->spe_tid, NULL)\n");
    if (sa->spe_tid) {
        pthread_join(sa->spe_tid, NULL);
        sa->spe_tid = 0;
    }

    ret = spe_context_destroy(sa->spe);
    if (ret) {
        fprintf(stderr, "spe_context_destroy: %s\n", strerror(errno));
        return -1;
    }

    ret = spe_event_handler_destroy(sa->evhandler);
    if (ret) {
        return -1;
    }

    fprintf(stdout, "pthread_join(sa->ppe_tid, NULL)\n");
    if (sa->ppe_tid) {
        pthread_join(sa->ppe_tid, NULL);
        sa->ppe_tid = 0;
    }

    if (sa->buffer) {
        free(sa->buffer);
        sa->buffer = 0;
    }

    if ((ret = pthread_cond_destroy(&sa->mmio_cond)) != 0) {
        fprintf(stderr, "destroy mmio_cond failed.\n");
    }

    if ((ret = pthread_mutex_destroy(&sa->mmio_mutex)) != 0) {
        fprintf(stderr, "destroy mmio_mutex failed.\n");
    }

    free(sa);

    return ret;
}

int exchange_data(sac_accessor_t *sa, int func_nr
        , uint8_t *write_buffer, int write_count
        , uint8_t *read_buffer, int read_count
        , int timeout) {

    struct timespec ts;
    int ret;

    if (sa == NULL) {
        return -1;
    }

    sa->read_count = 0xfffffff1;

    if (write_count > 0) {
        memcpy(sa->buffer, write_buffer, min(write_count, DMA_SIZE));
    }

    fprintf(stdout, "exchange_data: [pthread_mutex_lock]\n");
    ret = pthread_mutex_lock(&sa->mmio_mutex);
    if (ret != 0) {
        fprintf(stderr, "pthread_mutex_lock(&) failed. (%d)\n", ret);
        return ret;
    }

    fprintf(stdout, "exchange_data: [writing func %d for amount %d..]\n", func_nr, read_count);
    spe_in_mbox_write(sa->spe, &func_nr, 1, SPE_MBOX_ALL_BLOCKING);
    spe_in_mbox_write(sa->spe, &read_count, 1, SPE_MBOX_ALL_BLOCKING);

    fprintf(stdout, "exchange_data: [pthread_cond_wait]\n");
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout;
    ret = pthread_cond_timedwait(&sa->mmio_cond, &sa->mmio_mutex, &ts);
    if (ret != 0) {
        fprintf(stderr, "exchange_data: [pthread_cond_timedwait timeout]\n");
        pthread_mutex_unlock(&sa->mmio_mutex);
        return ret;
    }

    fprintf(stdout, "exchange_data: [pthread_mutex_unlock]\n");
    ret = pthread_mutex_unlock(&sa->mmio_mutex);
    if (ret != 0) {
        return ret;
    }

    if (sa->read_count == 0xfffffff1) {
        return -1;
    }

    if (read_buffer != NULL && read_count > 0) {
        memcpy(read_buffer, sa->buffer, min(read_count, DMA_SIZE));
    }

    return read_count;
}
