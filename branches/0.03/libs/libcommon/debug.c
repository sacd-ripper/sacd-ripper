#include "debug.h"

FILE *txt_log_file = NULL;
FILE *bin_log_file = NULL;

int open_log_files(void)
{
    txt_log_file = fopen(LOG_TEXT_FILE, "wt");
    if (NULL == txt_log_file)
    {
        fprintf(stderr, LOG_TEXT_FILE ": fopen() failed\n");
        return -1;
    }

    bin_log_file = fopen(LOG_BIN_FILE ".bin", "wb");
    if (NULL == bin_log_file)
    {
        fprintf(stderr, LOG_BIN_FILE ": fopen() failed\n");
        return -1;
    }
    return 0;
}

void close_log_files(void)
{
    fclose(txt_log_file);
    fclose(bin_log_file);
    txt_log_file = bin_log_file = 0;
}
