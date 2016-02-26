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
#include <inttypes.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include <charset.h>
#include <version.h>
#include <utils.h>

#include "cuesheet.h"

static char *cue_escape(const char *src) 
{
    static char ret[512];
    char *s = str_replace(src, "\"", "\\\"");
    strcpy(ret, s);
    free(s);
    trim_whitespace(ret);
    return ret;
}

int write_cue_sheet(scarletbook_handle_t *handle, const char *filename, int area, char *cue_filename)
{
    FILE *fd;

#ifdef _WIN32
    {
        wchar_t *wide_filename = (wchar_t *) charset_convert(cue_filename, strlen(cue_filename), "UTF-8", "UCS-2-INTERNAL");
        fd = _wfopen(wide_filename, L"wb");
        free(wide_filename);
    }
#else
    fd = fopen(cue_filename, "wb");
#endif
    if (fd == 0)
    {
        return -1;
    }

    // Write UTF-8 BOM
    fputc(0xef, fd);
    fputc(0xbb, fd);
    fputc(0xbf, fd);
    fprintf(fd, "\nREM File created by SACD Extract, version: " SACD_RIPPER_VERSION_STRING "\n");

    if (handle->master_toc->disc_genre[0].genre > 0)
    {
        fprintf(fd, "REM GENRE %s\n", album_genre[handle->master_toc->disc_genre[0].genre]);
    }

    if (handle->master_toc->disc_date_year)
    {
        fprintf(fd, "REM DATE %04d-%02d-%02d\n", handle->master_toc->disc_date_year
                                               , handle->master_toc->disc_date_month
                                               , handle->master_toc->disc_date_day);
    }

    if (handle->master_text.disc_artist)
    {
        fprintf(fd, "PERFORMER \"%s\"\n", cue_escape(handle->master_text.disc_artist));
    }

    if (handle->master_text.disc_title)
    {
        fprintf(fd, "TITLE \"%s\"\n", cue_escape(handle->master_text.disc_title));
    }

    if (strlen(handle->master_toc->disc_catalog_number) > 0)
    {
        fprintf(fd, "CATALOG %s\n", cue_escape(substr(handle->master_toc->disc_catalog_number, 0, 16)));
    }

    fprintf(fd, "FILE \"%s\" WAVE\n", cue_escape(filename));
    {
        int track, track_count = handle->area[area].area_toc->track_count;
        uint64_t prev_abs_end = 0;

        for (track = 0; track < track_count; track++)
        {
            area_tracklist_time_t *time = &handle->area[area].area_tracklist_time->start[track];

            fprintf(fd, "  TRACK %02d AUDIO\n", track + 1);
            
            if (handle->area[area].area_track_text[track].track_type_title)
            {
                fprintf(fd, "      TITLE \"%s\"\n", cue_escape(handle->area[area].area_track_text[track].track_type_title));
            }

            if (handle->area[area].area_track_text[track].track_type_performer)
            {
                fprintf(fd, "      PERFORMER \"%s\"\n", cue_escape(handle->area[area].area_track_text[track].track_type_performer));
            }

            if (*handle->area[area].area_isrc_genre->isrc[track].country_code)
            {
                fprintf(fd, "      ISRC %s\n", cue_escape(substr(handle->area[area].area_isrc_genre->isrc[track].country_code, 0, 12)));
            }

            if ((uint64_t) TIME_FRAMECOUNT(&handle->area[area].area_tracklist_time->start[track]) > prev_abs_end)
            {
                int prev_sec = (int) (prev_abs_end / SACD_FRAME_RATE);

                fprintf(fd, "      INDEX 00 %02d:%02d:%02d\n", prev_sec / 60, (int) prev_sec % 60, (int) prev_abs_end % SACD_FRAME_RATE);
                fprintf(fd, "      INDEX 01 %02d:%02d:%02d\n", time->minutes, time->seconds, time->frames);
            }
            else
            {
                fprintf(fd, "      INDEX 01 %02d:%02d:%02d\n", time->minutes, time->seconds, time->frames);
            }

            prev_abs_end = TIME_FRAMECOUNT(&handle->area[area].area_tracklist_time->start[track]) + 
                             TIME_FRAMECOUNT(&handle->area[area].area_tracklist_time->duration[track]);
        }
    }

    fclose(fd);

    return 0;
}
