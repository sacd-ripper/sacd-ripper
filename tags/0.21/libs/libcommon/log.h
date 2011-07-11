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

#ifndef __LOG_H__
#define __LOG_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** log.h -- Declare interfaces to the Logging service
**
** To use the service from a client program, you should create a
** PRLogModuleInfo structure by calling PR_NewLogModule(). After
** creating the LogModule, you can write to the log using the LOG()
** macro.
**
** Initialization of the log service is handled by log_init().
**
** At execution time, you must enable the log service. To enable the
** log service, set the environment variable: LOG_MODULES
** variable.
**
** LOG_MODULES variable has the form:
**
**     <moduleName>:<value>[, <moduleName>:<value>]*
**
** Where:
**  <moduleName> is the name passed to PR_NewLogModule().
**  <value> is a numeric constant, e.g. 5. This value is the maximum
** value of a log event, enumerated by PRLogModuleLevel, that you want
** written to the log.
**
** For example: to record all events of greater value than or equal to
** LOG_ERROR for a LogModule names "gizmo", say:
**
** set LOG_MODULES=gizmo:2
**
** Note that you must specify the numeric value of LOG_ERROR.
**
** Special LogModule names are provided for controlling the log
** service at execution time. These controls should be set in the
** LOG_MODULES environment variable at execution time to affect
** the log service for your application.
**
** The special LogModule "all" enables all LogModules. To enable all
** LogModule calls to LOG(), say:
**
** set LOG_MODULES=all:5
**
** The special LogModule name "sync" tells the log service to do
** unbuffered logging.
**
** The special LogModule name "bufsize:<size>" tells the log service 
** to set the log buffer to <size>.
**
** The environment variable LOG_FILE specifies the log file to use
** unless the default of "stderr" is acceptable. For MS Windows
** systems, LOG_FILE can be set to a special value: "WinDebug"
** (case sensitive). This value causes LOG() output to be written
** using the Windows API OutputDebugString(). OutputDebugString()
** writes to the debugger window; some people find this helpful.
**
**
** To put log messages in your programs, use the LOG macro:
**
**     LOG(<module>, <level>, (<printfString>, <args>*));
**
** Where <module> is the address of a PRLogModuleInfo structure, and
** <level> is one of the levels defined by the enumeration:
** PRLogModuleLevel. <args> is a printf() style of argument list. That
** is: (fmtstring, ...).
**
** Example:
**
** main() {
**    int one = 1;
**    PRLogModuleInfo * myLm = PR_NewLogModule("gizmo");
**    LOG( myLm, LOG_ALWAYS, ("Log this! %d\n", one));
**    return;
** }
**
** Note the use of printf() style arguments as the third agrument(s) to
** LOG().
**
** After compiling and linking you application, set the environment:
**
** set LOG_MODULES=gizmo:5
** set LOG_FILE=logfile.txt
**
** When you execute your application, the string "Log this! 1" will be
** written to the file "logfile.txt".
**
*/

typedef enum log_module_level_t
{
    LOG_NONE    = 0,                /* nothing */
    LOG_ALWAYS  = 1,                /* always printed */
    LOG_ERROR   = 2,                /* error messages */
    LOG_WARNING = 3,                /* warning messages */
    LOG_DEBUG   = 4,                /* debug messages */
                                    
    LOG_NOTICE  = LOG_DEBUG,        /* notice messages */
    LOG_WARN    = LOG_WARNING,      /* warning messages */
    LOG_MIN     = LOG_DEBUG,        /* minimal debugging messages */
    LOG_MAX     = LOG_DEBUG         /* maximal debugging messages */
} log_module_level_t;

/*
** One of these structures is created for each module that uses logging.
**    "name" is the name of the module
**    "level" is the debugging level selected for that module
*/
typedef struct log_module_info_t
{
    const char             *name;
    log_module_level_t       level;
    struct log_module_info_t *next;
} log_module_info_t;

/*
 ** initialize logging
 */
void log_init(void);

/*
 ** destroy logging
 */
void log_destroy(void);

/*
 ** Create a new log module.
 */
log_module_info_t* create_log_module(const char *name);

/*
** Set the file to use for logging. Returns PR_FALSE if the file cannot
** be created
*/
int set_log_file(const char *name);

/*
** Set the size of the logging buffer. If "buffer_size" is zero then the
** logging becomes "synchronous" (or unbuffered).
*/
void set_log_buffering(int buffer_size);

/*
** Print a string to the log. "fmt" is a PR_snprintf format type. All
** messages printed to the log are preceeded by the name of the thread
** and a time stamp. Also, the routine provides a missing newline if one
** is not provided.
*/
void log_print(const char *fmt, ...);

/*
** Flush the log to its file.
*/
void log_flush(void);

void log_assert(const char *s, const char *file, int ln);

#define DEBUG 1

#if defined(DEBUG) || defined(FORCE_LOG)
#define LOGGING    1

#define LOG_TEST(_module, _level) \
    ((_module)->level >= (_level))

/*
** Log something.
**    "module" is the address of a PRLogModuleInfo structure
**    "level" is the desired logging level
**    "args" is a variable length list of arguments to print, in the following
**       format:  ("printf style format string", ...)
*/
#define LOG(_module, _level, _args)  \
    {                                   \
    if (LOG_TEST(_module, _level)) { \
        log_print _args;              \
    }                                   \
    }

#else /* defined(DEBUG) || defined(FORCE_LOG) */

#undef LOGGING
#define LOG_TEST(module, level)    0
#define LOG(module, level, args)

#endif /* defined(DEBUG) || defined(FORCE_LOG) */

#if defined(DEBUG) || defined(FORCE_ASSERT)

#define ASSERT(_expr) \
    ((_expr) ? ((void) 0) : log_assert(# _expr, __FILE__, __LINE__))

#define NOT_REACHED(_reasonStr) \
    log_assert(_reasonStr, __FILE__, __LINE__)

#else

#define ASSERT(expr)    ((void) 0)
#define NOT_REACHED(reasonStr)

#endif /* defined(DEBUG) || defined(FORCE_ASSERT) */

#ifdef __cplusplus
};
#endif

#endif /* __LOG_H__ */
