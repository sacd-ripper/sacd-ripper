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

#ifndef __IOCTL_H__
#define __IOCTL_H__

#ifdef __lv2ppu__

#include <stdint.h>
#include <ppu-lv2.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int ioctl_eject(int fd);
extern int ioctl_init(int fd, uint8_t *buffer);
extern int ioctl_enable_encryption(int fd, uint8_t *buffer, uint32_t lba);
extern int ioctl_get_configuration(int fd, uint8_t *buffer);
extern int ioctl_mode_sense(int fd, uint8_t *buffer);
extern int ioctl_mode_select(int fd);
extern int ioctl_get_event_status_notification(int fd, uint8_t *buffer);
extern int ioctl_read_toc_header(int fd, uint8_t *buffer);
extern int ioctl_read_toc_entry(int fd, uint8_t *buffer);
extern int ioctl_read_track(int fd, uint8_t *buffer, uint8_t track);
extern int ioctl_read_block(int fd, uint8_t *buffer, int lba);
extern int ioctl_report_key_start(int fd, uint8_t *buffer);
extern int ioctl_send_key(int fd, uint8_t agid, uint32_t key_size, uint8_t *key, uint8_t sequence);
extern int ioctl_report_key(int fd, uint8_t agid, uint32_t *key_size, uint8_t *key, uint8_t sequence);
extern int ioctl_report_key_finish(int fd, uint8_t agid);

#ifdef __cplusplus
};
#endif

#endif

#endif /* __IOCTL_H__ */
