#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TEXT_FILE    "/dev_usb000/debug.log"
#define LOG_BIN_FILE     "/dev_usb000/debug.bin"

extern FILE *txt_log_file;
extern FILE *bin_log_file;

#define LOG_DATA(x, s)      { fwrite((void *) x, 1, s, bin_log_file); }
#define LOG_WARN(x ...)     { fprintf(txt_log_file, "[WARN] "); fprintf(txt_log_file, x); }
#define LOG_INFO(x ...)     { fprintf(txt_log_file, "[INFO] "); fprintf(txt_log_file, x); }
#define LOG_ERROR(x ...)    { fprintf(txt_log_file, "[ERR] "); fprintf(txt_log_file, x); }
#define TRACE    { fprintf(txt_log_file, "TRACE : %s:%s - %d\n", __FILE__, __FUNCTION__, __LINE__); }

extern int open_log_files(void);
extern void close_log_files(void);

#endif /* __DEBUG_H__ */
