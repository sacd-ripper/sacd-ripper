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
#include <sysutil/sysutil.h>
#include <sysutil/msg.h>
#include <sys/systime.h>
#include <sys/file.h>
#include <sys/atomic.h>
#include <utils.h>
#include <fileutils.h>
#include <logging.h>

#include <sacd_reader.h>
#include <scarletbook_read.h>
#include <scarletbook_output.h>
#include <sac_accessor.h>

#include "ripping.h"
#include "output_device.h"
#include "exit_handler.h"
#include "rsxutil.h"

static int               dialog_action = 0;

static atomic_t stats_total_sectors;
static atomic_t stats_total_sectors_processed;
static atomic_t stats_current_file_total_sectors;               // total amount of block to process
static atomic_t stats_current_file_sectors_processed; 

static atomic_t selected_progress_message;
static char     progress_message_upper[2][64];
static char     progress_message_lower[2][64];

char *get_album_dir(scarletbook_handle_t *handle)
{
    char disc_artist[60];
    char disc_album_title[60];
    char disc_album_year[5];
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
        return parse_format("%A - %L", 0, disc_album_year, disc_artist, disc_album_title, NULL);
    else if (strlen(disc_artist) > 0)
        return parse_format("%A", 0, disc_album_year, disc_artist, disc_album_title, NULL);
    else if (strlen(disc_album_title) > 0)
        return parse_format("%L", 0, disc_album_year, disc_artist, disc_album_title, NULL);
    else
        return parse_format("Unknown Album", 0, disc_album_year, disc_artist, disc_album_title, NULL);
}

char *get_music_filename(scarletbook_handle_t *handle, int area, int track)
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

static void dialog_handler(msgButton button, void *user_data)
{
    switch (button)
    {
    case MSG_DIALOG_BTN_OK:
        dialog_action = 1;
        break;
    case MSG_DIALOG_BTN_NO:
    case MSG_DIALOG_BTN_ESCAPE:
        dialog_action = 2;
        break;
    case MSG_DIALOG_BTN_NONE:
        dialog_action = -1;
        break;
    default:
        break;
    }
}

int start_ripping_gui(void)
{
    char *albumdir, *musicfilename, *file_path;
    sacd_reader_t   *sacd_reader;
    scarletbook_handle_t *handle;
    msgType          dialog_type;
    int              area_idx, i;

    //uint32_t prev_upper_progress = 0;
    uint32_t prev_lower_progress = 0;
    uint32_t delta;

    uint32_t prev_stats_total_sectors_processed = 0;
    uint64_t tb_start, tb_freq;

    atomic_set(&selected_progress_message, 0);
    atomic_set(&stats_total_sectors, 0);
    atomic_set(&stats_total_sectors_processed, 0);
    atomic_set(&stats_current_file_total_sectors, 0);
    atomic_set(&stats_current_file_sectors_processed, 0); 

    memset(progress_message_upper, 0, sizeof(progress_message_upper));
    memset(progress_message_lower, 0, sizeof(progress_message_lower));

    sacd_reader = sacd_open("/dev_bdvd");
    if (sacd_reader) 
    {
        handle = scarletbook_open(sacd_reader, 0);

        // select the channel area
        area_idx = (has_multi_channel(handle) && 0/*opts.multi_channel*/) ? handle->mulch_area_idx : handle->twoch_area_idx;

        albumdir      = get_album_dir(handle);
        recursive_mkdir(albumdir, 0666);

        // fill the queue with items to rip
        for (i = 0; i < handle->area[area_idx].area_toc->track_count; i++) 
        {
            musicfilename = get_music_filename(handle, area_idx, i);
            file_path     = make_filename(".", albumdir, musicfilename, "dff");
            queue_track_to_rip(area_idx, i, file_path, "dsdiff", 
                handle->area[area_idx].area_tracklist_offset->track_start_lsn[i], 
                handle->area[area_idx].area_tracklist_offset->track_length_lsn[i], 
                handle->area[area_idx].area_toc->frame_format == FRAME_FORMAT_DST);

            free(musicfilename);
            free(file_path);
        }

        //init_stats(handle_status_update_callback);
        start_ripping(handle);

        tb_freq = sysGetTimebaseFrequency();
        tb_start = __gettime(); 

        dialog_action = 0;
        dialog_type   = MSG_DIALOG_MUTE_ON | MSG_DIALOG_DOUBLE_PROGRESSBAR;
        msgDialogOpen2(dialog_type, "Copying to:...", dialog_handler, NULL, NULL);
        while (!user_requested_exit() && dialog_action == 0) // TODO: && atomic_read(&stop_processing) == 0
        {
            uint32_t tmp_stats_total_sectors_processed = atomic_read(&stats_total_sectors_processed);
            uint32_t tmp_stats_total_sectors = atomic_read(&stats_total_sectors);
            if (tmp_stats_total_sectors != 0 && prev_stats_total_sectors_processed != tmp_stats_total_sectors_processed)
            {
                //msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, delta);

                delta = (tmp_stats_total_sectors_processed + (tmp_stats_total_sectors_processed - prev_stats_total_sectors_processed)) * 100 / tmp_stats_total_sectors - prev_lower_progress;
                prev_lower_progress += delta;
                msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX1, delta);

                snprintf(progress_message_lower[0], 64, "transfer rate: (%8.3f MB/sec)", (float)((float) tmp_stats_total_sectors_processed * SACD_LSN_SIZE / 1048576.00) / (float)((__gettime() - tb_start) / (float)(tb_freq)));
                
                //msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0, progress_message_upper[atomic_read(&selected_progress_message)]);
                msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, progress_message_lower[0]);
                
                prev_stats_total_sectors_processed = tmp_stats_total_sectors_processed;
            }

            sysUtilCheckCallback();
            flip();
        }
        msgDialogAbort();

        // user requested abort
        if (dialog_action != 0)
            stop_ripping(handle);

        scarletbook_close(handle);

        free(albumdir);
    }
    sacd_close(sacd_reader);

    // did we complete?
    if (1)
    {
        dialog_type = (MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
        msgDialogOpen2(dialog_type, "Ripping was successful!", dialog_handler, NULL, NULL);

        dialog_action = 0;
        while (!dialog_action && !user_requested_exit())
        {
            sysUtilCheckCallback();
            flip();
        }
        msgDialogAbort();
    }

    return 0;
}
