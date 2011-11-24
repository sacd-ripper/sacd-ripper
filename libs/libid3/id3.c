/*
 *    Copyright (C) 1999, 2001,  Espen Skoglund
 *    Copyright (C) 2000 - 2004  Haavard Kvaalen
 *
 * $Id: id3.c,v 1.14 2004/04/04 16:14:31 havard Exp $
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

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#ifdef _WIN32
#include <io.h>
#endif
#include <sys/types.h>

#include <utils.h>

#include "id3.h"
#include "id3_header.h"


/*
**
** Functions for accessing the ID3 tag using a memory pointer.
**
*/

/*
 * Function id3_seek_mem (id3, offset)
 *
 *    Seek `offset' bytes forward in the indicated ID3-tag.  Return 0
 *    upon success, or -1 if an error occured.
 *
 */
static int id3_seek_mem(struct id3_tag *id3, int offset)
{
	if (id3->id3_pos + offset > id3->id3_tagsize ||
	    id3->id3_pos + offset < 0)
	{
		id3_error(id3, "seeking beyond tag boundary");
		return -1;
	}
	id3->s.me.id3_ptr = (char *) id3->s.me.id3_ptr + offset;
	id3->id3_pos += offset;

	return 0;
}


/*
 * Function id3_read_mem (id3, buf, size)
 *
 *    Read `size' bytes from indicated ID3-tag.  If `buf' is non-NULL,
 *    read into that buffer.  Return a pointer to the data which was
 *    read, or NULL upon error.
 *
 */
static void *id3_read_mem(struct id3_tag *id3, void *buf, int size)
{
	void *ret = id3->s.me.id3_ptr;

	/*
	 * Check boundary.
	 */
	if (id3->id3_pos + size > id3->id3_tagsize)
		return NULL;

	/*
	 * If buffer is non-NULL, we have to copy the data.
	 */
	if (buf != NULL)
	{
		if (size > ID3_FD_BUFSIZE)
			return NULL;
		memcpy(buf, id3->s.me.id3_ptr, size);
	}

	/*
	 * Update memory pointer.
	 */
	id3->s.me.id3_ptr = (char *) id3->s.me.id3_ptr + size;
	id3->id3_pos += size;

	return ret;
}


/*
**
** Functions for accessing the ID3 tag using a file descriptor.
**
*/

/*
 * Function id3_seek_fd (id3, offset)
 *
 *    Seek `offset' bytes forward in the indicated ID3-tag.  Return 0
 *    upon success, or -1 if an error occured.
 *
 */
static int id3_seek_fd(struct id3_tag *id3, int offset)
{
	/*
	 * Check boundary.
	 */
	if (id3->id3_pos + offset > id3->id3_tagsize ||
	    id3->id3_pos + offset < 0)
		return -1;

	if (lseek(id3->s.fd.id3_fd, offset, SEEK_CUR) == -1)
	{
		id3_error(id3, "seeking beyond tag boundary");
		return -1;
	}
	id3->id3_pos += offset;

	return 0;
}


/*
 * Function id3_read_fd (id3, buf, size)
 *
 *    Read `size' bytes from indicated ID3-tag.  If `buf' is non-NULL,
 *    read into that buffer.  Return a pointer to the data which was
 *    read, or NULL upon error.
 *
 */
static void *id3_read_fd(struct id3_tag *id3, void *buf, int size)
{
	int done = 0;

	/*
	 * Check boundary.
	 */
	if (id3->id3_pos + size > id3->id3_tagsize)
		return NULL;

	/*
	 * If buffer is NULL, we use the default buffer.
	 */
	if (buf == NULL)
	{
		if (size > ID3_FD_BUFSIZE)
			return NULL;
		buf = id3->s.fd.id3_buf;
	}

	/*
	 * Read until we have slurped as much data as we wanted.
	 */
	while (done < size)
	{
		char *buffer = (char *)buf + done;
		int ret;

		/*
		 * Try reading from file.
		 */
		ret = read(id3->s.fd.id3_fd, buffer, size);
		if (ret <= 0)
		{
			id3_error(id3, "read(2) failed");
			return NULL;
		}

		id3->id3_pos += ret;
		done += ret;
	}

	return buf;
}


/*
**
** Functions for accessing the ID3 tag using a file pointer.
**
*/

/*
 * Function id3_seek_fp (id3, offset)
 *
 *    Seek `offset' bytes forward in the indicated ID3-tag.  Return 0
 *    upon success, or -1 if an error occured.
 *
 */
static int id3_seek_fp(struct id3_tag *id3, int offset)
{
	/*
	 * Check boundary.
	 */
	if (id3->id3_pos + offset > id3->id3_tagsize ||
	    id3->id3_pos + offset < 0)
		return -1;

	if (offset > 0)
	{
		/*
		 * If offset is positive, we use fread() instead of fseek().  This
		 * is more robust with respect to streams.
		 */
		char buf[64];
		int r, remain = offset;

		while (remain > 0)
		{
			int size = min(64, remain);
			r = fread(buf, 1, size, id3->s.fp.id3_fp);
			if (r == 0)
			{
				id3_error(id3, "fread() failed");
				return -1;
			}
			remain -= r;
		}
	}
	else
	{
		/*
		 * If offset is negative, we ahve to use fseek().  Let us hope
		 * that it works.
		 */
		if (fseek(id3->s.fp.id3_fp, offset, SEEK_CUR) == -1)
		{
			id3_error(id3, "seeking beyond tag boundary");
			return -1;
		}
	}
	id3->id3_pos += offset;

	return 0;
}


