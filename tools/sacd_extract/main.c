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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <wchar.h>
#include <locale.h>
#include <time.h>
#ifndef __APPLE__
#include <malloc.h>
#endif
#if defined(WIN32) || defined(_WIN32)
#include <io.h>
#endif

#include <pthread.h>

#include <charset.h>
#include <logging.h>

#include "getopt.h"

#include <sacd_reader.h>
#include <scarletbook.h>
#include <scarletbook_read.h>
#include <scarletbook_output.h>
#include <scarletbook_print.h>
#include <scarletbook_helpers.h>
#include <scarletbook_id3.h>
#include <cuesheet.h>
#include <endianess.h>
#include <fileutils.h>
#include <utils.h>
#include <yarn.h>
#include <version.h>

#if defined(WIN32) || defined(_WIN32)

#define CHAR2WCHAR(dst, src) dst = (wchar_t *)charset_convert(src, strlen(src), "UTF-8", "UCS-2-INTERNAL")
#else

#define CHAR2WCHAR(dst, src) dst = (wchar_t *)charset_convert(src, strlen(src), "UTF-8", "WCHAR_T") 
#endif

static struct opts_s
{
    int            two_channel;
    int            multi_channel;
    int            output_dsf;
    int            output_dsdiff_em;
    int            output_dsdiff;
    int            output_iso;
    int            convert_dst;
    int            export_cue_sheet;
    int            print;
    char           *input_device; /* Access method driver should use for control */
    char           *output_dir;
	char           *output_dir_conc;
    int            select_tracks;
    char           selected_tracks[256]; /* scarletbook is limited to 256 tracks */
    int            version;
} opts;

scarletbook_handle_t *handle;
scarletbook_output_t *output;

