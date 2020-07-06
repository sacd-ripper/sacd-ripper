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
#include "sacd_reader.h"
#include "scarletbook.h"
#include "scarletbook_read.h"
#include "scarletbook_output.h"
#include "scarletbook_print.h"
#include "scarletbook_helpers.h"
#include "scarletbook_id3.h"
#include "cuesheet.h"
#include "endianess.h"
#include "fileutils.h"
#include "utils.h"
#include "yarn.h"
#include "version.h"
#include "scarletbook_xml.h"




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
    uint8_t        selected_tracks[256]; /* scarletbook is limited to 256 tracks */
    int            dsf_nopad;
    int            audio_frame_trimming; // if 1  trimm out audioframes in trimecode interval [area_tracklist_time->start...+duration]
    int            artist_flag;          // if artist ==1 then the artist name is added in folder name
    int            performer_flag;       // if performer ==1 the performer from each track is added
    int            concatenate;  // concatenate consecutive tracks specified in selected_tracks
    int            logging;  // if 1 save logs in a file
    int            id3_tag_mode; // id3_tag_mode;  // 0=no id3 inserted; 1 or 3 =default id3 v2.3; 2=miminal id3v2.3 tag; 4=id3v2.4;5=id3v2.4 minimal
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
        "Usage: %s [options] -i[=FILE]\n"
        "Options:\n"
        "  -2, --2ch-tracks                : Export two channel tracks (default)\n"
        "  -m, --mch-tracks                : Export multi-channel tracks\n"
        "  -e, --output-dsdiff-em          : output as Philips DSDIFF (Edit Master) file\n"
        "  -p, --output-dsdiff             : output as Philips DSDIFF file\n"
        "  -s, --output-dsf                : output as Sony DSF file\n"
        "  -z, --dsf-nopad                 : Do not zero pad DSF (cannot be used with -t)\n"
        "  -t, --select-track              : only output selected track(s) (ex. -t 1,5,13)\n"
        "  -k, --concatenate               : concatenate consecutive selected track(s) (ex. -k -t 2,3,4)\n"
        "  -I, --output-iso                : output as RAW ISO\n"
#ifndef SECTOR_LIMIT
        "  -w, --concurrent                : Concurrent ISO+DSF/DSDIFF processing mode\n"
#endif
        "  -c, --convert-dst               : convert DST to DSD\n"
        "  -C, --export-cue                : Export a CUE Sheet\n"
        "  -o, --output-dir[=DIR]          : Output directory for ISO or DSDIFF Edit Master\n"
        "  -y, --output-dir-conc[=DIR]     : Output directory for DSF or DSDIFF \n"
        "  -P, --print                     : display disc and track information\n"
        "  -A, --artist                    : artist name is added in folder name. Default is disabled\n"
        "  -a, --performer                 : performer name is added in track filename. Default is disabled\n"
        "  -b, --pauses                    : all pauses will be included. Default is disabled\n"
        "  -v, --version                   : Display version\n"
        "\n"
        "  -i, --input[=FILE]              : set source and determine if \"iso\" image, \n"
        "                                    device or server (ex. -i 192.168.1.10:2002)\n"
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
    static const char options_string[] = "2mepszkaAbIcCvi:o:y:t:P?";
#else
    static const char options_string[] = "2mepszkaAbIwcCvi:o:y:t:P?";
