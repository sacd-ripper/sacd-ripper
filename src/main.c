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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <io/pad.h>

#include <sysutil/msg.h>
#include <sysutil/sysutil.h>
#include <sysutil/disc.h>

#include <lv2/sysfs.h>
#include <sys/file.h>

#include <sys/storage.h>
#include <patch-utils.h>

#include <scarletbook_read.h>
#include <sacd_reader.h>

#include "rsxutil.h"
#include "exit_handler.h"
#include "install.h"

char *substr(const char *pstr, int start, int numchars) {
	static char pnew[255];
	strncpy(pnew, pstr + start, numchars);
	pnew[numchars] = '\0';
	return pnew;
}

static int dialog_action = 0;

static void dialog_handler(msgButton button,void *usrData) {
	switch(button) {
		case MSG_DIALOG_BTN_OK:
			dialog_action = 1;
			break;
		case MSG_DIALOG_BTN_NO:
		case MSG_DIALOG_BTN_ESCAPE:
			dialog_action = 2;
			break;
		case MSG_DIALOG_BTN_NONE:
			dialog_action = -1;
			break;
		default:
			break;
	}
}

int patch_syscall_864(void) {
	const uint64_t addr = 0x80000000002D7820ULL; // 3.55 addr location
	uint8_t access_rights = peekq(addr) >> 56;
	if (access_rights == 0x20) {
		install_new_poke();
	
		if (!map_lv1()) {
			remove_new_poke();
			exit(0);
		}
	
		patch_lv2_protection();
		remove_new_poke();
	
		unmap_lv1();

		pokeq(addr, (uint64_t) 0x40 << 56);
	} else if (access_rights != 0x40) {
		return -1;
	}
	return 0;
}

static int disc_status = -1;
static int disc_changed = 0;

static void bd_eject_disc_callback(void)
{
	disc_status = 0;
	disc_changed = 1;
	printf("GameSample:: Disc Ejected.\n") ;
}

static void bd_insert_disc_callback(uint32_t disc_type, char *title_id)
{
	disc_changed = 1;
	printf("GameSample:: Disc Inserted.\n") ;
	if (disc_type == SYS_DISCTYPE_PS3) {
		disc_status = 0;
		printf("PS3 Game Disc Inserted.\n") ;
		printf("InsertDiscInfo::titleId       = [%s]\n", title_id);
	} else if (disc_type == SYS_DISCTYPE_OTHER) {
		printf("Other Format Disc Inserted.\n") ;
		disc_status = 1;
	} else {
		printf("Unknown discType.\n");
		disc_status = 1;
	}
}

int get_largest_output_device(char **device, double *device_space) {
	static const char *device_list[11] = {
		"/dev_usb000", "/dev_usb001", "/dev_usb002", "/dev_usb003",
		"/dev_usb004", "/dev_usb005", "/dev_usb006", "/dev_usb007",
		"/dev_cf", "/dev_sd", "/dev_ms"
	}; 
	static int old_devices;
	static char* current_device;
	uint32_t current_devices = 0;
	char *largest_device = 0;
	double largest_device_space = 0;
	int i;

	for (i = 0; i < 11; i++) {
		sysFSStat fstatus;
		if (sysLv2FsStat(device_list[i], &fstatus) == 0) {
			current_devices |= 1 << i;
		} else {
			current_devices &= ~(1 << i);
		}
	}
	
	if (old_devices != current_devices) {
		for (i = 0; i < 11; i++) {
			if ((current_devices >> i) & 1) {
				double free_disk_space;
				uint32_t block_size;
				uint64_t free_block_count;
		
				sysFsGetFreeSize(device_list[i], &block_size, &free_block_count);
				free_disk_space = (((uint64_t) block_size * free_block_count));
				free_disk_space = free_disk_space / 1073741824.00; // convert to GB
				
				if (free_disk_space > largest_device_space) {
					largest_device = (char *) device_list[i];
					largest_device_space = free_disk_space;
				}
			}
		}
	}
	
	if (old_devices != current_devices) {
		old_devices = current_devices;
		if (current_device != largest_device) {
			current_device = *device = largest_device;
			*device_space = largest_device_space;
			return 1;
		}
	}

	return 0;
} 

