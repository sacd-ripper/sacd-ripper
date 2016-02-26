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
#include <string.h>
#include <sysutil/sysutil.h>
#include <sysutil/msg.h>
#include <sys/systime.h>
#include <sys/file.h>
#include <sys/atomic.h>
#include <sys/stat.h>
#include <errno.h>
#include <utils.h>
#include <fileutils.h>
#include <logging.h>

#include <sacd_reader.h>
#include <scarletbook_read.h>
#include <scarletbook_output.h>
#include <scarletbook_helpers.h>
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

static atomic_t stats_current_track;
static atomic_t stats_total_tracks;

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

static int check_disc_space(sacd_reader_t *sacd_reader, scarletbook_handle_t *handle, int ripping_flags)
{
    uint64_t needed_sectors = 0;

    if (ripping_flags & RIP_ISO)
    {
        needed_sectors = sacd_get_total_sectors(sacd_reader);
    }
    else if (has_two_channel(handle) && ripping_flags & RIP_2CH)
    {
        needed_sectors = get_two_channel(handle)->track_end - get_two_channel(handle)->track_start;
    }
    else if (has_both_channels(handle) && ripping_flags & RIP_MCH)
    {
        needed_sectors = get_multi_channel(handle)->track_end - get_multi_channel(handle)->track_start;
    }

    if (needed_sectors > output_device_sectors)
    {
        msgType  dialog_type;
        char     *message   = (char *) malloc(512);

        LOG(lm_main, LOG_ERROR, ("no enough disc space on %s (%llu), needs: %llu", output_device, output_device_sectors, needed_sectors));

        snprintf(message, 512, "Ripping aborted.\nYou do not have enough disc space on [%s (%.2fGB available)].", output_device, output_device_space);
        dialog_type = (MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
        dialog_action = 0;
        msgDialogOpen2(dialog_type, message, dialog_handler, NULL, NULL);
        while (!user_requested_exit() && !dialog_action)
        {
            sysUtilCheckCallback();
            flip();
        }
        msgDialogAbort();
        free(message);

        return 0;
    }
    return 1;
}

static void handle_status_update_track_callback(char *filename, int current_track, int total_tracks)
{
    sysAtomicSet(&stats_current_track, current_track);
    sysAtomicSet(&stats_total_tracks, total_tracks);
}

static void handle_status_update_progress_callback(uint32_t total_sectors, uint32_t total_sectors_processed,
                                 uint32_t current_file_total_sectors, uint32_t current_file_sectors_processed)
{
    sysAtomicSet(&stats_total_sectors, total_sectors);
    sysAtomicSet(&stats_total_sectors_processed, total_sectors_processed);
    sysAtomicSet(&stats_current_file_total_sectors, current_file_total_sectors);
    sysAtomicSet(&stats_current_file_sectors_processed, current_file_sectors_processed);
}

static int safe_fwprintf(FILE *stream, const wchar_t *format, ...)
{
    return 0;
} 
int start_ripping_gui(int ripping_flags)
{
    char *albumdir, *musicfilename, *file_path = 0;
    sacd_reader_t   *sacd_reader;
    scarletbook_handle_t *handle;
    scarletbook_output_t *output;
    msgType          dialog_type;
    int              area_idx, i, ret;

    uint32_t prev_upper_progress = 0;
    uint32_t prev_lower_progress = 0;
    uint32_t delta;

    int prev_current_track = 0;
    uint32_t prev_stats_total_sectors_processed = 0;
    uint32_t prev_stats_current_file_sectors_processed = 0;
    uint64_t tb_start, tb_freq;
    uint64_t tmp_total_ripping_sectors = 0;

    char progress_message[64];

    sysAtomicSet(&stats_total_sectors, 0);
    sysAtomicSet(&stats_total_sectors_processed, 0);
    sysAtomicSet(&stats_current_file_total_sectors, 0);
    sysAtomicSet(&stats_current_file_sectors_processed, 0); 
    sysAtomicSet(&stats_current_track, 0);
    sysAtomicSet(&stats_total_tracks, 0);

    sacd_reader = sacd_open("/dev_bdvd");
    if (sacd_reader) 
    {
        handle = scarletbook_open(sacd_reader, 0);

        if (check_disc_space(sacd_reader, handle, ripping_flags))
        {
            ret = sacd_authenticate(sacd_reader);
            if (ret != 0)
            {
                LOG(lm_main, LOG_ERROR, ("authentication failed: %x", ret));
            }

            // select the channel area
            area_idx = ((has_multi_channel(handle) && ripping_flags & RIP_MCH) || !has_two_channel(handle)) ? handle->mulch_area_idx : handle->twoch_area_idx;

            albumdir = get_album_dir(handle);

            output = scarletbook_output_create(handle, handle_status_update_track_callback, handle_status_update_progress_callback, safe_fwprintf);

            if (ripping_flags & RIP_ISO)
            {
                #define FAT32_SECTOR_LIMIT 2090000
                uint32_t total_sectors = sacd_get_total_sectors(sacd_reader);
                uint32_t sector_size = FAT32_SECTOR_LIMIT;
                uint32_t sector_offset = 0;
                if (total_sectors > FAT32_SECTOR_LIMIT)
                 {
                    musicfilename = (char *) malloc(512);
                    file_path = make_filename(output_device, 0, albumdir, "iso");
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
                {
                    file_path = make_filename(output_device, 0, albumdir, "iso");
                    scarletbook_output_enqueue_raw_sectors(output, 0, total_sectors, file_path, "iso");
                    free(file_path);
                }
                tmp_total_ripping_sectors = sacd_get_total_sectors(sacd_reader);
            }
            else 
            {
                // do not overwrite previous dump
                get_unique_dir(output_device, &albumdir);

                // fill the queue with items to rip
                for (i = 0; i < handle->area[area_idx].area_toc->track_count; i++) 
                {
                    musicfilename = get_music_filename(handle, area_idx, i, 0);
                    if (ripping_flags & RIP_DSF)
                    {
                        file_path = make_filename(output_device, albumdir, musicfilename, "dsf");
                        scarletbook_output_enqueue_track(output, area_idx, i, file_path, "dsf", 
                            1 /* always decode to DSD */);
                    }
                    else if (ripping_flags & RIP_DSDIFF)
                    {
                        file_path = make_filename(output_device, albumdir, musicfilename, "dff");
                        scarletbook_output_enqueue_track(output, area_idx, i, file_path, "dsdiff", 
                            ((ripping_flags & RIP_2CH_DST || ripping_flags & RIP_MCH_DST) ? 0 : 1));
                    }

                    tmp_total_ripping_sectors += handle->area[area_idx].area_tracklist_offset->track_length_lsn[i];

                    free(musicfilename);
                    free(file_path);
                }

                file_path = make_filename(output_device, albumdir, 0, 0);
                LOG(lm_main, LOG_NOTICE, ("setting output folder to: %s", file_path));
                recursive_mkdir(file_path, 0777);
                free(file_path);
            }

            scarletbook_output_start(output);

            tb_freq = sysGetTimebaseFrequency();
            tb_start = __gettime(); 

            {
                char *message = (char *) malloc(512);

                file_path = make_filename(output_device, albumdir, 0, 0);
                snprintf(message, 512, "Title: %s\nOutput: %s\nFormat: %s\nSize: %.2fGB\nArea: %s\nEncoding: %s", 
                        substr(albumdir, 0, 100), 
                        file_path, 
                        (ripping_flags & RIP_DSDIFF ? "DSDIFF" : (ripping_flags & RIP_DSF ? "DSF" : "ISO")),
                        ((double) ((tmp_total_ripping_sectors * SACD_LSN_SIZE) / 1073741824.00)),
                        (ripping_flags & RIP_2CH ? "2ch" : "mch"),
                        (ripping_flags & RIP_2CH_DST || ripping_flags & RIP_MCH_DST ? "DST" : (ripping_flags & RIP_ISO ? "DECRYPTED" : "DSD"))
                        );
                free(file_path);

                dialog_action = 0;
                dialog_type   = MSG_DIALOG_MUTE_ON | MSG_DIALOG_DOUBLE_PROGRESSBAR;
                msgDialogOpen2(dialog_type, message, dialog_handler, NULL, NULL);
                while (!user_requested_exit() && dialog_action == 0 && scarletbook_output_is_busy(output))
                {
                    uint32_t tmp_stats_total_sectors_processed = sysAtomicRead(&stats_total_sectors_processed);
                    uint32_t tmp_stats_total_sectors = sysAtomicRead(&stats_total_sectors);
                    uint32_t tmp_stats_current_file_sectors_processed = sysAtomicRead(&stats_current_file_sectors_processed);
                    uint32_t tmp_stats_current_file_total_sectors = sysAtomicRead(&stats_current_file_total_sectors);
                    int tmp_current_track = sysAtomicRead(&stats_current_track);

                    if (tmp_current_track != 0 && tmp_current_track != prev_current_track)
                    {
                        memset(progress_message, 0, 64);
       
                        musicfilename = get_music_filename(handle, area_idx, tmp_current_track - 1, 0);
                        // HACK: substr is not thread safe, but it's only used in this thread..
                        snprintf(progress_message, 63, "Track (%d/%d): [%s...]", tmp_current_track, sysAtomicRead(&stats_total_tracks), substr(musicfilename, 0, 40));
                        free(musicfilename);

                        msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX0);
                        msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, progress_message);
                        prev_upper_progress = 0;
                        prev_stats_current_file_sectors_processed = 0;

                        prev_current_track = tmp_current_track;
                    }

                    if (tmp_stats_total_sectors != 0 && prev_stats_total_sectors_processed != tmp_stats_total_sectors_processed)
                    {
                        delta = (tmp_stats_current_file_sectors_processed + (tmp_stats_current_file_sectors_processed - prev_stats_current_file_sectors_processed)) * 100 / tmp_stats_current_file_total_sectors - prev_upper_progress;
                        prev_upper_progress += delta;
                        msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, delta);

                        delta = (tmp_stats_total_sectors_processed + (tmp_stats_total_sectors_processed - prev_stats_total_sectors_processed)) * 100 / tmp_stats_total_sectors - prev_lower_progress;
                        prev_lower_progress += delta;
                        msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX1, delta);

                        snprintf(progress_message, 64, "Ripping %.1fMB/%.1fMB at %.2fMB/sec", 
                                ((float)(tmp_stats_current_file_sectors_processed * SACD_LSN_SIZE) / 1048576.00),
                                ((float)(tmp_stats_current_file_total_sectors * SACD_LSN_SIZE) / 1048576.00),
                                (float)((float) tmp_stats_total_sectors_processed * SACD_LSN_SIZE / 1048576.00) / (float)((__gettime() - tb_start) / (float)(tb_freq)));
                        
                        msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0, progress_message);
                        
                        prev_stats_total_sectors_processed = tmp_stats_total_sectors_processed;
                        prev_stats_current_file_sectors_processed = tmp_stats_current_file_sectors_processed;
                    }

                    sysUtilCheckCallback();
                    flip();
                }
                msgDialogAbort();
                free(message);
            }
            free(albumdir);

            scarletbook_output_destroy(output);
        }
        scarletbook_close(handle);
    }
    sacd_close(sacd_reader);

    if (user_requested_exit())
    {
        return 0;
    }
    else if (1)
    {
        dialog_type = (MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
        msgDialogOpen2(dialog_type, "ripping process completed.", dialog_handler, NULL, NULL);

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