#endif

    static const struct option options_table[] = {
        {"2ch-tracks", no_argument, NULL, '2'},
        {"mch-tracks", no_argument, NULL, 'm'},
        {"output-dsdiff-em", no_argument, NULL, 'e'},
        {"output-dsdiff", no_argument, NULL, 'p'},
        {"output-dsf", no_argument, NULL, 's'},
        {"dsf-nopad", no_argument, NULL, 'z'},
        {"concatenate", no_argument, NULL, 'k'},
        {"artist", no_argument, NULL, 'A'},
        {"performer", no_argument, NULL, 'a'},
        {"pauses", no_argument, NULL, 'b'},
        {"output-iso", no_argument, NULL, 'I'},
#ifndef SECTOR_LIMIT
        {"concurrent", no_argument, NULL, 'w'}, 
#endif
        {"convert-dst", no_argument, NULL, 'c'},
        {"export-cue", no_argument, NULL, 'C'},
        {"version", no_argument, NULL, 'v'},
        {"input", required_argument, NULL, 'i'},
        {"output-dir", required_argument, NULL, 'o'},
        {"output-dir-conc", required_argument, NULL, 'y'},
        {"print", no_argument, NULL, 'P'},
        {"help", no_argument, NULL, '?'},
        {"usage", no_argument, NULL, 'u'},
        {NULL, 0, NULL, 0}};

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
                for(int m=0;m<255;m++)opts.selected_tracks[m]=0x00;
                int track_nr, count = 0;
                char *track = strtok(optarg, " ,");
                while (track != 0)
                {
                    track_nr = atoi(track);
                    track = strtok(0, " ,");
                    if (!track_nr)
                        continue;
                    track_nr = (track_nr - 1) & 0xff;
                    opts.selected_tracks[track_nr] = 0x01;
                    count++;
                }
                opts.select_tracks = count != 0;
            }
            break;
        case 'z':
            opts.dsf_nopad = 1;
            break;
        case 'b':
            opts.audio_frame_trimming = 0;
            break;
        case 'A':
            opts.artist_flag = 1;
            break;
        case 'a':
            opts.performer_flag = 1;
            break;
        case 'k': // concatenate consecutive tracks specified in selected_tracks
            opts.concatenate = 1;
            // must enable include pauses
            if (opts.audio_frame_trimming == 1)opts.audio_frame_trimming = 0;        
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
        case 'o':
        {
			size_t n = strlen(optarg);
            if (n > 2)
            {     				
				// remove double quotes if exists (especially in Windows)
				char * start_dir;
                if (optarg[0] ==  '\"' )
                {  
					start_dir=optarg + 1;
                    n =n-1;
                }
				else
					start_dir=optarg;
				if (start_dir[n - 1] ==  '\"' )  
				  n = n-1;
								 
                // strip ending slash if exists
                if (start_dir[n - 1] == '\\' ||
                    start_dir[n - 1] == '/')
                {
					n=n-1;
                }
                //opts.output_dir = strndup(start_dir, n - 1); //  strndup didn't exist in Windows
                opts.output_dir = calloc(n+1, sizeof(char));
                memcpy(opts.output_dir, start_dir, n);                               
            }
            break;
        }
        case 'y': 
        {
			size_t n = strlen(optarg);
            if (n > 2)
            {     				
				// remove double quotes if exists (especially in Windows)
				char * start_dir;
                if (optarg[0] ==  '\"' )
                {
                    start_dir = optarg + 1;
                    n = n - 1;
                }
                else
					start_dir=optarg;
				if (start_dir[n - 1] ==  '\"' )  
				  n = n-1;
								 
                // strip ending slash if exists
                if (start_dir[n - 1] == '\\' ||
                    start_dir[n - 1] == '/')
                {
					n=n-1;
                }
                //opts.output_dir_conc = strndup(start_dir, n - 1); //  strndup didn't exist in Windows
                opts.output_dir_conc = calloc(n+1, sizeof(char));
                memcpy(opts.output_dir_conc, start_dir, n);                               
            }		
            break;
        }
        case 'P': opts.print = 1; break;
        case 'v': opts.version = 1; break;

        case '?':
            fprintf(stdout, help_text, program_name);
            free(program_name);
            return 0;
            break;

        case 'u':
            fprintf(stdout, usage_text, program_name);
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
    opts.version            = 0;
    opts.dsf_nopad          = 0;
    opts.audio_frame_trimming=1;  // default is On ; eliminates pauses
    opts.artist_flag        = 0;    // if artist ==1 then the artist name is added in folder name
    opts.performer_flag     = 0; // if performer ==1 the performer from each track is added
    opts.concatenate        = 0; // concatenate consecutive tracks specified in t
    opts.select_tracks      = 0;
    opts.logging            = 0;
    opts.id3_tag_mode       = 4; // default id3v2. tag and UTF8 encoding

