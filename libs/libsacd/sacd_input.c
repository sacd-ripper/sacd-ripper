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
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#if defined(__lv2ppu__)
#include <sys/file.h>
#include <sys/stat.h>
#include <ppu-asm.h>

#include <sys/storage.h>
#include "ioctl.h"
#include "sac_accessor.h"
#elif defined(WIN32)
#include <io.h>
#endif

#include <utils.h>
#include <logging.h>
#include <socket.h>
#include <pb.h>
#include <pb_encode.h>
#include <pb_decode.h>

#include "scarletbook.h"
#include "sacd_input.h"
#include "sacd_pb_stream.h"
#include "sacd_ripper.pb.h"

sacd_input_t (*sacd_input_open)         (const char *);
int          (*sacd_input_close)        (sacd_input_t);
ssize_t      (*sacd_input_read)         (sacd_input_t, int, int, void *);
char *       (*sacd_input_error)        (sacd_input_t);
int          (*sacd_input_authenticate) (sacd_input_t);
int          (*sacd_input_decrypt)      (sacd_input_t, uint8_t *, int);
uint32_t     (*sacd_input_total_sectors)(sacd_input_t);

struct sacd_input_s
{
    int                 fd;
    uint8_t            *input_buffer;
#if defined(__lv2ppu__)
    device_info_t       device_info;
#endif
};

static int sacd_dev_input_authenticate(sacd_input_t dev)
{
#if defined(__lv2ppu__)
    int ret = create_sac_accessor();
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("create_sac_accessor (%#x)\n", ret));
        return ret;
    }

    ret = sac_exec_initialize();
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sac_exec_initialize (%#x)\n", ret));
        return ret;
    }

    ret = sac_exec_key_exchange(dev->fd);
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sac_exec_key_exchange (%#x)\n", ret));
        return ret;
    }
#endif
    return 0;
}

static int sacd_dev_input_decrypt(sacd_input_t dev, uint8_t *buffer, int blocks)
{
#if defined(__lv2ppu__)
    int ret, block_number = 0;
    while(block_number < blocks)
    {
        // SacModule has an internal max of 3*2048 to process
        int block_size = min(blocks - block_number, 3);     
        ret = sac_exec_decrypt_data(buffer + block_number * SACD_LSN_SIZE, block_size * SACD_LSN_SIZE, buffer + (block_number * SACD_LSN_SIZE));
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("sac_exec_decrypt_data: (%#x)\n", ret));
            return -1;
        }

        block_number += block_size;
    }
    return 0;
#elif 0
    // testing..
    int block_number = 0;
    while(block_number < blocks)
    {
        uint8_t * p = buffer + (block_number * SACD_LSN_SIZE);
        uint8_t * e = p + SACD_LSN_SIZE;
        while (p < e)
        {
            *p = 'E';
            p += 2;
        }
        block_number++;
    }
#endif
    return 0;
}

/**
 * initialize and open a SACD device or file.
 */
