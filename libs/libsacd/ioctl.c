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

#ifdef __lv2ppu__

#include <sys/storage.h>
#include "ioctl.h"

int ioctl_eject(int fd)
{
    int                         res;
    struct lv2_atapi_cmnd_block atapi_cmnd;

    sys_storage_init_atapi_cmnd(&atapi_cmnd, 0, ATAPI_NON_DATA_PROTO, ATAPI_DIR_WRITE);

    atapi_cmnd.pkt[0] = GPCMD_START_STOP_UNIT;
    atapi_cmnd.pkt[1] = 1;
    atapi_cmnd.pkt[4] = 0x02;     /* eject */

    res = sys_storage_send_atapi_command(fd, &atapi_cmnd, 0);

    return res;
}

int ioctl_get_configuration(int fd, uint8_t *buffer)
{
    int                         res;
    struct lv2_atapi_cmnd_block atapi_cmnd;

    sys_storage_init_atapi_cmnd(&atapi_cmnd, 0x10, ATAPI_PIO_DATA_IN_PROTO, ATAPI_DIR_READ);
    atapi_cmnd.pkt[0] = GPCMD_GET_CONFIGURATION;
    atapi_cmnd.pkt[1] = 0;
    atapi_cmnd.pkt[2] = 0xff;
    atapi_cmnd.pkt[3] = 0x41;
    atapi_cmnd.pkt[8] = 0x10;

    res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);
    // if (buffer[10] & 1 == 0) return 0xF000FFFF
    // if (buffer[0] & 1 != 0) exec_mode_sense
    return res;
}

int ioctl_mode_sense(int fd, uint8_t *buffer)
{
    int                         res;
    struct lv2_atapi_cmnd_block atapi_cmnd;

    sys_storage_init_atapi_cmnd(&atapi_cmnd, 0x10, ATAPI_PIO_DATA_IN_PROTO, ATAPI_DIR_READ);

    atapi_cmnd.pkt[0] = GPCMD_MODE_SENSE_10;
    atapi_cmnd.pkt[1] = 0x08;
    atapi_cmnd.pkt[2] = 0x03;
    atapi_cmnd.pkt[8] = 0x10;

    res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);
    // if (buffer[11] == 2) exec_mode_select
    return res;
}

int ioctl_mode_select(int fd)
{
    int                         res;
    struct lv2_atapi_cmnd_block atapi_cmnd;
    static uint8_t              buffer[256];
    memset(buffer, 0, sizeof(buffer));

    buffer[1]   = 0x0e;
    buffer[7]   = 8;
    buffer[8]   = 3;
    buffer[9]   = 6;
    buffer[11]  = 3;    // ? 3 == SACD
    buffer[255] = 0x10;

    sys_storage_init_atapi_cmnd(&atapi_cmnd, 0x10, ATAPI_PIO_DATA_OUT_PROTO, ATAPI_DIR_WRITE);

    atapi_cmnd.pkt[0] = GPCMD_MODE_SELECT_10;
    atapi_cmnd.pkt[1] = 0x10;     /* PF */
    atapi_cmnd.pkt[8] = 0x10;

    res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);

    return res;
}

int ioctl_enable_encryption(int fd, uint8_t *buffer, uint32_t lba)
{
    int                         res;
    struct lv2_atapi_cmnd_block atapi_cmnd;

    sys_storage_init_atapi_cmnd(&atapi_cmnd, 0x0a, ATAPI_PIO_DATA_IN_PROTO, ATAPI_DIR_READ);

    atapi_cmnd.pkt[0] = GPCMD_READ_DVD_STRUCTURE;
    atapi_cmnd.pkt[1] = 2;

    atapi_cmnd.pkt[2] = lba >> 24;
    atapi_cmnd.pkt[3] = lba >> 16;
    atapi_cmnd.pkt[4] = lba >> 8;
    atapi_cmnd.pkt[5] = lba & 0xff;

    atapi_cmnd.pkt[9] = 0x0a;

    res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);

    return res;
}

int ioctl_get_event_status_notification(int fd, uint8_t *buffer)
{
    int                         res;
    struct lv2_atapi_cmnd_block atapi_cmnd;

    sys_storage_init_atapi_cmnd(&atapi_cmnd, 0x08, ATAPI_PIO_DATA_IN_PROTO, ATAPI_DIR_READ);

    atapi_cmnd.pkt[0] = GPCMD_GET_EVENT_STATUS_NOTIFICATION;
    atapi_cmnd.pkt[1] = 1;     /* IMMED */
    atapi_cmnd.pkt[4] = 4;     /* media event */
    atapi_cmnd.pkt[8] = 0x08;

    res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);

    return res;
}

int ioctl_report_key_start(int fd, uint8_t *buffer)
{
    int                         res;
    struct lv2_atapi_cmnd_block atapi_cmnd;

    sys_storage_init_atapi_cmnd(&atapi_cmnd, 0x08, ATAPI_PIO_DATA_IN_PROTO, ATAPI_DIR_READ);

    atapi_cmnd.pkt[0] = GPCMD_REPORT_KEY;

    atapi_cmnd.pkt[2] = 0;
    atapi_cmnd.pkt[3] = 0;
    atapi_cmnd.pkt[4] = 0;
    atapi_cmnd.pkt[5] = 0x08;

    atapi_cmnd.pkt[6] = 0;
    atapi_cmnd.pkt[7] = 0x10;

    res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);

    return res;
}

