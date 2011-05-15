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
#include "output_device.h"

char *substr(const char *pstr, int start, int numchars) {
	static char pnew[255];
	strncpy(pnew, pstr + start, numchars);
	pnew[numchars] = '\0';
	return pnew;
}

static int dialog_action = 0;

// information about the current disc
static int bd_contains_sacd_disc = -1;

// when a disc has changed this is set to zero
static int bd_disc_changed = -1;

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

void dump_sample_to_output_device(void) {
	msgType dialog_type;
	char file_path[50];
	char message[200];
	int fd_in, i;
	FILE * fd_out;
	uint32_t sectors_read;
	snprintf(file_path, 200, "%s/sacd_analysis.bin", output_device);
	
	fd_out = fopen (file_path, "wb");
	if (fd_out) {

		uint8_t *buffer = (uint8_t *) malloc(2048);

		if (sys_storage_open(BD_DEVICE, &fd_in) == 0) {

			for (i = 0; i < 1024; i++) {
				memset(buffer, 0, 2048);
				sys_storage_read(fd_in, i, 1, buffer, &sectors_read);
				fwrite(buffer, 1, 2048, fd_out);
			}
			
			sys_storage_close(fd_in);
		}
		free(buffer);
		fclose(fd_out);
	}

	snprintf(message, 200, "Dump analysis has been written to: [%s]\nPlease send this sample to the author of this program.", file_path);
	dialog_type = (MSG_DIALOG_NORMAL | MSG_DIALOG_DISABLE_CANCEL_ON);
	msgDialogOpen2(dialog_type, message, dialog_handler, NULL, NULL);

	dialog_action = 0;
	bd_disc_changed = 0;
	while(!dialog_action && !user_requested_exit() && bd_disc_changed == 0) {

		sysUtilCheckCallback();
		flip();
	}
	msgDialogAbort();

}

static void bd_eject_disc_callback(void) {
	bd_contains_sacd_disc = -1;
	bd_disc_changed = -1;
}

static void bd_insert_disc_callback(uint32_t disc_type, char *title_id) {

	bd_disc_changed = 1;

	if (disc_type == SYS_DISCTYPE_PS3) {
		// cannot do anything with a PS3 disc..
		bd_contains_sacd_disc = 0;
	} else {
		// unknown disc
		bd_contains_sacd_disc = 1;
	}
}

void main_loop(void) {
	msgType dialog_type;
	char message[512];
	char *message_ptr = message;

	sacd_reader_t *sacd_reader;
	scarletbook_handle_t *sb_handle = 0;
	
	// did the disc change?
	if (bd_contains_sacd_disc && (output_device_changed || bd_disc_changed)) {

		// open the BD device
		sacd_reader = sacd_open("/dev_bdvd");
		if (sacd_reader) {

			// read the scarletbook information
			sb_handle = scarletbook_open(sacd_reader, 0);
			if (sb_handle) {

				master_text_t *master_text = sb_handle->master_text[0];
	
				snprintf(message_ptr, 512, 
						"disc status: %d\n"
						"title: %s\n"
						"device: %s %.2fGB\n"
						, bd_contains_sacd_disc
						, substr((char*) master_text + master_text->disc_title_position, 0, 60)
						, (output_device ? output_device : "no device"), output_device_space);

				scarletbook_close(sb_handle);
				sb_handle = 0;
			} else {
				bd_contains_sacd_disc = 0;
			}

			// close the input device asap
			sacd_close(sacd_reader);
			sacd_reader = 0;

		} else {
			bd_contains_sacd_disc = 0;
		}
	}

	// by default we have no user controls
	dialog_type = (MSG_DIALOG_NORMAL | MSG_DIALOG_DISABLE_CANCEL_ON);

	if (!bd_contains_sacd_disc) {
		
		// was the disc changed since startup?
		if (bd_disc_changed == -1 || !output_device) {
			snprintf(message_ptr, 512, "The current disc is empty or not recognized as an SACD, please re-insert.\n\n%s"
					, (output_device ? "" : "(Also make sure you connect an external fat32 formatted harddisk!)"));
		} else {
			snprintf(message_ptr, 512, "The containing disc is not recognized as an SACD.\n"
									   "Would you like to RAW dump the first 2Mb to [%s (%.2fGB available)] for analysis?",
			   						   output_device, output_device_space);

			dialog_type |= MSG_DIALOG_BTN_TYPE_OK;
		}
	}

	// can we start ripping?
	//dialog_type |= MSG_DIALOG_BTN_TYPE_OK;

	msgDialogOpen2(dialog_type, message_ptr, dialog_handler, NULL, NULL);

	dialog_action = 0;
	bd_disc_changed = 0;
	output_device_changed = 0;
	while(!dialog_action && !user_requested_exit() && bd_disc_changed == 0 && output_device_changed == 0) {
		
		// poll for new output devices
		poll_output_devices();

		sysUtilCheckCallback();
		flip();
	}
	msgDialogAbort();

	// user wants to dump 2Mb to output device
	if (dialog_action == 1 && !bd_contains_sacd_disc) {

		dump_sample_to_output_device();

		// action is already handled
		dialog_action = 0;
	}
    
}

int main(int argc,char *argv[]) {
	int ret;
	void *host_addr = memalign(1024 * 1024, HOST_SIZE);
	msgType dialog_type;

	init_screen(host_addr, HOST_SIZE);
	ioPadInit(7);

	ret = initialize_exit_handlers();
	if (ret != 0)
		return -1;

	// patch syscall 864 to allow drive re-init
	ret = patch_syscall_864();
	if (ret < 0) {
	
		dialog_type = (MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
		msgDialogOpen2(dialog_type, "ERROR: Couldn't patch syscall 864, returning to the XMB.", dialog_handler, NULL, NULL);

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
	
		dialog_type = (MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
		msgDialogOpen2(dialog_type, "Installation was aborted, returning to the XMB.", dialog_handler, NULL, NULL);

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

	// poll for an output_device
	poll_output_devices();


	while(1) {

		// main loop
		main_loop();
		
		// did user request to start the ripping process?
		if (dialog_action == 1) {
			
		}
		
		// break out of the loop when requested
		if (user_requested_exit())
			break;
	}

	ret = sysDiscUnregisterDiscChangeCallback();

	return 0;
}
