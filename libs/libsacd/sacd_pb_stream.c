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

#include <sys/types.h>
#include <pb_encode.h>
#include <pb_decode.h>

#include "socket.h"
#include "sacd_pb_stream.h"

static bool write_callback(pb_ostream_t *stream, const uint8_t *buf, size_t count)
{
    bool ret;
    size_t written;
    p_socket socket = (p_socket) stream->state;

    ret = (socket_send(socket, (const char *) buf, count, &written, 0, 0) == IO_DONE && written == count);

    return ret;
}

static bool read_callback(pb_istream_t *stream, uint8_t *buf, size_t count)
{
    int result;
    size_t got = 0;
    p_socket socket = (p_socket) stream->state;

    if (buf == NULL)
    {
        /* Well, this is a really inefficient way to skip input. */
        /* It is only used when there are unknown fields. */
        char dummy;
        while (count-- && socket_recv(socket, &dummy, 1, &got, MSG_WAITALL, 0) == IO_DONE && got == 1);
        return count == 0;
    }
    
    result = socket_recv(socket, (char *) buf, count, &got, MSG_WAITALL, 0);

    if (result != IO_DONE)
        stream->bytes_left = 0; /* EOF */
    
    return got == count && result == IO_DONE;
}

pb_ostream_t pb_ostream_from_socket(p_socket socket)
{
    pb_ostream_t stream = {&write_callback, (void*)socket, SIZE_MAX, 0};
    return stream;
}

pb_istream_t pb_istream_from_socket(p_socket socket)
{
    pb_istream_t stream = {&read_callback, (void*)socket, SIZE_MAX};
    return stream;
}
