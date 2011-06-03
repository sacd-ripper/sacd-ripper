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
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#endif

#include <logging.h>

#include "getopt.h"

#include <sacd_reader.h>
#include <scarletbook.h>
#include <scarletbook_read.h>
#include <scarletbook_output.h>
#include <scarletbook_print.h>
#include <scarletbook_id3.h>
#include <endianess.h>
#include <fileutils.h>
#include <utils.h>

static struct opts_s
{
    int            two_channel;
    int            multi_channel;
    int            output_dsf;
    int            output_dsdiff;
    int            output_iso;
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

void handle_sigint(int sig_no)
{
    printf("\rUser interrupted..                                \n");
    interrupt_ripping();
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
    opts.output_iso    = 0;
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

char *get_album_dir()
{
    char disc_artist[60];
    char disc_album_title[60];
    char disc_album_year[5];
    char *albumdir;
    master_text_t *master_text = handle->master_text[0];
    int disc_artist_position = (master_text->disc_artist_position ? master_text->disc_artist_position : master_text->disc_artist_phonetic_position);
    int disc_album_title_position = (master_text->disc_title_position ? master_text->disc_title_position : master_text->disc_title_phonetic_position);

    memset(disc_artist, 0, sizeof(disc_artist));
    if (disc_artist_position)
    {
        char *c = (char *) master_text + disc_artist_position;
        char *pos = strchr(c, ';');
        if (!pos)
            pos = c + strlen(c);
        strncpy(disc_artist, c, min(pos - c, 59));
    }

    memset(disc_album_title, 0, sizeof(disc_album_title));
    if (disc_album_title_position)
    {
        char *c = (char *) master_text + disc_album_title_position;
        char *pos = strchr(c, ';');
        if (!pos)
            pos = c + strlen(c);
        strncpy(disc_album_title, c, min(pos - c, 59));
    }

    snprintf(disc_album_year, sizeof(disc_album_year), "%04d", handle->master_toc->disc_date_year);
    
    sanitize_filename(disc_artist);
    sanitize_filename(disc_album_title);

    if (strlen(disc_artist) > 0 && strlen(disc_album_title) > 0)
        albumdir = parse_format("%A - %L", 0, disc_album_year, disc_artist, disc_album_title, NULL);
    else if (strlen(disc_artist) > 0)
        albumdir = parse_format("%A", 0, disc_album_year, disc_artist, disc_album_title, NULL);
    else if (strlen(disc_album_title) > 0)
        albumdir = parse_format("%L", 0, disc_album_year, disc_artist, disc_album_title, NULL);
    else
        albumdir = parse_format("Unknown Album", 0, disc_album_year, disc_artist, disc_album_title, NULL);

    return albumdir;
}

char *get_music_filename(int area, int track)
{
    char *c;
    char track_artist[60];
    char track_title[60];
    char disc_album_title[60];
    char disc_album_year[5];
    master_text_t *master_text = handle->master_text[0];
    int disc_album_title_position = (master_text->disc_title_position ? master_text->disc_title_position : master_text->disc_title_phonetic_position);

    memset(track_artist, 0, sizeof(track_artist));
    c = handle->area[area].area_track_text[track].track_type_performer;
    if (c)
    {
        strncpy(track_artist, c, 59);
    }

    memset(track_title, 0, sizeof(track_title));
    c = handle->area[area].area_track_text[track].track_type_title;
    if (c)
    {
        strncpy(track_title, c, 59);
    }

    memset(disc_album_title, 0, sizeof(disc_album_title));
    if (disc_album_title_position)
    {
        char *c = (char *) master_text + disc_album_title_position;
        char *pos = strchr(c, ';');
        if (!pos)
            pos = c + strlen(c);
        strncpy(disc_album_title, c, min(pos - c, 59));
    }

    snprintf(disc_album_year, sizeof(disc_album_year), "%04d", handle->master_toc->disc_date_year);

    sanitize_filename(track_artist);
    sanitize_filename(disc_album_title);
    sanitize_filename(track_title);

    if (strlen(track_artist) > 0 && strlen(track_title) > 0)
        return parse_format("%N - %A - %T", track + 1, disc_album_year, track_artist, disc_album_title, track_title);
    else if (strlen(track_artist) > 0)
        return parse_format("%N - %A", track + 1, disc_album_year, track_artist, disc_album_title, track_title);
    else if (strlen(track_title) > 0)
        return parse_format("%N - %T", track + 1, disc_album_year, track_artist, disc_album_title, track_title);
    else if (strlen(disc_album_title) > 0)
        return parse_format("%N - %L", track + 1, disc_album_year, track_artist, disc_album_title, track_title);
    else
        return parse_format("%N - Unknown Artist", track + 1, disc_album_year, track_artist, disc_album_title, track_title);
}

int main(int argc, char* argv[]) {
    char *albumdir, *musicfilename, *file_path;
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
            if (handle)
            {
                if (opts.print_only)
                {
                    scarletbook_print(handle);
                }

                initialize_ripping();

                // select the channel area
                area_idx = (has_multi_channel(handle) && opts.multi_channel) ? handle->mulch_area_idx : handle->twoch_area_idx;

                albumdir      = get_album_dir();
                recursive_mkdir(albumdir, 0666);

                if (opts.output_iso)
                {
                    #define FAT32_SECTOR_LIMIT 100000
                    uint32_t total_sectors = sacd_get_total_sectors(sacd_reader);
                    uint32_t sector_size = FAT32_SECTOR_LIMIT;
                    uint32_t sector_offset = 0;
                    if (total_sectors > FAT32_SECTOR_LIMIT)
                    {
                        musicfilename = (char *) malloc(512);
                        file_path = make_filename(0, 0, albumdir, "iso");
                        for (i = 1; total_sectors != 0; i++)
                        {
                            sector_size = min(total_sectors, FAT32_SECTOR_LIMIT);
                            snprintf(musicfilename, 512, "%s.%03d", file_path, i);
                            queue_track_to_rip(0, 0, musicfilename, "iso", sector_offset, sector_size, 0);
                            sector_offset += sector_size;
                            total_sectors -= sector_size;
                        }
                        free(file_path);
                        free(musicfilename);
                    }
                    else
                    {
                        file_path = make_filename(0, 0, albumdir, "iso");
                        queue_track_to_rip(0, 0, file_path, "iso", 0, total_sectors, 0);
                        free(file_path);
                    }
                }
                else 
                {
                    // fill the queue with items to rip
                    for (i = 0; i < 1; i++) //handle->area[area_idx].area_toc->track_count; i++) 
                    {
                        musicfilename = get_music_filename(area_idx, i);
                        if (opts.output_dsf)
                        {
                            file_path     = make_filename(0, albumdir, musicfilename, "dsf");
                            queue_track_to_rip(area_idx, i, file_path, "dsf", 
                                handle->area[area_idx].area_tracklist_offset->track_start_lsn[i], 
                                handle->area[area_idx].area_tracklist_offset->track_length_lsn[i], 
                                handle->area[area_idx].area_toc->frame_format == FRAME_FORMAT_DST);
                        }
                        else if (opts.output_dsdiff)
                        {
                            file_path     = make_filename(0, albumdir, musicfilename, "dff");
                            queue_track_to_rip(area_idx, i, file_path, "dsdiff", 
                                handle->area[area_idx].area_tracklist_offset->track_start_lsn[i], 
                                handle->area[area_idx].area_tracklist_offset->track_length_lsn[i], 
                                handle->area[area_idx].area_toc->frame_format == FRAME_FORMAT_DST);
                        }

                        free(musicfilename);
                        free(file_path);
                    }
                }

                init_stats(handle_status_update_callback);
                start_ripping(handle);
                stop_ripping(handle);
                scarletbook_close(handle);

                free(albumdir);

                printf("\rWe are done..                                     \n");
            }
        }

        sacd_close(sacd_reader);
    }

    destroy_logging();
    
    return 0;
}