int ioctl_send_key(int fd, uint8_t agid, uint32_t key_size, uint8_t *key, uint8_t sequence)
{
    int                         res;
    struct lv2_atapi_cmnd_block atapi_cmnd;
    uint32_t                    buffer_size;
    uint8_t                     buffer[256];
    uint8_t                     buffer_align = 0;

    if ((key_size & 3) != 0)
    {
        buffer_align = ~(key_size & 3) + 4 + 1;
    }
    buffer_size = key_size + 4 + buffer_align;

    sys_storage_init_atapi_cmnd(&atapi_cmnd, buffer_size, ATAPI_PIO_DATA_OUT_PROTO, ATAPI_DIR_WRITE);

    atapi_cmnd.pkt[0] = GPCMD_SEND_KEY;

    atapi_cmnd.pkt[2] = buffer_size >> 24;
    atapi_cmnd.pkt[3] = buffer_size >> 16;
    atapi_cmnd.pkt[4] = buffer_size >> 8;
    atapi_cmnd.pkt[5] = buffer_size & 0xff;

    atapi_cmnd.pkt[6]  = sequence;
    atapi_cmnd.pkt[7]  = 0x10;
    atapi_cmnd.pkt[10] = agid;

    memset(buffer, 0, sizeof(buffer));

    buffer[0] = key_size >> 24;
    buffer[1] = key_size >> 16;
    buffer[2] = key_size >> 8;
    buffer[3] = key_size & 0xff;
    memcpy(buffer + 4, key, key_size);

    //if (buffer_align != 0) {
    //  memset(buffer + key_size + 4, 0, buffer_align);
    //}

    res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);

    return res;
}

int ioctl_report_key(int fd, uint8_t agid, uint32_t *key_size, uint8_t *key, uint8_t sequence)
{
    int                         res;
    struct lv2_atapi_cmnd_block atapi_cmnd;
    uint32_t                    buffer_size;
    uint8_t                     buffer[256];
    uint8_t                     buffer_align = 0;
    uint32_t                    new_key_size, old_key_size = *key_size;

    memset(buffer, 0, sizeof(buffer));

    if ((old_key_size & 3) != 0)
    {
        buffer_align = ~(old_key_size & 3) + 4 + 1;
    }
    buffer_size = old_key_size + 4 + buffer_align;

    sys_storage_init_atapi_cmnd(&atapi_cmnd, buffer_size, ATAPI_PIO_DATA_IN_PROTO, ATAPI_DIR_READ);

    atapi_cmnd.pkt[0] = GPCMD_REPORT_KEY;

    atapi_cmnd.pkt[2] = buffer_size >> 24;
    atapi_cmnd.pkt[3] = buffer_size >> 16;
    atapi_cmnd.pkt[4] = buffer_size >> 8;
    atapi_cmnd.pkt[5] = buffer_size & 0xff;

    atapi_cmnd.pkt[6]  = sequence;
    atapi_cmnd.pkt[7]  = 0x10;
    atapi_cmnd.pkt[10] = agid;

    res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);

    new_key_size = buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
    *key_size    = new_key_size;

    memcpy(key, buffer + 4, (old_key_size > new_key_size ? new_key_size : old_key_size));

    return res;
}

int ioctl_report_key_finish(int fd, uint8_t agid)
{
    int                         res;
    struct lv2_atapi_cmnd_block atapi_cmnd;

    sys_storage_init_atapi_cmnd(&atapi_cmnd, 0, ATAPI_NON_DATA_PROTO, ATAPI_DIR_READ);

    atapi_cmnd.pkt[0] = GPCMD_REPORT_KEY;

    atapi_cmnd.pkt[6]  = 0xff;
    atapi_cmnd.pkt[7]  = 0x10;
    atapi_cmnd.pkt[10] = agid;

    res = sys_storage_send_atapi_command(fd, &atapi_cmnd, 0);

    return res;
}


int ioctl_read_toc_header(int fd, uint8_t *buffer)
{
    int                         res;
    struct lv2_atapi_cmnd_block atapi_cmnd;

    sys_storage_init_atapi_cmnd(&atapi_cmnd, 12, ATAPI_PIO_DATA_IN_PROTO, ATAPI_DIR_READ);

    atapi_cmnd.pkt[0] = GPCMD_READ_TOC_PMA_ATIP;
    atapi_cmnd.pkt[6] = 0;
    atapi_cmnd.pkt[8] = 12;     /* LSB of length */

    res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);

    return res;
}

int ioctl_read_toc_entry(int fd, uint8_t *buffer)
{
    int                         res;
    struct lv2_atapi_cmnd_block atapi_cmnd;

    sys_storage_init_atapi_cmnd(&atapi_cmnd, 12, ATAPI_PIO_DATA_IN_PROTO, ATAPI_DIR_READ);

    atapi_cmnd.pkt[0] = GPCMD_READ_TOC_PMA_ATIP;
    atapi_cmnd.pkt[6] = 0x01;
    atapi_cmnd.pkt[8] = 12;     /* LSB of length */

    res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);

    return res;
}

int ioctl_read_track(int fd, uint8_t *buffer, uint8_t track)
{
    int                         res;
    struct lv2_atapi_cmnd_block atapi_cmnd;

    sys_storage_init_atapi_cmnd(&atapi_cmnd, 48, ATAPI_PIO_DATA_IN_PROTO, ATAPI_DIR_READ);

    atapi_cmnd.pkt[0] = GPCMD_READ_TRACK_RZONE_INFO;
    atapi_cmnd.pkt[1] = 1;
    atapi_cmnd.pkt[5] = track;  /* track */
    atapi_cmnd.pkt[8] = 48;     /* LSB of length */

    res = sys_storage_send_atapi_command(fd, &atapi_cmnd, buffer);

    return res;
}

#endif