/* Parse all options. */
static int parse_options(int argc, char *argv[]) 
{
    int opt; /* used for argument parsing */
    char *program_name = NULL;

    static const char help_text[] =
        "Usage: %s [options] [outfile]\n"
        "  -2, --2ch-tracks                : Export two channel tracks (default)\n"
        "  -m, --mch-tracks                : Export multi-channel tracks\n"
        "  -e, --output-dsdiff-em          : output as Philips DSDIFF (Edit Master) file\n"
        "  -p, --output-dsdiff             : output as Philips DSDIFF file\n"
        "  -s, --output-dsf                : output as Sony DSF file\n"
        "  -t, --select-track              : only output selected track(s) (ex. -t 1,5,13)\n"
        "  -I, --output-iso                : output as RAW ISO\n"
#ifndef SECTOR_LIMIT
        "  -w, --concurrent                : Concurrent ISO+DSF/DSDIFF processing mode\n"
#endif
        "  -c, --convert-dst               : convert DST to DSD\n"
        "  -C, --export-cue                : Export a CUE Sheet\n"
        "  -i, --input[=FILE]              : set source and determine if \"iso\" image, \n"
        "                                    device or server (ex. -i 192.168.1.10:2002)\n"
        "  -o, --output-dir[=DIR]          : Output directory for ISO or DSDIFF Edit Master\n"
		"  -y, --output-dir-conc[=DIR]     : Output directory for DSF or DSDIFF \n"
        "  -P, --print                     : display disc and track information\n"
        "  -v, --version                   : Display version\n"
        "\n"
        "Help options:\n"
        "  -?, --help                      : Show this help message\n"
        "  --usage                         : Display brief usage message\n";

    static const char usage_text[] =
        "Usage: %s [-2|--2ch-tracks] [-m|--mch-tracks] [-p|--output-dsdiff]\n"
#ifdef SECTOR_LIMIT
        "        [-e|--output-dsdiff-em] [-s|--output-dsf] [-I|--output-iso]\n"
#else
        "        [-e|--output-dsdiff-em] [-s|--output-dsf] [-I|--output-iso] [-w|--concurrent]\n"
#endif
        "        [-c|--convert-dst] [-C|--export-cue] [-i|--input FILE] [-o|--output-dir DIR] [-y|--output-dir-conc DIR] [-P|--print]\n"
        "        [-?|--help] [--usage]\n";


#ifdef SECTOR_LIMIT
    static const char options_string[] = "2mepsIcCvi:o:y:t:P?";
#else
    static const char options_string[] = "2mepsIwcCvi:o:y:t:P?";
#endif	

    static const struct option options_table[] = {
        {"2ch-tracks", no_argument, NULL, '2'},
        {"mch-tracks", no_argument, NULL, 'm'},
        {"output-dsdiff-em", no_argument, NULL, 'e'},
        {"output-dsdiff", no_argument, NULL, 'p'},
        {"output-dsf", no_argument, NULL, 's'},
        {"output-iso", no_argument, NULL, 'I'},
#ifndef SECTOR_LIMIT
        {"concurrent", no_argument, NULL, 'w'}, 
#endif		
        {"convert-dst", no_argument, NULL, 'c'},
        {"export-cue", no_argument, NULL, 'C'},
        {"version", no_argument, NULL, 'v'},
        {"input", required_argument, NULL, 'i'},
        {"output-dir", required_argument, NULL, 'o'},
		{"output-dir-conc", required_argument, NULL, 'y' },
        {"print", no_argument, NULL, 'P'},
        {"help", no_argument, NULL, '?'},
        {"usage", no_argument, NULL, 'u'},
        {NULL, 0, NULL, 0}
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
        case 'e': 
            opts.output_dsdiff_em = 1;
            //opts.output_dsdiff = 0;
            //opts.output_dsf = 0; 
            //opts.output_iso = 0;
            opts.export_cue_sheet = 1;
            break;
        case 'p': 
            //opts.output_dsdiff_em = 0; 
            opts.output_dsdiff = 1; 
            //opts.output_dsf = 0; 
            //opts.output_iso = 0;
            break;
        case 's': 
            //opts.output_dsdiff_em = 0; 
            //opts.output_dsdiff = 0; 
            opts.output_dsf = 1; 
            //opts.output_iso = 0;
            break;
        case 't': 
            {
                int track_nr, count = 0;
                char *track = strtok(optarg, " ,");
                while (track != 0)
                {
                    track_nr = atoi(track);
                    track = strtok(0, " ,");
                    if (!track_nr)
                        continue;
                    track_nr = (track_nr - 1) & 0xff;
                    opts.selected_tracks[track_nr] = 1;
                    count++;
                }
                opts.select_tracks = count != 0;
            }
            break;
        case 'I': 
            //opts.output_dsdiff_em = 0; 
            //opts.output_dsdiff = 0; 
            //opts.output_dsf = 0; 
            opts.output_iso = 1;
            break;
		case 'w':
            //opts.concurrent = 1;  // do nothing. The program already makes all required operations in multiple steps
            break;	
        case 'c': opts.convert_dst = 1; break;
        case 'C': opts.export_cue_sheet = 1; break;
        case 'i': opts.input_device = strdup(optarg); break;
        case 'o': opts.output_dir = strdup(optarg);   break;
		case 'y': opts.output_dir_conc = strdup(optarg); break;
        case 'P': opts.print = 1; break;
        case 'v': opts.version = 1; break;

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

    return 1;
}

static lock *g_fwprintf_lock = 0;

static int safe_fwprintf(FILE *stream, const wchar_t *format, ...)
{
    int retval;
    va_list arglist;

    possess(g_fwprintf_lock);

    va_start(arglist, format);
    retval = vfwprintf(stream, format, arglist);
    va_end(arglist);

    fflush(stream);

    release(g_fwprintf_lock);

    return retval;
}

static void handle_sigint(int sig_no)
{
    safe_fwprintf(stdout, L"\n\n Program interrupted...                                                      \n");
    scarletbook_output_interrupt(output);
}

static void handle_sigterm(int sig_no)
{
    safe_fwprintf(stdout, L"\n\n Received signal to terminate program...                                                      \n");
    scarletbook_output_interrupt(output);
}

static void handle_status_update_track_callback(char *filename, int current_track, int total_tracks)
{
    wchar_t *wide_filename;

    CHAR2WCHAR(wide_filename, filename);
    safe_fwprintf(stdout, L"\nProcessing [%ls] (%d/%d)..\n", wide_filename, current_track, total_tracks);
    free(wide_filename);
}

static time_t started_processing;

static void handle_status_update_progress_callback(uint32_t stats_total_sectors, uint32_t stats_total_sectors_processed,
                                 uint32_t stats_current_file_total_sectors, uint32_t stats_current_file_sectors_processed)
{
    // safe_fwprintf(stdout, L"\rCompleted: %d%% (%.1fMB), Total: %d%% (%.1fMB) at %.2fMB/sec", (stats_current_file_sectors_processed*100/stats_current_file_total_sectors), 
    //                                          ((float)((double) stats_current_file_sectors_processed * SACD_LSN_SIZE / 1048576.00)),
    //                                          (stats_total_sectors_processed * 100 / stats_total_sectors),
    //                                          ((float)((double) stats_current_file_total_sectors * SACD_LSN_SIZE / 1048576.00)),
    //                                          (float)((double) stats_total_sectors_processed * SACD_LSN_SIZE / 1048576.00) / (float)(time(0) - started_processing)
    //                                          );
    if (stats_current_file_total_sectors == stats_total_sectors) // no need to print both stats because is one file  (ISO or DFF)
    {
        safe_fwprintf(stdout, L"\rCompleted: %d%% (file sectors processed: %d / total sectors:%d)",
                    (stats_current_file_sectors_processed * 100 / stats_current_file_total_sectors),
                    stats_current_file_sectors_processed,
                    stats_current_file_total_sectors);
    }
    else
    {
        safe_fwprintf(stdout, L"\rCompleted: %d%% (file sectors processed: %d / total sectors:%d), Total: %d%% (total sectors processed: %d / total sectors: %d)",
                    (stats_current_file_sectors_processed * 100 / stats_current_file_total_sectors),
                    stats_current_file_sectors_processed,
                    stats_current_file_total_sectors,
                    (stats_total_sectors_processed * 100 / stats_total_sectors),
                    stats_total_sectors_processed,
                    stats_total_sectors);
    }

}

/* Initialize global variables. */
static void init(void) 
{
    /* Default option values. */
    opts.two_channel        = 0;
    opts.multi_channel      = 0;
    opts.output_dsf         = 0;
    opts.output_iso         = 0;
    opts.output_dsdiff      = 0;
    opts.output_dsdiff_em   = 0;
    opts.convert_dst        = 0;
    opts.export_cue_sheet   = 0;
    opts.print              = 0;
    opts.output_dir         = NULL;
	opts.output_dir_conc	= NULL;
    opts.input_device       = NULL; //"/dev/cdrom";
    opts.version            =0;

#if defined(WIN32) || defined(_WIN32)
    signal(SIGINT, handle_sigint);
	signal(SIGTERM, handle_sigterm);
#else
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &handle_sigint;
    sigaction(SIGINT, &sa, NULL);
	
	struct sigaction sa_term;
    memset(&sa_term, 0, sizeof(sa_term));
    sa_term.sa_handler = &handle_sigterm;
    sigaction(SIGTERM, &sa_term, NULL);
#endif

    init_logging();
    g_fwprintf_lock = new_lock(0);
}
void print_start_time()
{
	started_processing = time(0);
	wchar_t *wide_asctime;
	CHAR2WCHAR(wide_asctime, asctime(localtime(&started_processing)));
	fwprintf(stdout, L"\n Started at: %ls    \n", wide_asctime );
	free(wide_asctime);
}
void print_end_time()
{
	time_t ended_processing=time(0);
	time_t seconds = difftime(ended_processing,started_processing);

	char elapsed_time[100];
	strftime(elapsed_time, 90, "%H hours:%M minutes:%S seconds", gmtime(&seconds));
    wchar_t *wide_result_time, *wide_asctime;
    CHAR2WCHAR(wide_result_time, elapsed_time);

	CHAR2WCHAR(wide_asctime, asctime(localtime(&ended_processing)));

	fwprintf(stdout, L"\n\n Ended at: %ls [elapsed: %ls]\n", wide_asctime, wide_result_time);
	free(wide_result_time);
	free(wide_asctime);	
}
#if defined(WIN32) || defined(_WIN32)
/*  Convert wide argv to UTF8   */
/*  only for Windows           */

char ** convert_wargv_to_UTF8(int argc,wchar_t *wargv[])
{
    int i;

    char **argv = malloc((argc + 1) * sizeof(*argv));

    for (i = 0; i < argc; i++)
    {
        argv[i] = (char *)charset_convert((char *)wargv[i], wcslen((const wchar_t *)wargv[i]) * sizeof(wchar_t), "UCS-2-INTERNAL", "UTF-8");
    }
         
    argv[i] = NULL;
    return argv;
}

#endif

#if defined(WIN32) || defined(_WIN32)
    int wmain(int argc, wchar_t *wargv[])      
#else
    int main(int argc, char *argv[])
#endif
{
    char *albumdir = NULL, *musicfilename = NULL, *file_path = NULL;
    int i, area_idx;
    sacd_reader_t *sacd_reader = NULL;

#ifdef PTW32_STATIC_LIB
    pthread_win32_process_attach_np();
    pthread_win32_thread_attach_np();
#endif

    init();
   
#if defined(WIN32) || defined(_WIN32)
    char **argvw_utf8 = convert_wargv_to_UTF8(argc,wargv);
    if (parse_options(argc, argvw_utf8))
#else
    if (parse_options(argc, argv))
#endif
    {
        setlocale(LC_ALL, "");
        if (fwide(stdout, 1) < 0)
        {
            fprintf(stderr, "ERROR: Output not set to wide.\n");
            goto exit_main_1;
        }

        if (opts.version==1)
        {
            fwprintf(stdout, L"sacd_extract version " SACD_RIPPER_VERSION_STRING "\n");
            //fwprintf(stdout, L"git repository: " SACD_RIPPER_REPO "\n", s_wchar);
            goto exit_main;
        }

        // default to 2 channel
        if (opts.two_channel == 0 && opts.multi_channel == 0) 
        {
            opts.two_channel = 1;
        }

        if (opts.output_dir != NULL   ) // test if exists 
        {
            if (path_dir_exists(opts.output_dir) == 0)
            {
                fprintf(stderr, "%s doesn't exist or is not a directory.\n", opts.output_dir);
                goto exit_main;
            }
            if (opts.output_dir_conc == NULL)
                opts.output_dir_conc = strdup(opts.output_dir);
        }
		
		if (opts.output_dir_conc != NULL   ) // test if exists 
        {
            if (path_dir_exists(opts.output_dir_conc) == 0)
            {
                fprintf(stderr, "%s doesn't exist or is not a directory.\n", opts.output_dir_conc);
                goto exit_main;
            }
            if (opts.output_dir == NULL)
                opts.output_dir = strdup(opts.output_dir_conc);
        }

        if(opts.input_device == NULL)
        {
            opts.input_device = strdup("/dev/cdrom");
        }

        sacd_reader = sacd_open(opts.input_device);
        if (sacd_reader != NULL) 
        {

            handle = scarletbook_open(sacd_reader, 0);
            if (handle)
            {
                if (opts.print)
                {
                    scarletbook_print(handle);
                }

                albumdir = get_album_dir(handle);

                if (opts.export_cue_sheet)
                {
                    char *cue_file_path_unique = get_unique_filename(NULL, opts.output_dir, albumdir, "cue");

                    wchar_t *wide_filename;
                    CHAR2WCHAR(wide_filename, cue_file_path_unique);
                    fwprintf(stdout, L"\n\n Exporting CUE sheet: [%ls] ... \n", wide_filename);
                    free(wide_filename);

                    if (!file_path)
                        file_path = make_filename(NULL, NULL, albumdir, "dff");

                    write_cue_sheet(handle, file_path, 0, cue_file_path_unique);
                    free(cue_file_path_unique);
                    fwprintf(stdout, L"\n\n We are done exporting CUE sheet. \n");
                }
               
                if (opts.output_iso)
                {
                        output = scarletbook_output_create(handle, handle_status_update_track_callback, handle_status_update_progress_callback, safe_fwprintf);

                        uint32_t total_sectors = sacd_get_total_sectors(sacd_reader);
#ifdef SECTOR_LIMIT
#define FAT32_SECTOR_LIMIT 2090000
                        uint32_t sector_size = FAT32_SECTOR_LIMIT;
                        uint32_t sector_offset = 0;
                        if (total_sectors > FAT32_SECTOR_LIMIT)
                        {
                            musicfilename = (char *) malloc(512);
                            file_path = make_filename(NULL, opts.output_dir, albumdir, "iso");
                            for (i = 1; total_sectors != 0; i++)
                            {
                                sector_size = min(total_sectors, FAT32_SECTOR_LIMIT);
                                snprintf(musicfilename, 512, "%s.%03d", file_path, i);
                                scarletbook_output_enqueue_raw_sectors(output, sector_offset, sector_size, musicfilename, "iso");
                                sector_offset += sector_size;
                                total_sectors -= sector_size;
                            }
                            free(file_path);
                            free(musicfilename);
                            
                        }
                        else
#endif
                        {

                            char *file_path_iso_unique = get_unique_filename(NULL, opts.output_dir, albumdir, "iso");

                            wchar_t *wide_filename;
                            CHAR2WCHAR(wide_filename, file_path_iso_unique);
                            fwprintf(stdout, L"\n Exporting ISO output in file: %ls\n", wide_filename);
                            free(wide_filename);

                            scarletbook_output_enqueue_raw_sectors(output, 0, total_sectors, file_path_iso_unique, "iso");

                            free(file_path_iso_unique);
                            
                        }

						print_start_time();
                        scarletbook_output_start(output);
                        scarletbook_output_destroy(output);
						print_end_time();

                        fwprintf(stdout, L"\n We are done exporting ISO.                                                          \n");

                } // end if (opts.output_iso)

                while (opts.two_channel + opts.multi_channel > 0)
                {
                    if ((opts.multi_channel && (!has_multi_channel(handle))) ||             // skip if we want multich but disc have no multich area
                        (opts.two_channel  && (!has_two_channel(handle)))   )                  // or want 2ch but disc have no 2 ch area
                        goto end_while_opts;

                    if (opts.output_dsdiff_em)
                    {
                        // select the channel area
                        area_idx =  has_multi_channel(handle) && opts.multi_channel  ? handle->mulch_area_idx : handle->twoch_area_idx;
                        
                        char *file_path_dsdiff_unique;
                        
                        if (area_idx == handle->twoch_area_idx)
                            file_path_dsdiff_unique = get_unique_filename(NULL, opts.output_dir, albumdir, "dff");
                        else
                        {
                            char *albumdir_multi = calloc(strlen(albumdir) + 16, sizeof(char));
                            utf8cpy(albumdir_multi, albumdir, strlen(albumdir));
                            strcat(albumdir_multi, get_speaker_config_string(handle->area[area_idx].area_toc));
                            file_path_dsdiff_unique = get_unique_filename(NULL, opts.output_dir, albumdir_multi, "dff");
                            free(albumdir_multi);
                        }
                            
                        wchar_t *wide_filename;
                        CHAR2WCHAR(wide_filename, file_path_dsdiff_unique);
                        fwprintf(stdout, L"\n Exporting DFF edit master output in file: %ls\n", wide_filename);
                        free(wide_filename);

                        output = scarletbook_output_create(handle, handle_status_update_track_callback, handle_status_update_progress_callback, safe_fwprintf);

                        scarletbook_output_enqueue_track(output, area_idx, 0, file_path_dsdiff_unique, "dsdiff_edit_master",
                                                        (opts.convert_dst ? 1 : handle->area[area_idx].area_toc->frame_format != FRAME_FORMAT_DST));

                        free(file_path_dsdiff_unique);

						print_start_time();
                        
                        scarletbook_output_start(output);
                        scarletbook_output_destroy(output);
						
						print_end_time();						

                        fwprintf(stdout, L"\n\n We are done exporting DFF edit master.                                                          \n");

                    } // end if  opts.output_dsdiff_em

                    if (opts.output_dsf || opts.output_dsdiff)
                    {
                        // select the channel area
                        area_idx = has_multi_channel(handle) && opts.multi_channel ? handle->mulch_area_idx : handle->twoch_area_idx;

                        // create the output folder
                        char *unique_dir;                   
                        if (area_idx == handle->twoch_area_idx)
                            unique_dir = get_unique_dir(opts.output_dir_conc, albumdir);
                        else
                        {
                            char *albumdir_multi = calloc(strlen(albumdir) + 16, sizeof(char));
                            utf8cpy(albumdir_multi, albumdir, strlen(albumdir));
                            strcat(albumdir_multi, get_speaker_config_string(handle->area[area_idx].area_toc));
                            unique_dir = get_unique_dir(opts.output_dir_conc, albumdir_multi);
                            free(albumdir_multi);
                        }

                        wchar_t *wide_folder;
                        CHAR2WCHAR(wide_folder, unique_dir);
                        if (opts.output_dsf)
                        {
                            fwprintf(stdout, L"\n Exporting DSF output in folder: %ls\n", wide_folder);
                        }
                        else
                        {
                            fwprintf(stdout, L"\n Exporting DSDIFF output in folder: %ls\n", wide_folder);
                        }
                        
                        int ret_mkdir;
#if defined(WIN32) || defined(_WIN32)
                        ret_mkdir = _wmkdir(wide_folder);                        
#else
                        ret_mkdir = mkdir(unique_dir, 0774);
#endif
                        free(wide_folder);

                        if (ret_mkdir != 0)
                        {
                            fprintf(stderr, "%s directory can't be created.\n", unique_dir);
                            free(unique_dir);
                            free(albumdir);
                            goto exit_main;
                        }

                        output = scarletbook_output_create(handle, handle_status_update_track_callback, handle_status_update_progress_callback, safe_fwprintf);
                        // fill the queue with items to rip
                        for (i = 0; i < handle->area[area_idx].area_toc->track_count; i++) 
                        {
                            if (opts.select_tracks && opts.selected_tracks[i] == 0)
                                continue;

                            musicfilename = get_music_filename(handle, area_idx, i, "");

                            if (opts.output_dsf)
                            {
                                file_path = make_filename(NULL, unique_dir, musicfilename, "dsf");
                                scarletbook_output_enqueue_track(output, area_idx, i, file_path, "dsf", 
                                    1 /* always decode to DSD */);
                            }
                            else if (opts.output_dsdiff)
                            {
                                file_path = make_filename(NULL, unique_dir, musicfilename, "dff");
                                scarletbook_output_enqueue_track(output, area_idx, i, file_path, "dsdiff", 
                                    (opts.convert_dst ? 1 : handle->area[area_idx].area_toc->frame_format != FRAME_FORMAT_DST));
                            }
                            free(file_path);
                            free(musicfilename);                                                   
                        }

                        free(unique_dir);

						print_start_time();

                        LOG(lm_main, LOG_NOTICE, ("Start processing dsf/dff files"));
                        scarletbook_output_start(output);
						LOG(lm_main, LOG_NOTICE, ("Start destroy dsf/dff"));
                        scarletbook_output_destroy(output);
						LOG(lm_main, LOG_NOTICE, ("Finish destroy dsf/dff"));
						
						print_end_time();

                        if (opts.output_dsf)
                            fwprintf(stdout, L"\n\n We are done exporting DSF..                                                          \n");                       
                        else
                            fwprintf(stdout, L"\n\n We are done exporting DSDIFF..                                                          \n");

                    } // end if (opts.output_dsf || opts.output_dsdiff)

                    
                    

end_while_opts:     if (opts.multi_channel == 1)
                        opts.multi_channel = 0;
                    else if(opts.two_channel == 1)
                        opts.two_channel = 0;

                } // end while opts.two_channel + opts.multi_channel
                free(albumdir);

            }  // end if handle
            scarletbook_close(handle);
        }

        sacd_close(sacd_reader);

exit_main:
    fwprintf(stdout, L"\nProgram terminates!\n");
#ifndef _WIN32
            freopen(0, "w", stdout);
#endif
        if (fwide(stdout, -1) >= 0)
        {
            fprintf(stderr, "ERROR: Output not set to byte oriented.\n");
        }
    }
exit_main_1:
    free_lock(g_fwprintf_lock);
    destroy_logging();

    if (opts.output_dir != NULL) free(opts.output_dir);

	if (opts.output_dir_conc != NULL) free(opts.output_dir_conc);

    if (opts.input_device != NULL) free(opts.input_device);

#ifdef PTW32_STATIC_LIB
    pthread_win32_process_detach_np();
    pthread_win32_thread_detach_np();
#endif

    printf("\n");
    
#if defined(WIN32) || defined(_WIN32)
     for (int t=0; t < argc;t++)
	 {
		 free(argvw_utf8[t]);		 
	 }
	 free(argvw_utf8);
	 
#endif
	
    return 0;
}