/*
 * Function id3_read_fp (id3, buf, size)
 *
 *    Read `size' bytes from indicated ID3-tag.  If `buf' is non-NULL,
 *    read into that buffer.  Return a pointer to the data which was
 *    read, or NULL upon error.
 *
 */
static void *id3_read_fp(struct id3_tag *id3, void *buf, int size)
{
	int ret;

	/*
	 * Check boundary.
	 */
	if (id3->id3_pos + size > id3->id3_tagsize)
		size = id3->id3_tagsize - id3->id3_pos;

	/*
	 * If buffer is NULL, we use the default buffer.
	 */
	if (buf == NULL)
	{
		if (size > ID3_FD_BUFSIZE)
			return NULL;
		buf = id3->s.fd.id3_buf;
	}

	/*
	 * Try reading from file.
	 */
	ret = fread(buf, 1, size, id3->s.fp.id3_fp);
	if (ret != size)
	{
		id3_error(id3, "fread() failed");
		return NULL;
	}

	id3->id3_pos += ret;

	return buf;
}




/*
 * Function id3_open_mem (ptr, flags)
 *
 *    Open an ID3 tag using a memory pointer.  Return a pointer to a
 *    structure describing the ID3 tag, or NULL if an error occured.
 *
 */
struct id3_tag *id3_open_mem(void *ptr, int flags)
{
	struct id3_tag *id3;

	/*
	 * Allocate ID3 structure.
	 */
	id3 = calloc(sizeof (struct id3_tag), 1);

	/*
	 * Initialize access pointers.
	 */
	id3->id3_seek = id3_seek_mem;
	id3->id3_read = id3_read_mem;

	id3->id3_oflags = flags;
	id3->id3_type = ID3_TYPE_MEM;
	id3->id3_pos = 0;
	id3->s.me.id3_ptr = ptr;

    /*
	 * Initialize frames.
	 */
    INIT_LIST_HEAD(&id3->id3_frame);

	/*
	 * Try reading ID3 tag.
	 */
	if (id3_read_tag(id3) == -1)
	{
		if (~flags & ID3_OPENF_CREATE)
			goto Return_NULL;
		id3_init_tag(id3);
	}

	return id3;

 Return_NULL:
	free(id3);
	return NULL;
}


/*
 * Function id3_open_fd (fd, flags)
 *
 *    Open an ID3 tag using a file descriptor.  Return a pointer to a
 *    structure describing the ID3 tag, or NULL if an error occured.
 *
 */
struct id3_tag *id3_open_fd(int fd, int flags)
{
	struct id3_tag *id3;

	/*
	 * Allocate ID3 structure.
	 */
	id3 = calloc(sizeof(struct id3_tag), 1);

	/*
	 * Initialize access pointers.
	 */
	id3->id3_seek = id3_seek_fd;
	id3->id3_read = id3_read_fd;

	id3->id3_oflags = flags;
	id3->id3_type = ID3_TYPE_FD;
	id3->id3_pos = 0;
	id3->s.fd.id3_fd = fd;

	/*
	 * Allocate buffer to hold read data.
	 */
	id3->s.fd.id3_buf = malloc(ID3_FD_BUFSIZE);

    /*
	 * Initialize frames.
	 */
    INIT_LIST_HEAD(&id3->id3_frame);

	/*
	 * Try reading ID3 tag.
	 */
	if (id3_read_tag(id3) == -1)
	{
		if (~flags & ID3_OPENF_CREATE)
			goto Return_NULL;
		id3_init_tag(id3);
	}

	return id3;

	/*
	 * Cleanup code.
	 */
 Return_NULL:
	free(id3->s.fd.id3_buf);
	free(id3);
	return NULL;
}


/*
 * Function id3_open_fp (fp, flags)
 *
 *    Open an ID3 tag using a file pointer.  Return a pointer to a
 *    structure describing the ID3 tag, or NULL if an error occured.
 *
 */
struct id3_tag *id3_open_fp(FILE *fp, int flags)
{
	struct id3_tag *id3;

	/*
	 * Allocate ID3 structure.
	 */
	id3 = calloc(sizeof(struct id3_tag), 1);

	/*
	 * Initialize access pointers.
	 */
	id3->id3_seek = id3_seek_fp;
	id3->id3_read = id3_read_fp;

	id3->id3_oflags = flags;
	id3->id3_type = ID3_TYPE_FP;
	id3->id3_pos = 0;
	id3->s.fp.id3_fp = fp;

	/*
	 * Allocate buffer to hold read data.
	 */
	id3->s.fp.id3_buf = malloc(ID3_FD_BUFSIZE);