static sacd_input_t sacd_dev_input_open(const char *target)
{
    sacd_input_t dev;

    /* Allocate the library structure */
    dev = (sacd_input_t) calloc(sizeof(*dev), 1);
    if (dev == NULL)
    {
        fprintf(stderr, "libsacdread: Could not allocate memory.\n");
        return NULL;
    }

    /* Open the device */
#if defined(WIN32)
    dev->fd = open(target, O_RDONLY | O_BINARY);
#elif defined(__lv2ppu__)
    {
        uint8_t                 buffer[64];
        int                     ret;

        ret = sys_storage_get_device_info(BD_DEVICE, &dev->device_info);
        if (ret != 0)
        {
            goto error;
        }

        ret = sys_storage_open(BD_DEVICE, &dev->fd);
        if (ret != 0)
        {
            goto error;
        }

        memset(buffer, 0, 64);
        ioctl_get_configuration(dev->fd, buffer);
        print_hex_dump(LOG_NOTICE, "config: ", 16, 1, buffer, 64, 0);
        if ((buffer[0] & 1) != 0)
        {
            LOG(lm_main, LOG_NOTICE, ("executing ioctl_mode_sense."));
            ioctl_mode_sense(dev->fd, buffer);
            if (buffer[11] == 2)
            {
                LOG(lm_main, LOG_NOTICE, ("executing ioctl_mode_select."));
                ioctl_mode_select(dev->fd);
            }
        }
        sys_storage_get_device_info(BD_DEVICE, &dev->device_info);

        print_hex_dump(LOG_NOTICE, "device_info: ", 16, 1, &dev->device_info, sizeof(device_info_t), 0);
        if (dev->device_info.sector_size != SACD_LSN_SIZE)
        {
            LOG(lm_main, LOG_ERROR, ("incorrect LSN size [%x]\n", dev->device_info.sector_size));
            goto error;
        }

    }
#elif defined _WIN32
    dev->fd = open(target, O_RDONLY | O_BINARY);
#else
    dev->fd = open(target, O_RDONLY);
#endif
    if (dev->fd < 0)
    {
        goto error;
    }

    return dev;

error:

    free(dev);

    return 0;
}

/**
 * return the last error message
 */
static char *sacd_dev_input_error(sacd_input_t dev)
{
    /* use strerror(errno)? */
    return (char *) "unknown error";
}

/**
 * read data from the device.
 */
static ssize_t sacd_dev_input_read(sacd_input_t dev, int pos, int blocks, void *buffer)
{
#if defined(__lv2ppu__)
    int      ret;
    uint32_t sectors_read;

    ret = sys_storage_read(dev->fd, pos, blocks, buffer, &sectors_read);

    return (ret != 0) ? 0 : sectors_read;

#else
    ssize_t ret, len;

    ret = lseek(dev->fd, (off_t) pos * (off_t) SACD_LSN_SIZE, SEEK_SET);
    if (ret < 0)
    {
        return 0;
    }

    len = (size_t) blocks * SACD_LSN_SIZE;

    while (len > 0)
    {
        ret = read(dev->fd, buffer, (unsigned int) len);

        if (ret < 0)
        {
            /* One of the reads failed, too bad.  We won't even bother
             * returning the reads that went OK, and as in the POSIX spec
             * the file position is left unspecified after a failure. */
            return ret;
        }

        if (ret == 0)
        {
            /* Nothing more to read.  Return all of the whole blocks, if any.
             * Adjust the file position back to the previous block boundary. */
            ssize_t bytes     = (ssize_t) blocks * SACD_LSN_SIZE - len;
            off_t   over_read = -(bytes % SACD_LSN_SIZE);
            /*off_t pos =*/ lseek(dev->fd, over_read, SEEK_CUR);
            /* should have pos % 2048 == 0 */
            return (int) (bytes / SACD_LSN_SIZE);
        }

        len -= ret;
    }

    return blocks;
#endif
}

/**
 * close the SACD device and clean up.
 */
static int sacd_dev_input_close(sacd_input_t dev)
{
    int ret;

#if defined(__lv2ppu__)
    ret = sac_exec_exit();
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("sac_exec_exit (%#x)\n", ret));
    }

    ret = destroy_sac_accessor();
    if (ret != 0)
    {
        LOG(lm_main, LOG_ERROR, ("destroy_sac_accessor (%#x)\n", ret));
    }

    ret = sys_storage_close(dev->fd);
#else
    ret = close(dev->fd);
#endif

    free(dev);

    return ret;
}

static uint32_t sacd_dev_input_total_sectors(sacd_input_t dev)
{
    if (!dev)
        return 0;

#if defined(__lv2ppu__)
    return dev->device_info.total_sectors;
#else
    {
        struct stat file_stat;
        if(fstat(dev->fd, &file_stat) < 0)    
            return 0;

        return (uint32_t) (file_stat.st_size / SACD_LSN_SIZE);
    }
#endif
}

/**
 * initialize and open a SACD device or file.
 */
