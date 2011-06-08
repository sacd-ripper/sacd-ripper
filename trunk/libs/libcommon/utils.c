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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "utils.h"

char *substr(const char *pstr, int start, int numchars)
{
    static char pnew[512];
    strncpy(pnew, pstr + start, numchars);
    pnew[numchars] = '\0';
    return pnew;
}

char *str_replace(const char *src, const char *from, const char *to)
{
    size_t size    = strlen(src) + 1;
    size_t fromlen = strlen(from);
    size_t tolen   = strlen(to);
    char *value = malloc(size);
    char *dst = value;
    if (value != NULL)
    {
        for ( ;; )
        {
            const char *match = strstr(src, from);
            if ( match != NULL )
            {
                size_t count = match - src;
                char *temp;
                size += tolen - fromlen;
                temp = realloc(value, size);
                if ( temp == NULL )
                {
                    free(value);
                    return NULL;
                }
                dst = temp + (dst - value);
                value = temp;
                memmove(dst, src, count);
                src += count;
                dst += count;
                memmove(dst, to, tolen);
                src += fromlen;
                dst += tolen;
            }
            else /* No match found. */
            {
                strcpy(dst, src);
                break;
            }
        }
    }
    return value;
}

void replace_double_space_with_single(char *str)
{
    char * ret = str_replace(str, "  ", " ");
    strcpy(str, ret);
    free(ret);
}

// removes all instances of bad characters from the string
//
// str - the string to trim
// bad - the sting containing all the characters to remove
void trim_chars(char * str, const char * bad)
{
    int      i;
    int      pos;
    int      len = strlen(str);
    unsigned b;

    for (b = 0; b < strlen(bad); b++)
    {
        pos = 0;
        for (i = 0; i < len + 1; i++)
        {
            if (str[i] != bad[b])
            {
                str[pos] = str[i];
                pos++;
            }
        }
    }
}

// removes leading and trailing whitespace as defined by isspace()
//
// str - the string to trim
void trim_whitespace(char * s) 
{
    char * p = s;
    int l = strlen(p);

    while(isspace((int) p[l - 1])) p[--l] = 0;
    while(* p && isspace((int) *p)) ++p, --l;

    memmove(s, p, l + 1);
}