#if defined(WIN32) || defined(_WIN32)
    signal(SIGINT, handle_sigint);
	
#else
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &handle_sigint;
    sigaction(SIGINT, &sa, NULL);
#endif

        //init_logging(1);   //init_logging(0); 0 = not create a log file
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


//   read from file sacd_extract.cfg
//   if artist=1 then artist name will be added to the name of folder
//   if sacd_extract.cfg didn't exist then return 0 
//       else 1
//    
int read_config()
{
    int ret;
    const char *filename_cfg = "sacd_extract.cfg"; //char str[] = "sacd_extract.cfg";

#if defined(WIN32) || defined(_WIN32)
    struct _stat fileinfo_win;
    ret = _stat(filename_cfg, &fileinfo_win);
#else
    struct stat fileinfo;
    ret = stat(filename_cfg, &fileinfo);
#endif

    if (ret != 0)
        return 0; // if file cfg not exists then exit

#if defined(WIN32) || defined(_WIN32)
    if ((fileinfo_win.st_mode & _S_IFMT) == _S_IFREG)
#else
    if (S_ISCHR(fileinfo.st_mode) ||
        S_ISREG(fileinfo.st_mode))
#endif
    {
        FILE *fp;
        char content[100]; // content to be read

        fp = fopen(filename_cfg, "r");
        if (!fp)
            return 0;

        while (fgets(content, 100, fp) != NULL)
        {
            if ((strstr(content, "artist=1") != NULL) ||(strstr(content, "artist=yes") != NULL))
                opts.artist_flag = 1;
            if ((strstr(content, "performer=1") != NULL) || (strstr(content, "performer=yes") != NULL))
                opts.performer_flag = 1;
            if ((strstr(content, "pauses=1") != NULL) || (strstr(content, "pauses=yes") != NULL))
                opts.audio_frame_trimming = 0;
            if ((strstr(content, "nopad=1") != NULL) || (strstr(content, "nopad=yes") != NULL))
                opts.dsf_nopad = 1;
            if ((strstr(content, "concatenate=1") != NULL) || (strstr(content, "concatenate=yes") != NULL))
            {
                opts.concatenate = 1;
                opts.audio_frame_trimming = 0; // when concatenate must include all pausese and disable dsf_pad !!!
            }  
            if ((strstr(content, "logging=1") != NULL) || (strstr(content, "logging=yes") != NULL))
                    opts.logging = 1;

            if ((strstr(content, "id3tag=0") != NULL) || (strstr(content, "id3tag=no") != NULL)) // 0=no id3 inserted
                opts.id3_tag_mode=0;
            if (strstr(content, "id3tag=1") != NULL) // 1=id3 v2.3;  UTF-16 encoding
                opts.id3_tag_mode = 1;
            if (strstr(content, "id3tag=2") != NULL)   // 2=miminal id3v2.3 tag; UTF-16 encoding
                opts.id3_tag_mode = 2;
            if (strstr(content, "id3tag=3") != NULL) // 4=id3v2.3 ; ISO_8859_1 encoding
                opts.id3_tag_mode = 3;
            if (strstr(content, "id3tag=4") != NULL) // 4=id3v2.4; UTF-8 encoding
                opts.id3_tag_mode = 4;
            if (strstr(content, "id3tag=5") != NULL) // 5=id3v2.4 minimal;UTF-8 encoding
                opts.id3_tag_mode = 5;
        }
        fclose(fp);
        fwprintf(stdout, L"\nFound configuration 'sacd_extract.cfg' file...\n" );
        fwprintf(stdout, L"\tArtist will be added in folder name (artist=%d) %ls\n",opts.artist_flag, opts.artist_flag > 0 ? L"yes" : L"no");
        fwprintf(stdout, L"\tPerformer will be added in filename of track (performer=%d) %ls\n",opts.performer_flag, opts.performer_flag > 0 ? L"yes" : L"no");
        fwprintf(stdout, L"\tPadding-less (nopad=%d) %ls\n", opts.dsf_nopad, opts.dsf_nopad != 0 ? L"yes" : L"no");
        fwprintf(stdout, L"\tPauses included (pauses=%d) %ls\n", !opts.audio_frame_trimming, opts.audio_frame_trimming == 0 ? L"yes" : L"no");
        fwprintf(stdout, L"\tConcatenate (concatenate=%d) %ls\n", opts.concatenate, opts.concatenate > 0 ? L"yes" : L"no");
        switch (opts.id3_tag_mode)
        {
        case 0:
            fwprintf(stdout, L"\tID3tag no inserted (id3tag = %d) \n", opts.id3_tag_mode);
            break;
        case 1:
        case 2:
        case 3:
            fwprintf(stdout, L"\tID3tagV2.3 (id3tag = %d) %ls\n", opts.id3_tag_mode, opts.id3_tag_mode == 2 ? L"minimal" : L"yes");
            break;
        case 4:
        case 5:
            fwprintf(stdout, L"\tID3tagV2.4 (id3tag = %d) %ls\n", opts.id3_tag_mode, opts.id3_tag_mode == 5 ? L"minimal" : L"yes");
            break;
        default:
            fwprintf(stdout, L"\tID3tag (id3tag = %d)\n", opts.id3_tag_mode);
            break;
        }
        return 1;
    }
    else
    {
        return 0;
    }
    
} // end read_config