static sacd_input_t sacd_net_input_open(const char *target)
{
    ServerRequest request;
    ServerResponse response;
    sacd_input_t dev = 0;
    const char *err = 0;
    t_timeout tm;
    pb_istream_t input;
    pb_ostream_t output;
    uint8_t zero = 0;

    /* Allocate the library structure */
    dev = (sacd_input_t) calloc(sizeof(*dev), 1);
    if (dev == NULL)
    {
        fprintf(stderr, "libsacdread: Could not allocate memory.\n");
        return NULL;
    }

    dev->input_buffer = (uint8_t *) malloc(MAX_PROCESSING_BLOCK_SIZE * SACD_LSN_SIZE + 1024);
    if (dev->input_buffer == NULL)
    {
        fprintf(stderr, "libsacdread: Could not allocate memory.\n");
        goto error;
    }

    socket_open();

    socket_create(&dev->fd, AF_INET, SOCK_STREAM, 0);
    socket_setblocking(&dev->fd);

    timeout_markstart(&tm); 
    err = inet_tryconnect(&dev->fd, 
            substr(target, 0, strchr(target, ':') - target), 
            atoi(strchr(target, ':') + 1), &tm);
    if (err)
    {
        fprintf(stderr, "Failed to connect\n");
        goto error;
    }
    socket_setblocking(&dev->fd);

    input = pb_istream_from_socket(&dev->fd);

    output = pb_ostream_from_socket(&dev->fd);

    request.type = ServerRequest_Type_DISC_OPEN;

    if (!pb_encode(&output, ServerRequest_fields, &request))
    {
        fprintf(stderr, "Failed to encode request\n");
        goto error;
    }

    /* We signal the end of request with a 0 tag. */
    pb_write(&output, &zero, 1);

    if (!pb_decode(&input, ServerResponse_fields, &response))
    {
        fprintf(stderr, "Failed to decode response\n");
        goto error;
    }

    if (response.result != 0 || response.type != ServerResponse_Type_DISC_OPENED)
    {
        fprintf(stderr, "Response result non-zero or disc opened\n");
        goto error;
    }

    return dev;

error:

    sacd_input_close(dev);

    return 0;
}

/**
 * close the SACD device and clean up.
 */
static int sacd_net_input_close(sacd_input_t dev)
{
    if (!dev)
    {
        return 0;
    }
    else
    {
        ServerRequest request;
        ServerResponse response;
        pb_istream_t input = pb_istream_from_socket(&dev->fd);
        pb_ostream_t output = pb_ostream_from_socket(&dev->fd);
        uint8_t zero = 0;

        request.type = ServerRequest_Type_DISC_CLOSE;
        if (!pb_encode(&output, ServerRequest_fields, &request))
        {
            goto error;
        }

        pb_write(&output, &zero, 1);

        if (!pb_decode(&input, ServerResponse_fields, &response))
        {
            goto error;
        }

        if (response.result == 0 || response.type != ServerResponse_Type_DISC_CLOSED)
        {
            goto error;
        }
    }

error:

    if(dev)
    {
        socket_destroy(&dev->fd);
        socket_close();
        if (dev->input_buffer)
        {
            free(dev->input_buffer);
            dev->input_buffer = 0;
        }
        free(dev);
        dev = 0;
    }
    return 0;
}

static uint32_t sacd_net_input_total_sectors(sacd_input_t dev)
{
    if (!dev)
    {
        return 0;
    }
    else
    {
        ServerRequest request;
        ServerResponse response;
        pb_istream_t input = pb_istream_from_socket(&dev->fd);
        pb_ostream_t output = pb_ostream_from_socket(&dev->fd);
        uint8_t zero = 0;

        request.type = ServerRequest_Type_DISC_SIZE;

        if (!pb_encode(&output, ServerRequest_fields, &request))
        {
            return 0;
        }

        /* We signal the end of request with a 0 tag. */
        pb_write(&output, &zero, 1);

        if (!pb_decode(&input, ServerResponse_fields, &response))
        {
            return 0;
        }

        if (response.type != ServerResponse_Type_DISC_SIZE)
        {
            return 0;
        }

        return (uint32_t) response.result;
    }
}

