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

#include <sys/file.h>

#include "output_device.h"

int    output_device_changed = -1;
char   *output_device        = 0;
double output_device_space   = 0;
uint64_t output_device_sectors   = 0;

int poll_output_devices(void)
{
    static const char *device_list[11] = {
        "/dev_usb000", "/dev_usb001", "/dev_usb002", "/dev_usb003",
        "/dev_usb004", "/dev_usb005", "/dev_usb006", "/dev_usb007",
        "/dev_cf",     "/dev_sd",     "/dev_ms"
    };
    static int        old_devices;
    uint32_t          current_devices      = 0;
    char              *largest_device      = 0;
    double            largest_device_space = 0;
    uint64_t          largest_device_sectors = 0;
    int               i;

    for (i = 0; i < 11; i++)
    {
        sysFSStat fstatus;
        if (sysFsStat(device_list[i], &fstatus) == 0)
        {
            current_devices |= 1 << i;
        }
        else
        {
            current_devices &= ~(1 << i);
        }
    }

    if (old_devices != current_devices)
    {
        for (i = 0; i < 11; i++)
        {
            if ((current_devices >> i) & 1)
            {
                double   free_disk_space;
                uint32_t block_size;
                uint64_t free_block_count;

                sysFsGetFreeSize(device_list[i], &block_size, &free_block_count);
                free_disk_space = (((uint64_t) block_size * free_block_count));
                free_disk_space = free_disk_space / 1073741824.00;                 // convert to GB

                if (free_disk_space > largest_device_space)
                {
                    largest_device       = (char *) device_list[i];
                    largest_device_space = free_disk_space;
                    largest_device_sectors = (((uint64_t) block_size * free_block_count)) / 2048;
                }
            }
        }
    }

    if (old_devices != current_devices)
    {
        old_devices = current_devices;
        if (output_device != largest_device)
        {
            output_device         = largest_device;
            output_device_space   = largest_device_space;
            output_device_sectors = largest_device_sectors;
            output_device_changed = 1;
            return 1;
        }
    }

    output_device_changed = 0;
    return 0;
}