    /*
	 * Initialize frames.
	 */
    INIT_LIST_HEAD(&id3->id3_frame);

	/*
	 * Try reading ID3 tag.
	 */
	if (id3_read_tag(id3) == -1)
	{
		if (~flags & ID3_OPENF_CREATE)
			goto Return_NULL;
		id3_init_tag(id3);
	}

	return id3;

	/*
	 * Cleanup code.
	 */
 Return_NULL:
	free(id3->s.fp.id3_buf);
	free(id3);
	return NULL;
}


/*
 * Function id3_close (id3)
 *
 *    Free all resources assoicated with the ID3 tag.
 *
 */
int id3_close(struct id3_tag *id3)
{
	int ret = 0;

	switch(id3->id3_type)
	{
		case ID3_TYPE_MEM:
			break;
		case ID3_TYPE_FD:
			free(id3->s.fd.id3_buf);
			break;
		case ID3_TYPE_FP:
			free(id3->s.fp.id3_buf);
			break;
		case ID3_TYPE_NONE:
			id3_error(id3, "unknown ID3 type");
			ret = -1;
	}

	id3_destroy_frames(id3);

	free(id3);

	return ret;
}


/*
 * Function id3_tell (id3)
 *
 *    Return the current position in ID3 tag.  This will always be
 *    directly after the tag.
 *
 */
#if 0
int id3_tell(struct id3_tag *id3)
{
	if (id3->id3_newtag)
		return 0;
	else
		return id3->id3_tagsize + 3 + sizeof(id3_taghdr_t);
}
#endif


/*
 * Function id3_alter_file (id3)
 *
 *    When altering a file, some ID3 tags should be discarded.  As the ID3
 *    library has no means of knowing when a file has been altered
 *    outside of the library, this function must be called manually
 *    whenever the file is altered.
 *
 */
int id3_alter_file(struct id3_tag *id3)
{
	/*
	 * List of frame classes that should be discarded whenever the
	 * file is altered.
	 */
	static uint32_t discard_list[] = {
		ID3_ETCO, ID3_EQUA, ID3_MLLT, ID3_POSS, ID3_SYLT,
		ID3_SYTC, ID3_RVAD, ID3_TENC, ID3_TLEN, ID3_TSIZ,
		0
	};
	struct id3_frame *fr;
	uint32_t id, i = 0;

	/*
	 * Go through list of frame types that should be discarded.
	 */
	while ((id = discard_list[i++]) != 0)
	{
		/*
		 * Discard all frames of that type.
		 */
		while ((fr = id3_get_frame(id3, id, 1)))
			id3_delete_frame(fr);
	}

	return 0;
}

/*
 * Function id3_write_tag (id3, buffer)
 *
 *    Wrtite the ID3 tag to the indicated file descriptor.  Return 0
 *    upon success, or -1 if an error occured.
 *
 */
int id3_write_tag(struct id3_tag *id3, uint8_t *buffer)
{
	struct id3_frame *fr;
	int size = 0;
	char buf[ID3_TAGHDR_SIZE];
    uint8_t *buffer_ptr = buffer;
    struct list_head *node;

	/*
	 * Calculate size of ID3 tag.
	 */
    list_for_each(node, &id3->id3_frame)
    {
        fr = list_entry(node, struct id3_frame, siblings);
        size += fr->fr_size + ID3_FRAMEHDR_SIZE;
    }


	/*
	 * Write tag header.
	 */
	buf[0] = id3->id3_version;
	buf[1] = id3->id3_revision;
	buf[2] = id3->id3_flags;
	ID3_SET_SIZE28(size, buf[3], buf[4], buf[5], buf[6]);

	memcpy(buffer_ptr, "ID3", 3);
    buffer_ptr += 3;

    memcpy(buffer_ptr, buf, ID3_TAGHDR_SIZE);
    buffer_ptr += ID3_TAGHDR_SIZE;

	/*
	 * TODO: Write extended header.
	 */
#if 0
	if (id3->id3_flags & ID3_THFLAG_EXT)
	{
		id3_exthdr_t exthdr;
	}
#endif

    list_for_each(node, &id3->id3_frame)
    {
		char fhdr[ID3_FRAMEHDR_SIZE];
        char *raw = fhdr;

        fr = list_entry(node, struct id3_frame, siblings);

        // Add the frame id.
        memcpy(raw, fr->fr_desc->fd_idstr, 4);
        raw += 4;

        // Add the frame size (syncsafe).
        ID3_SET_SIZE28(fr->fr_size, raw[0], raw[1], raw[2], raw[3]);
        raw += 4;

        // The two flagbytes.
        *raw++ = (fr->fr_flags >> 8) & 0xff;
        *raw++ = fr->fr_flags & 0xff;

        memcpy(buffer_ptr, fhdr, sizeof(fhdr));
        buffer_ptr += sizeof(fhdr);

        memcpy(buffer_ptr, fr->fr_data, fr->fr_size);
        buffer_ptr += fr->fr_size;
	}
	return buffer_ptr - buffer;
}
