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

#ifndef ENDIANESS_H_INCLUDED
#define ENDIANESS_H_INCLUDED

#if defined(__BIG_ENDIAN__)

/* All bigendian systems are fine, just ignore the swaps. */
#define SWAP16(x)                  (void) (x)
#define SWAP32(x)                  (void) (x)
#define SWAP64(x)                  (void) (x)

#define hton16(x)                  (uint16_t) (x)
#define hton32(x)                  (uint32_t) (x)
#define hton64(x)                  (uint64_t) (x)

#define htole16(x)            \
    ((((x) & 0xff00) >> 8) | \
    (((x) & 0x00ff) << 8))
#define htole32(x)                 \
    ((((x) & 0xff000000) >> 24) | \
    (((x) & 0x00ff0000) >> 8) |  \
    (((x) & 0x0000ff00) << 8) |  \
    (((x) & 0x000000ff) << 24))
#define htole64(x)                            \
    ((((x) & 0xff00000000000000ULL) >> 56) | \
    (((x) & 0x00ff000000000000ULL) >> 40) | \
    (((x) & 0x0000ff0000000000ULL) >> 24) | \
    (((x) & 0x000000ff00000000ULL) >> 8) |  \
    (((x) & 0x00000000ff000000ULL) << 8) |  \
    (((x) & 0x0000000000ff0000ULL) << 24) | \
    (((x) & 0x000000000000ff00ULL) << 40) | \
    (((x) & 0x00000000000000ffULL) << 56))

#define MAKE_MARKER(a, b, c, d)    (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

#else

#define MAKE_MARKER(a, b, c, d)    ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

/* For __FreeBSD_version */
#if defined(HAVE_SYS_PARAM_H)
#include <sys/param.h>
#endif

#if defined(__linux__) || defined(__GLIBC__)
#include <byteswap.h>
#define SWAP16(x)    x = bswap_16(x)
#define SWAP32(x)    x = bswap_32(x)
#define SWAP64(x)    x = bswap_64(x)
#define hton16(x)    bswap_16(x)
#define hton32(x)    bswap_32(x)
#define hton64(x)    bswap_64(x)

#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define SWAP16(x)    x = OSSwapBigToHostInt16(x)
#define SWAP32(x)    x = OSSwapBigToHostInt32(x)
#define SWAP64(x)    x = OSSwapBigToHostInt64(x)
#define hton16(x)    OSSwapBigToHostInt16(x)
#define hton32(x)    OSSwapBigToHostInt32(x)
#define hton64(x)    OSSwapBigToHostInt64(x)

#elif defined(__NetBSD__)
#include <sys/endian.h>
#define SWAP16(x)    BE16TOH(x)
#define SWAP32(x)    BE32TOH(x)
#define SWAP64(x)    BE64TOH(x)
#define hton16(x)    BE16TOH(x)
#define hton32(x)    BE32TOH(x)
#define hton64(x)    BE64TOH(x)

#elif defined(__OpenBSD__)
#include <sys/endian.h>
#define SWAP16(x)    x = swap16(x)
#define SWAP32(x)    x = swap32(x)
#define SWAP64(x)    x = swap64(x)
#define hton16(x)    swap16(x)
#define hton32(x)    swap32(x)
#define hton64(x)    swap64(x)

#elif defined(__FreeBSD__) && __FreeBSD_version >= 470000
#include <sys/endian.h>
#define SWAP16(x)    x = be16toh(x)
#define SWAP32(x)    x = be32toh(x)
#define SWAP64(x)    x = be64toh(x)
#define hton16(x)    be16toh(x)
#define hton32(x)    be32toh(x)
#define hton64(x)    be64toh(x)

/* This is a slow but portable implementation, it has multiple evaluation
 * problems so beware.
 * Old FreeBSD's and Solaris don't have <byteswap.h> or any other such
 * functionality!
 */

#elif defined(__FreeBSD__) || defined(__sun) || defined(__bsdi__) || defined(WIN32) || defined(__CYGWIN__) || defined(__BEOS__)
#define hton16(x)            \
    ((((x) & 0xff00) >> 8) | \
     (((x) & 0x00ff) << 8))
#define hton32(x)                 \
    ((((x) & 0xff000000) >> 24) | \
     (((x) & 0x00ff0000) >> 8) |  \
     (((x) & 0x0000ff00) << 8) |  \
     (((x) & 0x000000ff) << 24))
#define hton64(x)                            \
    ((((x) & 0xff00000000000000ULL) >> 56) | \
     (((x) & 0x00ff000000000000ULL) >> 40) | \
     (((x) & 0x0000ff0000000000ULL) >> 24) | \
     (((x) & 0x000000ff00000000ULL) >> 8) |  \
     (((x) & 0x00000000ff000000ULL) << 8) |  \
     (((x) & 0x0000000000ff0000ULL) << 24) | \
     (((x) & 0x000000000000ff00ULL) << 40) | \
     (((x) & 0x00000000000000ffULL) << 56))

#define SWAP16(x)    x = (hton16(x))
#define SWAP32(x)    x = (hton32(x))
#define SWAP64(x)    x = (hton64(x))

#else

/* If there isn't a header provided with your system with this functionality
 * add the relevant || define( ) to the portable implementation above.
 */
#error "You need to add endian swap macros for you're system"

#endif

#endif /* __BIG_ENDIAN__ */

#ifndef __BIG_ENDIAN__

#ifndef htole16
#define htole16(x)                  (uint16_t) (x)
#endif
#ifndef htole32
#define htole32(x)                  (uint32_t) (x)
#endif
#ifndef htole64
#define htole64(x)                  (uint64_t) (x)
#endif

#endif

#endif /* ENDIANESS_H_INCLUDED */