static ssize_t sacd_net_input_read(sacd_input_t dev, int pos, int blocks, void *buffer)
{
    if (!dev)
    {
        return 0;
    }
    else
    {
        uint8_t output_buf[16];
        ServerRequest request;
        ServerResponse response;
        pb_ostream_t output = pb_ostream_from_buffer(output_buf, sizeof(output_buf));
        pb_istream_t input = pb_istream_from_socket(&dev->fd);
        uint8_t zero = 0;

        request.type = ServerRequest_Type_DISC_READ;
        request.sector_offset = pos;
        request.sector_count = blocks;

        if (!pb_encode(&output, ServerRequest_fields, &request))
        {
            return 0;
        }

        /* We signal the end of request with a 0 tag. */
        pb_write(&output, &zero, 1);

        // write the output buffer to the opened socket
        {
            bool ret;
            size_t written; 
            ret = (socket_send(&dev->fd, (char *) output_buf, output.bytes_written, &written, 0, 0) == IO_DONE && written == output.bytes_written); 

            if (!ret)
                return 0;
        }

#if 0
        response.data.bytes = buffer;
        {
            size_t got; 
            uint8_t *buf_ptr = dev->input_buffer;
            size_t buf_left = blocks * SACD_LSN_SIZE + 16;

            input = pb_istream_from_buffer(dev->input_buffer, MAX_PROCESSING_BLOCK_SIZE * SACD_LSN_SIZE + 1024);

            if (socket_recv(&dev->fd, (char *) buf_ptr, buf_left, &got, MSG_PARTIAL, 0) != IO_DONE)
                return 0;

            while(got > 0 && !pb_decode(&input, ServerResponse_fields, &response))
            {
                buf_ptr += got;
                buf_left -= got;

                if (socket_recv(&dev->fd, (char *) buf_ptr, buf_left, &got, MSG_PARTIAL, 0) != IO_DONE)
                    return 0;

                input = pb_istream_from_buffer(dev->input_buffer, MAX_PROCESSING_BLOCK_SIZE * SACD_LSN_SIZE + 1024);
            }
        }
#else
        response.data.bytes = buffer;
        if (!pb_decode(&input, ServerResponse_fields, &response))
        {
            return 0;
        }
#endif
        if (response.type != ServerResponse_Type_DISC_READ)
        {
            return 0;
        }

        if (response.has_data)
        {
            return response.result;
        }
    }

    return 0;
}

/**
 * Setup read functions with either network or file access
 */
int sacd_input_setup(const char* path)
{
    int net_conn = 0;
    {
        // TODO: replace this F*(&^*($#^(&*#^$GLY hack to detect IP
        int i = 0;
        const char *c = path;
        while ((c = strchr(c + 1, '.')))
        {
            if (++i == 3 && strchr(c + 1, ':'))
            {
                net_conn = 1;
                break;
            }
        }
    }

    if (net_conn)
    {
        sacd_input_open = sacd_net_input_open;
        sacd_input_close = sacd_net_input_close;
        sacd_input_read = sacd_net_input_read;
        sacd_input_error = sacd_dev_input_error;
        sacd_input_authenticate  = sacd_dev_input_authenticate;
        sacd_input_decrypt = sacd_dev_input_decrypt;
        sacd_input_total_sectors = sacd_net_input_total_sectors;

        return 1;
    } 

    sacd_input_open = sacd_dev_input_open;
    sacd_input_close = sacd_dev_input_close;
    sacd_input_read = sacd_dev_input_read;
    sacd_input_error = sacd_dev_input_error;
    sacd_input_authenticate  = sacd_dev_input_authenticate;
    sacd_input_decrypt = sacd_dev_input_decrypt;
    sacd_input_total_sectors = sacd_dev_input_total_sectors;

    return 0;
} 