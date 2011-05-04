/**
 * SACD Ripper - http://code.google.com/p/sacd-ripper/
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

#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h> 
#include <errno.h>
#include <string.h>

#include <fcntl.h>

#include <sys/ioctl.h>
#include <linux/cdrom.h> 

#include "sac_accessor.h"

#define INIT_CGC(cgc)	memset((cgc), 0, sizeof(struct cdrom_generic_command))
#define UHZ			100

static void dump_cgc(struct cdrom_generic_command *cgc) {
    struct request_sense *sense = cgc->sense;
    int i;

    printf("cdb: ");
    for (i = 0; i < 12; i++)
        printf("%02x ", cgc->cmd[i]);
    printf("\n");

    printf("buffer (%d): ", cgc->buflen);
    for (i = 0; i < cgc->buflen; i++)
        printf("%02x ", cgc->buffer[i]);
    printf("\n");

    if (!sense)
        return;

    printf("sense: %02x.%02x.%02x\n", sense->sense_key, sense->asc, sense->ascq);
}

/*
 * issue packet command (blocks until it has completed)
 */
static int wait_cmd(int fd, struct cdrom_generic_command *cgc, void *buffer,
        int len, int ddir, int timeout, int quiet) {
    struct request_sense sense;
    int ret;

    memset(&sense, 0, sizeof (sense));

    cgc->timeout = timeout;
    cgc->buffer = buffer;
    cgc->buflen = len;
    cgc->data_direction = ddir;
    cgc->sense = &sense;
    cgc->quiet = 0;

    ret = ioctl(fd, CDROM_SEND_PACKET, cgc);
    if (ret == -1 && !quiet) {
        perror("ioctl");
        dump_cgc(cgc);
    }

    return ret || sense.sense_key;
}

int ioctl_get_configuration(int fd, uint8_t *buffer) {
    struct cdrom_generic_command cgc;
    int ret;

    INIT_CGC(&cgc);
    memset(buffer, 0, 16);

    cgc.cmd[0] = 0x46;
    cgc.cmd[2] = 0xff;
    cgc.cmd[3] = 0x41;
    cgc.cmd[8] = 0x10;

    ret = wait_cmd(fd, &cgc, buffer, 16, CGC_DATA_READ, 10 * UHZ, 0);
    if (ret < 0) {
        perror("ioctl_get_configuration");
        return ret;
    }

    return ret;
}

int ioctl_report_key_1(int fd, uint8_t *buffer) {
    struct cdrom_generic_command cgc;
    int ret;

    INIT_CGC(&cgc);

    cgc.cmd[0] = GPCMD_REPORT_KEY;
    cgc.cmd[5] = 8;
    cgc.cmd[7] = 16;

    ret = wait_cmd(fd, &cgc, buffer, 8, CGC_DATA_READ, 10 * UHZ, 0);
    if (ret < 0) {
        perror("ioctl_report_key_1");
        return ret;
    }

    return ret;
}

int ioctl_send_key_1(int fd, uint8_t *buffer) {
    struct cdrom_generic_command cgc;
    int ret;

    INIT_CGC(&cgc);

    cgc.cmd[0] = GPCMD_SEND_KEY;
    cgc.cmd[5] = 208;
    cgc.cmd[6] = 2;
    cgc.cmd[7] = 16;
    cgc.cmd[10] = 1;

    ret = wait_cmd(fd, &cgc, buffer, 208, CGC_DATA_WRITE, 10 * UHZ, 0);
    if (ret < 0) {
        perror("ioctl_send_key_1");
        return ret;
    }

    return ret;
}

int ioctl_report_key_2(int fd, uint8_t *buffer) {
    struct cdrom_generic_command cgc;
    int ret;

    INIT_CGC(&cgc);

    cgc.cmd[0] = GPCMD_REPORT_KEY;
    cgc.cmd[5] = 208;
    cgc.cmd[6] = 2;
    cgc.cmd[7] = 16;
    cgc.cmd[10] = 1;

    ret = wait_cmd(fd, &cgc, buffer, 208, CGC_DATA_READ, 10 * UHZ, 0);
    if (ret < 0) {
        perror("ioctl_report_key_2");
        return ret;
    }

    return ret;
}

int ioctl_send_key_2(int fd, uint8_t *buffer) {
    struct cdrom_generic_command cgc;
    int ret;

    INIT_CGC(&cgc);

    cgc.cmd[0] = GPCMD_SEND_KEY;
    cgc.cmd[5] = 180;
    cgc.cmd[6] = 3;
    cgc.cmd[7] = 16;
    cgc.cmd[10] = 1;

    ret = wait_cmd(fd, &cgc, buffer, 180, CGC_DATA_WRITE, 10 * UHZ, 0);
    if (ret < 0) {
        perror("ioctl_send_key_2");
        return ret;
    }

    return ret;
}

int ioctl_report_key_3(int fd, uint8_t *buffer) {
    struct cdrom_generic_command cgc;
    int ret;

    INIT_CGC(&cgc);

    cgc.cmd[0] = GPCMD_REPORT_KEY;
    cgc.cmd[5] = 180;
    cgc.cmd[6] = 3;
    cgc.cmd[7] = 16;
    cgc.cmd[10] = 1;

    ret = wait_cmd(fd, &cgc, buffer, 180, CGC_DATA_READ, 10 * UHZ, 0);
    if (ret < 0) {
        perror("ioctl_report_key_3");
        return ret;
    }

    return ret;
}

int ioctl_report_key_4(int fd, uint8_t *buffer) {
    struct cdrom_generic_command cgc;
    int ret;

    INIT_CGC(&cgc);

    cgc.cmd[0] = GPCMD_REPORT_KEY;
    cgc.cmd[5] = 52;
    cgc.cmd[6] = 4;
    cgc.cmd[7] = 16;
    cgc.cmd[10] = 1;

    ret = wait_cmd(fd, &cgc, buffer, 52, CGC_DATA_READ, 10 * UHZ, 0);
    if (ret < 0) {
        perror("ioctl_report_key_4");
        return ret;
    }

    return ret;
}

int ioctl_report_key_5(int fd) {
    struct cdrom_generic_command cgc;
    int ret;

    INIT_CGC(&cgc);

    cgc.cmd[0] = GPCMD_REPORT_KEY;
    cgc.cmd[6] = 255;
    cgc.cmd[7] = 16;
    cgc.cmd[10] = 1;

    ret = wait_cmd(fd, &cgc, 0, 0, CGC_DATA_NONE, 10 * UHZ, 0);
    if (ret < 0) {
        perror("ioctl_report_key_5");
        return ret;
    }

    return ret;
}

int ioctl_read_disc_structure(int fd, uint32_t lb_number) {
    struct cdrom_generic_command cgc;
    int ret;
    uint8_t buffer[10];

    INIT_CGC(&cgc);
    memset(buffer, 0, sizeof (buffer));

    cgc.cmd[0] = GPCMD_READ_DVD_STRUCTURE;
    cgc.cmd[1] = 2;

    cgc.cmd[5] = lb_number;
    cgc.cmd[4] = lb_number >> 8;
    cgc.cmd[3] = lb_number >> 16;
    cgc.cmd[2] = lb_number >> 24;

    cgc.cmd[9] = sizeof (buffer);

    ret = wait_cmd(fd, &cgc, buffer, sizeof (buffer), CGC_DATA_READ, 10 * UHZ, 0);
    if (ret < 0) {
        perror("ioctl_read_disc_structure");
        return ret;
    }

    return ret;
}
