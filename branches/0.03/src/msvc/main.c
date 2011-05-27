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

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <io.h>

#include <logging.h>

#include "getopt.h"

#include <sacd_reader.h>
#include <scarletbook.h>
#include <scarletbook_read.h>
#include <scarletbook_output.h>
#include <scarletbook_print.h>
#include <scarletbook_id3.h>
#include <endianess.h>
#include <utils.h>

static struct opts_s
{
    int            two_channel;
    int            multi_channel;
    int            output_dsf;
    int            output_dsdiff;
    int            convert_dst;
    int            print_only;
    char          *input_device; /* Access method driver should use for control */
    char           output_file[512];
} opts;

scarletbook_handle_t *handle;

/* Parse all options. */
static int parse_options(int argc, char *argv[]) {
    int opt; /* used for argument parsing */
    char *program_name = NULL;

    static const char help_text[] =
        "Usage: %s [options] [outfile]\n"
        "  -2, --2ch-tracks                : Export two channel tracks (default)\n"
        "  -m, --mch-tracks                : Export multi-channel tracks\n"
        "  -p, --output-dsdiff             : output as Philips DSDIFF file (default)\n"
        "  -s, --output-dsf                : output as Sony DSF file\n"
        "                                    use of the old 'cooked ioctl' kernel\n"
        "                                    interface. -k cannot be used with -d\n"
        "                                    or -g.\n" 
        "  -c, --convert-dst               : convert DST to DSD (default)\n"
        "  -i, --input[=FILE]              : set source and determine if \"bin\" image or\n"
        "                                    device\n"
        "  -P, --print                     : display disc and track information\n" 
        "\n"
        "Help options:\n"
        "  -?, --help                      : Show this help message\n"
        "  --usage                         : Display brief usage message\n";

    static const char usage_text[] = 
        "Usage: %s [-2|--2ch-tracks] [-m|--mch-tracks] [-p|--output-dsdiff]\n"
        "        [-s|--output-dsf] [-c|--convert-dst] [-i|--input FILE] [-P|--print]\n"
        "        [-?|--help] [--usage]\n";

    static const char options_string[] = "2mpsci::P?";
    static const struct option options_table[] = {
        {"2ch-tracks", no_argument, NULL, '2' },
        {"mch-tracks", no_argument, NULL, 'm' },
        {"output-dsdiff", no_argument, NULL, 'p'}, 
        {"output-dsf", no_argument, NULL, 's'}, 
        {"convert-dst", no_argument, NULL, 'c'}, 
        {"input", required_argument, NULL, 'i' },
        {"print", no_argument, NULL, 'P' },

        {"help", no_argument, NULL, '?' },
        {"usage", no_argument, NULL, 'u' },
        { NULL, 0, NULL, 0 }
    };

    program_name = strrchr(argv[0],'/');
    program_name = program_name ? strdup(program_name+1) : strdup(argv[0]);

    while ((opt = getopt_long(argc, argv, options_string, options_table, NULL)) >= 0) {
        switch (opt) {
        case '2': 
            opts.two_channel = 1; 
            break;
        case 'm': 
            opts.multi_channel = 1; 
            break;
        case 'p': 
            opts.output_dsdiff = 1; 
            opts.output_dsf = 0; 
            break;
        case 's': 
            opts.output_dsdiff = 0; 
            opts.output_dsf = 1; 
            break;
        case 'c': opts.convert_dst = 1; break;
        case 'i': opts.input_device = strdup(optarg); break;
        case 'P': opts.print_only = 1; break;

        case '?':
            fprintf(stdout, help_text, program_name);
            free(program_name);
            return 0;
            break;

        case 'u':
            fprintf(stderr, usage_text, program_name);
            free(program_name);
            return 0;
            break;
        }
    }

    if (optind < argc) {
        const char *remaining_arg = argv[optind++];
        strcpy(opts.output_file, remaining_arg);
    }

    return 1;
}

static char *generate_trackname(int track) {
    static char outfile_name[255];
    char path[256];
    char *post = strrchr(opts.output_file, '/');
    int path_pos = (post ? post - opts.output_file + 1 : 0);
    char *file = opts.output_file + path_pos;

    path[0] = '\0';
    if (path_pos) {
        strncat(path, opts.output_file, path_pos > 256 ? 256 : path_pos);
    }

    if (strlen(file) == 0) {
        file = "sacd.dff";
    }

    snprintf(outfile_name, 246, "%strack%02d.%s", path, track, file); 
    return outfile_name;
}

void handle_sigint(int sig_no)
{
    printf("\rUser interrupted..                                \n");
    stop_ripping(handle);
}

void handle_status_update_callback(uint32_t stats_total_sectors, uint32_t stats_total_sectors_processed,
                                 uint32_t stats_current_file_total_sectors, uint32_t stats_current_file_sectors_processed,
                                 char *filename)
{
    if (filename)
    {
        printf("\rProcessing [%s]..\n", filename);
    }
    printf("\rCompleted: %d%%, Total: %d%%", (stats_current_file_sectors_processed*100/stats_current_file_total_sectors), 
                                             (stats_total_sectors_processed*100/stats_total_sectors));
    fflush(stdout);
}

/* Initialize global variables. */
static void init(void) {

    /* Default option values. */
    opts.two_channel   = 0;
    opts.multi_channel = 0;
    opts.output_dsf    = 0;
    opts.output_dsdiff = 1;
    opts.convert_dst   = 0;
    opts.print_only    = 0;
    opts.input_device  = "/dev/cdrom";

#ifdef _WIN32
    signal(SIGINT, handle_sigint);
#else
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &handle_sigint;
    sigaction(SIGINT, &sa, NULL);
#endif

    init_logging();
} 

int main(int argc, char* argv[]) {
    int i, area_idx;
    sacd_reader_t *sacd_reader;

    init();
    if (parse_options(argc, argv)) 
    {

        // default to 2 channel
        if (opts.two_channel == 0 && opts.multi_channel == 0) 
        {
            opts.two_channel = 1;
        }

        sacd_reader = sacd_open(opts.input_device);
        if (sacd_reader) 
        {

            handle = scarletbook_open(sacd_reader, 0);
            if (opts.print_only) 
            {
                scarletbook_print(handle);
            }

            // select the channel area
            area_idx = (has_multi_channel(handle) && opts.multi_channel) ? handle->mulch_area_idx : handle->twoch_area_idx;

            // fill the queue with items to rip
            for (i = 0; i < handle->area[area_idx].area_toc->track_count; i++) 
            {
                queue_track_to_rip(area_idx, i, generate_trackname(i + 1), "dsdiff", 
                    handle->area[area_idx].area_tracklist_offset->track_start_lsn[i], 
                    handle->area[area_idx].area_tracklist_offset->track_length_lsn[i], 1);
            }
            
            init_stats(handle_status_update_callback);
            start_ripping(handle);
            scarletbook_close(handle);

            printf("\rWe are done..                                     \n");
        }

        sacd_close(sacd_reader);
    }

    destroy_logging();
    
    return 0;
}