int get_button_pressed() {
	int i;
	padInfo padinfo;
	padData paddata;
	
	ioPadGetInfo(&padinfo);
	for (i = 0; i < MAX_PADS; i++) {
		if (padinfo.status[i]) {
			ioPadGetData(i, &paddata);
			if (paddata.BTN_LEFT || paddata.BTN_RIGHT || paddata.BTN_START || paddata.BTN_DOWN) {
				return 1;
			}
		}
	}
	return 0;
}


int main(int argc,char *argv[]) {
	int ret;
	void *host_addr = memalign(1024 * 1024, HOST_SIZE);
	msgType dialogType;

	init_screen(host_addr, HOST_SIZE);
	ioPadInit(7);

	ret = initialize_exit_handlers();
	if (ret != 0)
		return -1;

	// patch syscall 864 to allow drive re-init
	ret = patch_syscall_864();
	if (ret < 0) {
	
		dialogType = (MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
		msgDialogOpen2(dialogType, "ERROR: Couldn't patch syscall 864, returning to the XMB.", dialog_handler, NULL, NULL);

		dialog_action = 0;
		while(!dialog_action && !user_requested_exit()) {
			sysUtilCheckCallback();
			flip();
		}
	    
		msgDialogAbort();
		
		return 0;
	}

	// install the necessary modules
	ret = install_modules();
	if (ret < 0) {
	
		dialogType = (MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
		msgDialogOpen2(dialogType, "Installation was aborted, returning to the XMB.", dialog_handler, NULL, NULL);

		dialog_action = 0;
		while(!dialog_action && !user_requested_exit()) {
			sysUtilCheckCallback();
			flip();
		}
	    
		msgDialogAbort();
		
		return 0;
	}

	if (user_requested_exit())
		return 0;

	// reset & re-authenticate the BD drive
	sys_storage_reset_bd();
	sys_storage_authenticate_bd();

	ret = sysDiscRegisterDiscChangeCallback( &bd_eject_disc_callback, &bd_insert_disc_callback );

{
		double sp = 0;
		char *dv = 0;
		char *str = (char *) malloc(512);

		while(1) {

			sacd_reader_t *sacd_reader;
			scarletbook_handle_t *handle = 0;
			int dev, btn = 0;
			
			str[0] = 0x00;
		
			dev = get_largest_output_device(&dv, &sp);

			sacd_reader = sacd_open("/dev_bdvd");
			if (sacd_reader) {

				handle = scarletbook_open(sacd_reader, 0);

				scarletbook_close(handle);

				if (handle) {
					master_text_t *master_text = handle->master_text[0];
		
					snprintf(str, 512, 
							"disc status: %d\n"
							"title: %s\n"
							"device: %s %.2fGB\n"
							"button: %d\n"
							, disc_status
							, substr((char*) master_text + master_text->disc_title_position, 0, 60)
							, dv == 0 ? "no device" : dv, sp
							, btn);
				} else {
					snprintf(str, 512,
							"disc status: %d\n"
							"device: %s %.2fGB\n"
							"button: %d\n"
							, disc_status
							, dv == 0 ? "no device" : dv, sp
							, btn);
				}
				sacd_close(sacd_reader);
			}
	
			btn = 0;			
	
			dialogType = (MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
			msgDialogOpen2(dialogType, str, dialog_handler, NULL, NULL);
	
			dialog_action = 0;
			while(!dialog_action && !user_requested_exit() && disc_changed == 0 && dev == 0 && btn == 0) {
				dev = get_largest_output_device(&dv, &sp);

				//btn = get_button_pressed();
				
				sysUtilCheckCallback();
				flip();
			}
		    
			msgDialogAbort();
			
			if (disc_changed == 1)
				disc_changed = 0;

			if (user_requested_exit())
				break;
		}
}
	ret = sysDiscUnregisterDiscChangeCallback();

	return 0;
}
