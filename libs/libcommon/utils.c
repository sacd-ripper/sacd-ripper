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

#include "utils.h"

char *substr(const char *pstr, int start, int numchars)
{
    static char pnew[512];
    strncpy(pnew, pstr + start, numchars);
    pnew[numchars] = '\0';
    return pnew;
}

void replace_double_space_with_single(char *str)
{
    int len = strlen(str) - 1;
    int i = 0;
    int spaces = 0;

    if(!str || len < 0) 
        return;
        
    while (i < len)
    {
        while(str[i] == ' ') 
        {
            spaces++; 
            i++;
        }
        while(str[i] != ' ' && str[i] != '\0') 
        {
            str[i - spaces] = str[i]; 
            i++;
        }
        if(str[i + spaces - 1] != '\0') 
        {
            str[i - spaces] = ' '; 
            spaces--; 
        } 
        else 
        {
            break;
        }
    }
    str[i - spaces] = '\0';
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
void trim_whitespace(char * str)
{
    int i;
    int pos = 0;
    int len = strlen(str);

    // trim leading space
    for (i = 0; i < len + 1; i++)
    {
        if (!isspace((int) str[i]) || (pos > 0))
        {
            str[pos] = str[i];
            pos++;
        }
    }

    // trim trailing space
    len = strlen(str);
    for (i = len - 1; i >= 0; i--)
    {
        if (!isspace((int) str[i]))
        {
            break;
        }
        str[i] = '\0';
    }
}
