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

#ifndef __UTILS_H__
#define __UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "log.h"

#define max(a, b)    (((a) > (b)) ? (a) : (b))
#define min(a, b)    (((a) < (b)) ? (a) : (b))

#define is_between_exclusive(num,lowerbound,upperbound) \
    ( ((num) > (lowerbound)) && ((num) < (upperbound)) )

#define is_between_inclusive(num,lowerbound,upperbound) \
    ( ((num) >= (lowerbound)) && ((num) <= (upperbound)) )

char *substr(const char *, int, int);

char *str_replace(const char *src, const char *from, const char *to);

// replaces doubles spaces with a single space
//
// str - the string to process
void replace_double_space_with_single(char *str);

// removes leading and trailing whitespace as defined by isspace()
//
// str - the string to trim
void trim_whitespace(char * str);

// removes all instances of bad characters from the string
//
// str - the string to trim
// bad - the sting containing all the characters to remove
void trim_chars(char * str, const char * bad);

void print_hex_dump(log_module_level_t level, const char *prefix_str,
                    int rowsize, int groupsize,
                    const void *buf, int len, int ascii);


#ifdef __cplusplus
};
#endif

#endif /* __UTILS_H__ */
