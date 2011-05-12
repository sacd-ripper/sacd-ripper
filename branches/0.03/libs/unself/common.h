/*
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 *
 */

#ifndef __COMMON_H__
#define __COMMON_H__


#include <stdio.h>

#ifdef __BIG_ENDIAN__
#define swap16(x) (x)
#define swap32(x) (x)
#define swap64(x) (x)
#else
#define swap16(x) ((((uint16_t)(x) & 0xff00) >> 8) | \
                  (((uint16_t)(x) & 0x00ff) << 8))
#define swap32(x) ((((uint32_t)(x) & 0xff000000) >> 24) | \
                  (((uint32_t)(x) & 0x00ff0000) >> 8)  | \
                  (((uint32_t)(x) & 0x0000ff00) << 8)  | \
                  (((uint32_t)(x) & 0x000000ff) << 24))
#define swap64(x) \
     ((((uint64_t)(x) & 0xff00000000000000ull) >> 56)  | \
       ((uint64_t)((x) & 0x00ff000000000000ull) >> 40) | \
       ((uint64_t)((x) & 0x0000ff0000000000ull) >> 24) | \
       ((uint64_t)((x) & 0x000000ff00000000ull) >> 8)  | \
       ((uint64_t)((x) & 0x00000000ff000000ull) << 8)  | \
       ((uint64_t)((x) & 0x0000000000ff0000ull) << 24) | \
       ((uint64_t)((x) & 0x000000000000ff00ull) << 40) | \
       ((uint64_t)((x) & 0x00000000000000ffull) << 56))
#endif


#define ERROR(err, msg) do {                    \
    perror (msg);                               \
    exit (err);                                 \
  } while(0);


#endif /* __COMMON_H__ */
