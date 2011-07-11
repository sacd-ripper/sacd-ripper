/**
 * SACD Ripper - http://code.google.com/write_ptr/sacd-ripper/
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

#include "logging.h"
#include "fileutils.h"
#include "charset.h"
#include "utils.h"

// construct a filename from various parts
//
// path - the path the file is placed in (don't include a trailing '/')
// dir - the parent directory of the file (don't include a trailing '/')
// file - the filename
// extension - the suffix of a file (don't include a leading '.')
//
// NOTE: caller must free the returned string!
// NOTE: any of the parameters may be NULL to be omitted
char * make_filename(const char * path, const char * dir, const char * file, const char * extension)
{
    int  len   = 1;
    char * ret = NULL;
    int  pos   = 0;

    if (path)
    {
        len += strlen(path) + 1;
    }
    if (dir)
    {
        len += strlen(dir) + 1;
    }
    if (file)
    {
        len += strlen(file);
    }
    if (extension)
    {
        len += strlen(extension) + 1;
    }

    ret = malloc(sizeof(char) * (len + 1));
    if (ret == NULL)
        LOG(lm_main, LOG_ERROR, ("malloc(sizeof(char) * len) failed. Out of memory."));

    if (path)
    {
        strncpy(&ret[pos], path, strlen(path));
        pos     += strlen(path);
        ret[pos] = '/';
        pos++;
    }
    if (dir)
    {
        char *tmp_dir = strdup(dir);
        sanitize_filepath(tmp_dir);
        strncpy(&ret[pos], tmp_dir, strlen(tmp_dir));
        pos     += strlen(tmp_dir);
        ret[pos] = '/';
        pos++;
        free(tmp_dir);
    }
    if (file)
    {
        char * tmp_file = strdup(file);
        sanitize_filename(tmp_file);
        strncpy(&ret[pos], tmp_file, strlen(tmp_file));
        pos += strlen(tmp_file);
        free(tmp_file);
    }
    if (extension)
    {
        ret[pos] = '.';
        pos++;
        strncpy(&ret[pos], extension, strlen(extension));
        pos += strlen(extension);
    }
    ret[pos] = '\0';

    return ret;
}

// substitute various items into a formatted string (similar to printf)
//
// format - the format of the filename
// tracknum - gets substituted for %N in format
// year - gets substituted for %Y in format
// artist - gets substituted for %A in format
// album - gets substituted for %L in format
// title - gets substituted for %T in format
//
// NOTE: caller must free the returned string!
char * parse_format(const char * format, int tracknum, const char * year, const char * artist, const char * album, const char * title)
{
    unsigned i     = 0;
    int      len   = 0;
    char     * ret = NULL;
    int      pos   = 0;

    for (i = 0; i < strlen(format); i++)
    {
        if ((format[i] == '%') && (i + 1 < strlen(format)))
        {
            switch (format[i + 1])
            {
            case 'A':
                if (artist)
                    len += strlen(artist);
                break;
            case 'L':
                if (album)
                    len += strlen(album);
                break;
            case 'N':
                if ((tracknum > 0) && (tracknum < 100))
                    len += 2;
                break;
            case 'Y':
                if (year)
                    len += strlen(year);
                break;
            case 'T':
                if (title)
                    len += strlen(title);
                break;
            case '%':
                len += 1;
                break;
            }

            i++;             // skip the character after the %
        }
        else
        {
            len++;
        }
    }

    ret = malloc(sizeof(char) * (len + 1));
    if (ret == NULL)
        LOG(lm_main, LOG_ERROR, ("malloc(sizeof(char) * (len+1)) failed. Out of memory."));

    for (i = 0; i < strlen(format); i++)
    {
        if ((format[i] == '%') && (i + 1 < strlen(format)))
        {
            switch (format[i + 1])
            {
            case 'A':
                if (artist)
                {
                    strncpy(&ret[pos], artist, strlen(artist));
                    pos += strlen(artist);
                }
                break;
            case 'L':
                if (album)
                {
                    strncpy(&ret[pos], album, strlen(album));
                    pos += strlen(album);
                }
                break;
            case 'N':
                if ((tracknum > 0) && (tracknum < 100))
                {
                    ret[pos]     = '0' + ((int) tracknum / 10);
                    ret[pos + 1] = '0' + (tracknum % 10);
                    pos         += 2;
                }
                break;
            case 'Y':
                if (year)
                {
                    strncpy(&ret[pos], year, strlen(year));
                    pos += strlen(year);
                }
                break;
            case 'T':
                if (title)
                {
                    strncpy(&ret[pos], title, strlen(title));
                    pos += strlen(title);
                }
                break;
            case '%':
                ret[pos] = '%';
                pos     += 1;
            }

            i++;             // skip the character after the %
        }
        else
        {
            ret[pos] = format[i];
            pos++;
        }
    }
    ret[pos] = '\0';

    return ret;
}

#ifdef __lv2ppu__
#include <sys/stat.h>
#include <sys/file.h>
#endif

// Uses mkdir() for every component of the path
// and returns if any of those fails with anything other than EEXIST.
int recursive_mkdir(char* path_and_name, mode_t mode)
{
    int  count;
    int  path_and_name_length = strlen(path_and_name);
    int  rc;
    char charReplaced;

    for (count = 0; count < path_and_name_length; count++)
    {
        if (path_and_name[count] == '/')
        {
            charReplaced             = path_and_name[count + 1];
            path_and_name[count + 1] = '\0';

#ifdef _WIN32
            {
                wchar_t *wide_path_and_name = (wchar_t *) charset_convert(path_and_name, strlen(path_and_name), "UTF-8", "UCS-2-INTERNAL");
                rc = _wmkdir(wide_path_and_name);
                free(wide_path_and_name);
            }
#else
            rc = mkdir(path_and_name, mode);
#endif
#ifdef __lv2ppu__
            sysFsChmod(path_and_name, S_IFMT | 0777); 
#endif
            path_and_name[count + 1] = charReplaced;

            if (rc != 0 && !(errno == EEXIST || errno == EISDIR || errno == EACCES || errno == EROFS))
                return rc;
        }
    }

    // in case the path doesn't have a trailing slash:
#ifdef _WIN32
    {
        wchar_t *wide_path_and_name = (wchar_t *) charset_convert(path_and_name, strlen(path_and_name), "UTF-8", "UCS-2-INTERNAL");
        rc = _wmkdir(wide_path_and_name);
        free(wide_path_and_name);
    }
#else
    rc = mkdir(path_and_name, mode);
#endif
#ifdef __lv2ppu__
    sysFsChmod(path_and_name, S_IFMT | 0777);
#endif
    return rc;
}

// Uses mkdir() for every component of the path except the last one,
// and returns if any of those fails with anything other than EEXIST.
int recursive_parent_mkdir(char* path_and_name, mode_t mode)
{
    int count;
    int have_component = 0;
    int rc             = 1; // guaranteed fail unless mkdir is called

    // find the last component and cut it off
    for (count = strlen(path_and_name) - 1; count >= 0; count--)
    {
        if (path_and_name[count] != '/')
            have_component = 1;

        if (path_and_name[count] == '/' && have_component)
        {
            path_and_name[count] = 0;
#ifdef _WIN32
            {
                wchar_t *wide_path_and_name = (wchar_t *) charset_convert(path_and_name, strlen(path_and_name), "UTF-8", "UCS-2-INTERNAL");
                rc = _wmkdir(wide_path_and_name);
                free(wide_path_and_name);
            }
#else
            rc = mkdir(path_and_name, mode);
#endif
            path_and_name[count] = '/';
        }
    }

    return rc;
}

void get_unique_dir(char *device, char **dir)
{
    struct stat stat_dir;
    int dir_exists, count = 1;
    char *dir_org = strdup(*dir);
    char *device_dir = make_filename(device, *dir, 0, 0);
    dir_exists = (stat(device ? device_dir : *dir, &stat_dir) == 0);
    free(device_dir);
    while (dir_exists)
    {
        int len = strlen(dir_org) + 10;
        char *dir_copy = (char *) malloc(len);
        snprintf(dir_copy, len, "%s (%d)", dir_org, count++);
        free(*dir);
        *dir = dir_copy;
        device_dir = make_filename(device, dir_copy, 0, 0);
        dir_exists = (stat(device ? device_dir : dir_copy, &stat_dir) == 0);
        free(device_dir);
    }
    free(dir_org);
}

void sanitize_filename(char *f)
{
    const char unsafe_chars[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
                                 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a,
                                 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x22, 0x2a, 0x2f, 0x3a, 0x3c, 0x3e, 0x3f, 0x5c,
                                 0x7c, 0x7f, 0x00};

    char *c = f;

    if (!c || strlen(c) == 0)
        return;

    for (; *c; c++)
    {
        if (strchr(unsafe_chars, *c))
            *c = ' ';
    }
    replace_double_space_with_single(f);
    trim_whitespace(f);
}

static void trim_dots(char * s) 
{
    char * p = s;
    int l = strlen(p);

    while(p[l - 1] == '.') p[--l] = 0;
    while(*p && *p == '.') ++p, --l;

    memmove(s, p, l + 1);
}

void sanitize_filepath(char *f)
{
    const char unsafe_chars[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
                                 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a,
                                 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x22, 0x2a, 0x3a, 0x3c, 0x3e, 0x3f, 0x5c, 0x7c, 
                                 0x7f, 0x00};

    char *c = f;

    if (!c || strlen(c) == 0)
        return;

    for (; *c; c++)
    {
        if (strchr(unsafe_chars, *c))
            *c = ' ';
    }
    replace_double_space_with_single(f);

    trim_whitespace(f);
    trim_dots(f);
    trim_whitespace(f);
}