//   Creates directory tree like: Album title \ (Disc no..) \ Stereo (or Multich)
//     input: hadle
//            area_idx
//            If there is not multichannel area Then did not add \Stereo..or Multich 
//            base_output_dir = directory from where to start creating new directory tree
//
char *create_path_output(scarletbook_handle_t *handle, int area_idx, char * base_output_dir)
{
#if defined(WIN32) || defined(_WIN32)
char PATH_TRAILING_SLASH[2]= {'\\','\0'};
#else
char PATH_TRAILING_SLASH[2] = {'/', '\0'};
#endif

	char *path_output;
    char *album_path = get_path_disc_album(handle,opts.artist_flag);
	
	if(album_path==NULL)return NULL;
	
	if(base_output_dir !=NULL)
	{
      path_output = calloc(strlen(base_output_dir) + 1 + strlen(album_path) + 20, sizeof(char));
	  strncpy(path_output, base_output_dir, strlen(base_output_dir));
	  strncat(path_output, PATH_TRAILING_SLASH,1);
	}
	else
	  path_output = calloc(strlen(album_path) + 20, sizeof(char));

    strncat(path_output, album_path, strlen(album_path));
    free(album_path);

    if (has_multi_channel(handle))
    {
        strncat(path_output, PATH_TRAILING_SLASH, 1);
        strcat(path_output, get_speaker_config_string(handle->area[area_idx].area_toc));
    }

    int ret_mkdir = recursive_mkdir(path_output, base_output_dir, 0774);

    if (ret_mkdir != 0)
    {
        wchar_t *wide_filename;
        CHAR2WCHAR(wide_filename, path_output);
        fwprintf(stderr, L"\n\n Error: %s directory can't be created.\n", wide_filename);
        free(wide_filename);
        
        free(path_output);
        return NULL;
    }
    return path_output;
}

#if defined(WIN32) || defined(_WIN32)
    int wmain(int argc, wchar_t *wargv[])      
#else
    int main(int argc, char *argv[])
