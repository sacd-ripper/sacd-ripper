/* ***** BEGIN LICENSE BLOCK *****
* Version: MPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Mozilla Public License Version
* 1.1 (the "License"); you may not use this file except in compliance with
* the License. You may obtain a copy of the License at
* http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* The Original Code is the Netscape Portable Runtime (NSPR).
*
* The Initial Developer of the Original Code is
* Netscape Communications Corporation.
* Portions created by the Initial Developer are Copyright (C) 1998-2000
* the Initial Developer. All Rights Reserved.
*
* Contributor(s):
*
* Alternatively, the contents of this file may be used under the terms of
* either the GNU General Public License Version 2 or later (the "GPL"), or
* the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
* in which case the provisions of the GPL or the LGPL are applicable instead
* of those above. If you wish to allow use of your version of this file only
* under the terms of either the GPL or the LGPL, and not to allow others to
* use your version of this file under the terms of the MPL, indicate your
* decision by deleting the provisions above and replace them with the notice
* and other provisions required by the GPL or the LGPL. If you do not delete
* the provisions above, a recipient may use your version of this file under
* the terms of any one of the MPL, the GPL or the LGPL.
*
* ***** END LICENSE BLOCK ***** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>

#ifdef __lv2ppu__
#include <sys/mutex.h>
#include <sys/thread.h>
#include <sys/stat.h>
#include <sys/file.h>
#endif

#include "log.h"

#ifdef __lv2ppu__
static sys_mutex_t _log_lock;
#define _LOCK_LOG() sysMutexLock(_log_lock, 0)
#define _UNLOCK_LOG() sysMutexUnlock(_log_lock)
#else
#define _LOCK_LOG()
#define _UNLOCK_LOG()
#endif

#define _PUT_LOG(fd, buf, nb)    { fwrite(buf, 1, nb, fd); fflush(fd); }

static log_module_info_t *logModules;

static char            *log_buf = NULL;
static char            *logp;
static char            *log_endp;
static FILE            *log_file        = NULL;
static int             output_time_stamp = 0;

#define LINE_BUF_SIZE       512
#define DEFAULT_BUF_SIZE    16384

void log_init(void)
{
    char             *ev = 0;

#ifdef __lv2ppu__
    sys_mutex_attr_t mutex_attr;
    memset(&mutex_attr, 0, sizeof(sys_mutex_attr_t));
    mutex_attr.attr_protocol  = SYS_MUTEX_PROTOCOL_PRIO;
    mutex_attr.attr_recursive = SYS_MUTEX_ATTR_NOT_RECURSIVE;
    mutex_attr.attr_pshared   = SYS_MUTEX_ATTR_PSHARED;
    mutex_attr.attr_adaptive  = SYS_MUTEX_ATTR_NOT_ADAPTIVE;
    sysMutexCreate(&_log_lock, &mutex_attr);
#endif

    ev = getenv("LOG_MODULES");
    if (ev && ev[0])
    {
        char module[64];  /* Security-Critical: If you change this
                           * size, you must also change the sscanf
                           * format string to be size-1.
                           */
        int     is_sync  = 0;
        int     evlen   = strlen(ev), pos = 0;
        int32_t bufSize = DEFAULT_BUF_SIZE;
        while (pos < evlen)
        {
            int level = 1, count = 0, delta = 0;
            count = sscanf(&ev[pos], "%63[ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-]%n:%d%n",
                           module, &delta, &level, &delta);
            pos += delta;
            if (count == 0)
                break;

            /*
            ** If count == 2, then we got module and level. If count
            ** == 1, then level defaults to 1 (module enabled).
            */
            if (strcasecmp(module, "sync") == 0)
            {
                is_sync = 1;
            }
            else if (strcasecmp(module, "bufsize") == 0)
            {
                if (level >= LINE_BUF_SIZE)
                {
                    bufSize = level;
                }
            }
            else if (strcasecmp(module, "timestamp") == 0)
            {
                output_time_stamp = 1;
            }
            else
            {
                log_module_info_t *lm = logModules;
                int skip_modcheck = (0 == strcasecmp(module, "all")) ? 1 : 0;

                while (lm != NULL)
                {
                    if (skip_modcheck)
                        lm->level = (log_module_level_t) level;
                    else if (strcasecmp(module, lm->name) == 0)
                    {
                        lm->level = (log_module_level_t) level;
                        break;
                    }
                    lm = lm->next;
                }
            }
            /*found:*/
            count = sscanf(&ev[pos], " , %n", &delta);
            pos  += delta;
            if (count == EOF)
                break;
        }
        set_log_buffering(is_sync ? 0 : bufSize);

        ev = getenv("LOG_FILE");
        if (ev && ev[0])
        {
            if (set_log_file(ev) != 0)
            {
                fprintf(stderr, "Unable to create log file '%s'\n", ev);
            }
        }
        else
        {
            log_file = stderr;
        }
    }
}

void log_destroy(void)
{
    log_module_info_t *lm = logModules;

    log_flush();

    if (log_file && log_file != stdout && log_file != stderr)
    {
        fclose(log_file);
    }
    log_file = NULL;

    if (log_buf)
        free(log_buf);

    while (lm != NULL)
    {
        log_module_info_t *next = lm->next;
        free((/*const*/ char *) lm->name);
        free(lm);
        lm = next;
    }
    logModules = NULL;

#ifdef __lv2ppu__
    sysMutexDestroy(_log_lock);
#endif
}

