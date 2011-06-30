/*
 *    Copyright (C) 1998, 1999,  Espen Skoglund
 *    Copyright (C) 2000-2004  Haavard Kvaalen
 *
 * $Id: id3_header.h,v 1.7 2004/04/04 16:14:31 havard Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef ID3_HEADER_H
#define ID3_HEADER_H

/*
 * Layout for the ID3 tag header.
 */
#if 0
struct id3_taghdr {
	uint8_t th_version;
	uint8_t th_revision;
	uint8_t th_flags;
	uint32_t th_size;
};
#endif

/* Header size excluding "ID3" */
#define ID3_TAGHDR_SIZE 7 /* Size on disk */

#define ID3_THFLAG_USYNC	0x80
#define ID3_THFLAG_EXT		0x40
#define ID3_THFLAG_EXP		0x20

#define ID3_SET_SIZE28(size, a, b, c, d)	\
do {						\
    a = ((size & 0x800000) >> 23) | (((size & 0x7f000000) >> 24) << 1); \
    b = ((size & 0x8000) >> 15) | (((size & 0x7f0000) >> 16) << 1); \
    c = ((size & 0x80) >> 7) | (((size & 0x7f00) >> 8) << 1); \
    d = (size & 0x7f); \
} while (0)

#define ID3_GET_SIZE28(a, b, c, d)		\
(((a & 0x7f) << (24 - 3)) |			\
 ((b & 0x7f) << (16 - 2)) |			\
 ((c & 0x7f) << ( 8 - 1)) |			\
 ((d & 0x7f)))



/*
 * Layout for the extended header.
 */
#if 0
struct id3_exthdr {
	uint32_t eh_size;
	uint16_t eh_flags;
	uint32_t eh_padsize;
};
#endif

#define ID3_EXTHDR_SIZE 10

#define ID3_EHFLAG_V23_CRC	0x80
#define ID3_EHFLAG_V24_CRC	0x20



/*
 * Layout for the frame header.
 */
#if 0
struct id3_framehdr {
	uint32_t fh_id;
	uint32_t fh_size;
	uint16_t fh_flags;
};
#endif

#define ID3_FRAMEHDR_SIZE 10


#define ID3_FHFLAG_TAGALT	0x8000
#define ID3_FHFLAG_FILEALT	0x4000
#define ID3_FHFLAG_RO		0x2000
#define ID3_FHFLAG_COMPRESS	0x0080
#define ID3_FHFLAG_ENCRYPT	0x0040
#define ID3_FHFLAG_GROUP	0x0020


#define DEBUG_ID3
#ifdef DEBUG_ID3
#define id3_error(id3, error)		\
  (void) ( id3->id3_error_msg = error,	\
           printf( "Error %s, line %d: %s\n", __FILE__, __LINE__, error ) )


#else
#define id3_error(id3, error)		\
  (void) ( id3->id3_error_msg = error )

#endif

/*
 * Version 2.2.0 
 */

/*
 * Layout for the frame header.
 */
#if 0
struct id3_framehdr {
	char fh_id[3];
	uint8_t fh_size[3];
};
#endif

#define ID3_FRAMEHDR_SIZE_22 6

#endif /* ID3_HEADER_H */