#endif
{
    char *album_filename = NULL, *musicfilename = NULL, *file_path = NULL;
    int i, area_idx;
    sacd_reader_t *sacd_reader = NULL;
	int exit_main_flag=0; //0=succes; -1 failed

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
            fprintf(stderr, "\nERROR: Output not set to wide.\n");
			exit_main_flag=-1;
            goto exit_main_1;
        }
        fwprintf(stdout, L"\nsacd_extract 0.3.9.3 enhanced by euflo ....starting!\n");

        int exist_cfg = read_config();
        init_logging(opts.logging); //init_logging(0); 1= write logs in a file

        if (opts.version==1)
        {
            fwprintf(stdout, L"\nsacd_extract " SACD_RIPPER_VERSION_STRING "\n");
            fwprintf(stdout, L"git repository: " SACD_RIPPER_REPO "\n");
            size_t size = 512;
            char *buffer = (char *)calloc(size, sizeof(char));
            if (getcwd(buffer, size) != NULL)
            {
                wchar_t *wide_filename;
                CHAR2WCHAR(wide_filename, buffer);
                fwprintf(stdout, L"\nWorking directory (for the app and 'sacd_extract.cfg' file): %ls\n", wide_filename);
                free(wide_filename);
                if(!exist_cfg)  // do not repeat again the same text...as in read-config()
                {
                    fwprintf(stdout, L"Configuration settings:\n");
                    fwprintf(stdout, L"\tArtist will be added in folder name (artist=%d) %ls\n", opts.artist_flag, opts.artist_flag > 0 ? L"yes" : L"no");
                    fwprintf(stdout, L"\tPerformer will be added in filename of track (performer=%d) %ls\n", opts.performer_flag, opts.performer_flag > 0 ? L"yes" : L"no");
                    fwprintf(stdout, L"\tPadding-less (nopad=%d) %ls\n", opts.dsf_nopad, opts.dsf_nopad != 0 ? L"yes" : L"no");
                    fwprintf(stdout, L"\tPauses included (pauses=%d) %ls\n", !opts.audio_frame_trimming, opts.audio_frame_trimming == 0 ? L"yes" : L"no");
                    fwprintf(stdout, L"\tConcatenate (concatenate=%d) %ls\n", opts.concatenate, opts.concatenate > 0 ? L"yes" : L"no");
                    fwprintf(stdout, L"\tID3tag (id3tag = %d)\n", opts.id3_tag_mode);
                }              
            }
            free(buffer);
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
                wchar_t *wide_filename;
                CHAR2WCHAR(wide_filename, opts.output_dir);
                fwprintf(stdout, L"%ls doesn't exist or is not a directory.\n",wide_filename);
                free(wide_filename);

				exit_main_flag=-1;
                goto exit_main;
            }
            if (opts.output_dir_conc == NULL)
                opts.output_dir_conc = strdup(opts.output_dir);
        }
		
		if (opts.output_dir_conc != NULL   ) // test if exists 
        {
            if (path_dir_exists(opts.output_dir_conc) == 0)
            {
                wchar_t *wide_filename;
                CHAR2WCHAR(wide_filename, opts.output_dir_conc);
                fwprintf(stdout, L"%ls doesn't exist or is not a directory.\n", opts.output_dir_conc);
                free(wide_filename);

                exit_main_flag=-1;
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

            handle = scarletbook_open(sacd_reader);
            if (handle)
            {
                handle->concatenate = opts.concatenate;
                handle->audio_frame_trimming = opts.audio_frame_trimming;  
                handle->dsf_nopad = opts.dsf_nopad;
                handle->id3_tag_mode=opts.id3_tag_mode;


                album_filename = get_album_dir(handle, opts.artist_flag);
                
                if (opts.print)
                {
                    scarletbook_print(handle);
                }
               
                if (opts.output_iso)
                {
                    // create the output folder
                    
#if defined(WIN32) || defined(_WIN32)
                    char PATH_TRAILING_SLASH[2] = {'\\', '\0'};
#else
                    char PATH_TRAILING_SLASH[2] = {'/', '\0'};
#endif
                    char *album_path = get_path_disc_album(handle,opts.artist_flag);
					char *output_dir;
					if(opts.output_dir !=NULL)
					{
						output_dir = calloc(strlen(opts.output_dir) + 1 + strlen(album_path) + 1, sizeof(char));
						strncpy(output_dir, opts.output_dir, strlen(opts.output_dir));
						strncat(output_dir, PATH_TRAILING_SLASH, 1);
					}
					else
						output_dir = calloc(strlen(album_path) + 1, sizeof(char));
					

                    strncat(output_dir, album_path, strlen(album_path));
                    free(album_path);

                    int ret_mkdir = recursive_mkdir(output_dir, opts.output_dir, 0774);

                    if (ret_mkdir != 0)
                    {
                        free(album_filename);
                        scarletbook_close(handle);
                        sacd_close(sacd_reader);
						exit_main_flag=-1;
                        goto exit_main;
                    }

                    output = scarletbook_output_create(handle, handle_status_update_track_callback, handle_status_update_progress_callback, safe_fwprintf);

                    uint32_t total_sectors = sacd_get_total_sectors(sacd_reader);
#ifdef SECTOR_LIMIT
#define FAT32_SECTOR_LIMIT 2090000
                    uint32_t sector_size = FAT32_SECTOR_LIMIT;
                    uint32_t sector_offset = 0;
                    if (total_sectors > FAT32_SECTOR_LIMIT)
                    {
                        musicfilename = (char *) malloc(512);
                        file_path = make_filename(NULL, output_dir, album_filename, "iso");
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

                        char *file_path_iso_unique = get_unique_filename(NULL, output_dir, album_filename, "iso");

                        wchar_t *wide_filename;
                        CHAR2WCHAR(wide_filename, file_path_iso_unique);
                        fwprintf(stdout, L"\n Exporting ISO output in file: %ls\n", wide_filename);
                        free(wide_filename);

                        scarletbook_output_enqueue_raw_sectors(output, 0, total_sectors, file_path_iso_unique, "iso");

                        free(file_path_iso_unique);
                        
                    }
                    free(output_dir);
                    
                    print_start_time();
                    scarletbook_output_start(output);
                    scarletbook_output_destroy(output);
                    print_end_time();

                    fwprintf(stdout, L"\n We are done exporting ISO.                                                          \n");

                } // end if (opts.output_iso)

                if (opts.output_dsf || opts.output_dsdiff || opts.output_dsdiff_em || opts.export_cue_sheet)
                {
                    while (opts.two_channel + opts.multi_channel > 0)
                    {
                        if (opts.multi_channel && (!has_multi_channel(handle))) // skip if we want multich but disc have no multich area
                        {
                            fwprintf(stdout, L"\n Asked multichannel format but disc has no multichannel area. So skip processing...                                            \n");
                            opts.multi_channel = 0;
                            continue;
                        }

                        if (opts.two_channel && (!has_two_channel(handle) )) // skip;   if want 2ch but disc have no 2 ch area (YES !!! Exists these type of discs  - e.g Rubinstein - Grieg..only multich area)                                                    
                        {
                                fwprintf(stdout, L"\n Asked for stereo format but disc has no stereo area. So skip processing...                                            \n");
                                opts.two_channel = 0;
                                continue;
                        }

                        if (opts.output_dsdiff_em)
                        {
                            // select the channel area
                            area_idx =  has_multi_channel(handle) && opts.multi_channel  ? handle->mulch_area_idx : handle->twoch_area_idx;                        

                            // create the output folder
                            char *output_dir = create_path_output(handle, area_idx, opts.output_dir);
                            if (output_dir == NULL)
                            {
                                free(album_filename);
                                scarletbook_close(handle);
                                sacd_close(sacd_reader);
								exit_main_flag=-1;
                                goto exit_main;
                            }

                            char *file_path_dsdiff_unique = get_unique_filename(NULL, output_dir, album_filename, "dff");
                            free(output_dir);

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

                            // Must generate cue sheet
                            opts.export_cue_sheet=1;

                        } // end if  opts.output_dsdiff_em

                        if (opts.export_cue_sheet)
                        {
                            // select the channel area
                            area_idx = has_multi_channel(handle) && opts.multi_channel ? handle->mulch_area_idx : handle->twoch_area_idx;

                            // create the output folder
                            char *output_dir = create_path_output(handle, area_idx, opts.output_dir);
                            if (output_dir == NULL)
                            {
                                free(album_filename);
                                scarletbook_close(handle);
                                sacd_close(sacd_reader);
								exit_main_flag=-1;
                                goto exit_main;
                            }

                            char *cue_file_path_unique = get_unique_filename(NULL, output_dir, album_filename, "cue");

                            //free(output_dir);

                            wchar_t *wide_filename;
                            CHAR2WCHAR(wide_filename, cue_file_path_unique);
                            fwprintf(stdout, L"\n\n Exporting CUE sheet: [%ls] ... \n", wide_filename);
                            free(wide_filename);

                            file_path = make_filename(NULL, NULL, album_filename, "dff");

                            write_cue_sheet(handle, file_path, area_idx, cue_file_path_unique);
                            free(cue_file_path_unique);
                            free(file_path);

                            fwprintf(stdout, L"\n\n We are done exporting CUE sheet. \n");

                            // create file XML metadata file
                            char *metadata_file_path_unique = get_unique_filename(NULL, output_dir, album_filename, "xml");
                            if (metadata_file_path_unique == NULL)
                                fwprintf(stderr, L"\n ERROR: cannot create get_unique_filename (==NULL) !!\n");

                            free(output_dir);

                            //wchar_t *wide_filename;
                            CHAR2WCHAR(wide_filename, metadata_file_path_unique);

                            fwprintf(stdout, L"\n\n Exporting metadata XML file: [%ls] ... \n", wide_filename);
                            free(wide_filename);

                            write_metadata_xml(handle, metadata_file_path_unique);
                            free(metadata_file_path_unique);
                            fwprintf(stdout, L"\n\n We are done exporting XML metadata. \n");
                        }

                        if (opts.output_dsf || opts.output_dsdiff)
                        {
                            // select the channel area
                            area_idx = has_multi_channel(handle) && opts.multi_channel ? handle->mulch_area_idx : handle->twoch_area_idx;

                            // create the output folder
                            char *output_dir = create_path_output(handle, area_idx, opts.output_dir_conc);
                            if (output_dir == NULL)
                            {
                                free(album_filename);
                                scarletbook_close(handle);
                                sacd_close(sacd_reader);
								exit_main_flag=-1;
                                goto exit_main;
                            }

                            wchar_t *wide_folder;
                            CHAR2WCHAR(wide_folder, output_dir);
                            if (opts.output_dsf)
                            {
                                fwprintf(stdout, L"\n Exporting DSF output in folder: %ls\n", wide_folder);
                            }
                            else
                            {
                                fwprintf(stdout, L"\n Exporting DSDIFF output in folder: %ls\n", wide_folder);
                            }
                            free(wide_folder);

                            output = scarletbook_output_create(handle, handle_status_update_track_callback, handle_status_update_progress_callback, safe_fwprintf);

                            if(opts.concatenate == 0)
                            {
                                // fill the queue with items to rip
                                for (i = 0; i < handle->area[area_idx].area_toc->track_count; i++)
                                {
                                    if (opts.select_tracks && opts.selected_tracks[i] == 0x0)
                                        continue;

                                    musicfilename = get_music_filename(handle, area_idx, i, "", opts.performer_flag);

                                    if (opts.output_dsf)
                                    {
                                        file_path = make_filename(NULL, output_dir, musicfilename, "dsf");
                                        scarletbook_output_enqueue_track(output, area_idx, i, file_path, "dsf",
                                                                         1 /* always decode to DSD */);
                                    }
                                    else if (opts.output_dsdiff)
                                    {
                                        file_path = make_filename(NULL, output_dir, musicfilename, "dff");
                                        scarletbook_output_enqueue_track(output, area_idx, i, file_path, "dsdiff",
                                                                         (opts.convert_dst ? 1 : handle->area[area_idx].area_toc->frame_format != FRAME_FORMAT_DST));
                                    }
                                    free(file_path);
                                    free(musicfilename);
                                }
                            }
                            else  // made concatenation
                            {
                                // fill the queue with item to rip
                                if (opts.select_tracks)
                                {
                                    int first_track = handle->area[area_idx].area_toc->track_count-1;
                                    int last_track  = 0;
                                    // find first track and last track in list
                                    for (i = 0; i < handle->area[area_idx].area_toc->track_count; i++)
                                    {
                                        if (opts.selected_tracks[i] == 0x01)
                                        {
                                            if (first_track > i)
                                                first_track = i;
                                            if (last_track <  i)
                                                last_track = i;
                                        }                                         
                                    }


                                    if ((first_track < handle->area[area_idx].area_toc->track_count)&&
                                        (last_track < handle->area[area_idx].area_toc->track_count) )
                                    {
                                        char conc_string[10];
                                        snprintf(conc_string, sizeof(conc_string), "[%02d-%02d]",first_track + 1, last_track + 1);

                                        musicfilename = get_music_filename(handle, area_idx, first_track, conc_string, opts.performer_flag);

                                        fwprintf(stdout, L"\n Concatenate tracks: %d to %d\n", first_track+1,last_track+1);
                                        if (opts.output_dsf)
                                        {
                                            file_path = make_filename(NULL, output_dir, musicfilename, "dsf");
                                            //file_path = get_unique_filename(NULL, output_dir, album_filename, "dsf");
                                            scarletbook_output_enqueue_concatenate_tracks(output, area_idx, first_track, file_path, "dsf",
                                                                                          1 /* always decode to DSD */, last_track);
                                        }
                                        else if (opts.output_dsdiff)
                                        {
                                            file_path = make_filename(NULL, output_dir, musicfilename, "dff");
                                            //file_path = get_unique_filename(NULL, output_dir, album_filename, "dff");
                                            scarletbook_output_enqueue_concatenate_tracks(output, area_idx, first_track, file_path, "dsdiff",
                                                                                          (opts.convert_dst ? 1 : handle->area[area_idx].area_toc->frame_format != FRAME_FORMAT_DST), last_track);
                                        }
                                        free(file_path);
                                        free(musicfilename);
                                    
                                    }
                                }
                                else  // no tracks specified
                                {
                                    fwprintf(stdout, L"\n\n Warning! Concatenation activated but no tracks selected!\n");
                                }                                                                                                  
                            }                          

                            free(output_dir);

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

                        
                        if (opts.multi_channel == 1)
                            opts.multi_channel = 0;
                        else if(opts.two_channel == 1)
                            opts.two_channel = 0;

                    } // end while opts.two_channel + opts.multi_channel
                }  // end if opts....
                free(album_filename);
                
				scarletbook_close(handle);
            }  // end if handle
			else
				exit_main_flag=-1;
            
			sacd_close(sacd_reader);
        }
		else
			exit_main_flag=-1;

        

exit_main:
    fwprintf(stdout, L"\nProgram terminates!\n");
#ifndef _WIN32
            freopen(0, "w", stdout);
#endif
        if (fwide(stdout, -1) >= 0)
        {
            fwprintf(stderr, L"ERROR: Output not set to byte oriented.\n");
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
	
    return exit_main_flag;
}