static void set_log_module_level(log_module_info_t *lm)
{
    char *ev;

    ev = getenv("LOG_MODULES");
    if (ev && ev[0])
    {
        char module[64];  /* Security-Critical: If you change this
                           * size, you must also change the sscanf
                           * format string to be size-1.
                           */
        int evlen = strlen(ev), pos = 0;
        while (pos < evlen)
        {
            int level = 1, count = 0, delta = 0;

            count = sscanf(&ev[pos], "%63[ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-]%n:%d%n",
                           module, &delta, &level, &delta);
            pos += delta;
            if (count == 0)
                break;

            /*
            ** If count == 2, then we got module and level. If count
            ** == 1, then level defaults to 1 (module enabled).
            */
            if (lm != NULL)
            {
                if ((strcasecmp(module, "all") == 0)
                    || (strcasecmp(module, lm->name) == 0))
                {
                    lm->level = (log_module_level_t) level;
                }
            }
            count = sscanf(&ev[pos], " , %n", &delta);
            pos  += delta;
            if (count == EOF)
                break;
        }
    }
}

log_module_info_t* create_log_module(const char *name)
{
    log_module_info_t *lm;

    lm = (log_module_info_t *) malloc(sizeof(log_module_info_t));
    if (lm)
    {
        lm->name   = strdup(name);
        lm->level  = LOG_NONE;
        lm->next   = logModules;
        logModules = lm;
        set_log_module_level(lm);
    }
    return lm;
}

int set_log_file(const char *file)
{
    FILE *new_log_file;

    _LOCK_LOG();
    new_log_file = fopen(file, "w");
    if (!new_log_file) {
        _UNLOCK_LOG();
        return -1;
    }
#ifdef __lv2ppu__
    sysFsChmod(file, S_IFMT | 0777); 
#endif

    if (log_file && log_file != stdout && log_file != stderr)
    {
        fclose(log_file);
    }
    log_file = new_log_file;

    _UNLOCK_LOG();
    return 0;
}

void set_log_buffering(int buffer_size)
{
    log_flush();

    if (log_buf)
        free(log_buf);

    if (buffer_size >= LINE_BUF_SIZE)
    {
        logp    = log_buf = (char *) malloc(buffer_size);
        log_endp = logp + buffer_size;
    }
}

void log_print(const char *fmt, ...)
{
    va_list          ap;
    char             line[LINE_BUF_SIZE];
    char             *line_long = NULL;
    uint32_t         nb_tid     = 0, nb;
#ifdef __lv2ppu__
    sys_ppu_thread_t me        = 0;
#else
    int              me     = 0;
#endif
    time_t           now;
    struct tm        ts;

    if (!log_file)
    {
        return;
    }

    if (output_time_stamp)
    {
        time(&now);
        ts = *localtime(&now);

        nb_tid = snprintf(line, sizeof(line) - 1,
                          "%04d-%02d-%02d %02d:%02d:%02d - ",
                          ts.tm_year, ts.tm_mon + 1, ts.tm_mday,
                          ts.tm_hour, ts.tm_min, ts.tm_sec);
    }

#ifdef __lv2ppu__
    sysThreadGetId(&me);
#endif
    nb_tid += snprintf(line + nb_tid, sizeof(line) - nb_tid - 1, "%ld[%p]: ", me, (void *) me);

    va_start(ap, fmt);
    nb = nb_tid + vsnprintf(line + nb_tid, sizeof(line) - nb_tid - 1, fmt, ap);
    va_end(ap);

    /*
     * Check if we might have run out of buffer space (in case we have a
     * long line), and malloc a buffer just this once.
     */
    if (nb == sizeof(line) - 2)
    {
        line_long = (char *) malloc(LINE_BUF_SIZE * 8);
        va_start(ap, fmt);
        vsnprintf(line_long, LINE_BUF_SIZE, fmt, ap);
        va_end(ap);
        /* If this failed, we'll fall back to writing the truncated line. */
    }

    if (line_long)
    {
        nb = strlen(line_long);
        _LOCK_LOG();
        if (log_buf != 0)
        {
            _PUT_LOG(log_file, log_buf, logp - log_buf);
            logp = log_buf;
        }
        /*
         * Write out the thread id (with an optional timestamp) and the
         * malloc'ed buffer.
         */
        _PUT_LOG(log_file, line, nb_tid);
        _PUT_LOG(log_file, line_long, nb);
        /* Ensure there is a trailing newline. */
        if (!nb || (line_long[nb - 1] != '\n'))
        {
            char eol[2];
            eol[0] = '\n';
            eol[1] = '\0';
            _PUT_LOG(log_file, eol, 1);
        }
        _UNLOCK_LOG();
        free(line_long);
    }
    else
    {
        /* Ensure there is a trailing newline. */
        if (nb && (line[nb - 1] != '\n'))
        {
            line[nb++] = '\n';
            line[nb]   = '\0';
        }
        _LOCK_LOG();
        if (log_buf == 0)
        {
            _PUT_LOG(log_file, line, nb);
        }
        else
        {
            /* If nb can't fit into logBuf, write out logBuf first. */
            if (logp + nb > log_endp)
            {
                _PUT_LOG(log_file, log_buf, logp - log_buf);
                logp = log_buf;
            }
            /* nb is guaranteed to fit into logBuf. */
            memcpy(logp, line, nb);
            logp += nb;
        }
        _UNLOCK_LOG();
    }
    log_flush();
}

void log_flush(void)
{
    if (log_buf && log_file)
    {
        _LOCK_LOG();
        if (logp > log_buf)
        {
            _PUT_LOG(log_file, log_buf, logp - log_buf);
            logp = log_buf;
        }
        _UNLOCK_LOG();
    }
}

void log_abort(void)
{
    log_print("Aborting");
    abort();
}

void log_assert(const char *s, const char *file, int ln)
{
    log_print("Assertion failure: %s, at %s:%d\n", s, file, ln);
    fprintf(stderr, "Assertion failure: %s, at %s:%d\n", s, file, ln);
    fflush(stderr);
    abort();
}
