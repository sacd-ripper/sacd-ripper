#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <io/pad.h>

#include <sysutil/msg.h>
#include <sysutil/sysutil.h>

#include "rsxutil.h"
#include "exit_handler.h"
#include "install.h"

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

int main(int argc,char *argv[]) {
	int ret;
	void *host_addr = memalign(1024*1024, HOST_SIZE);
	msgType dialogType;

	init_screen(host_addr,HOST_SIZE);
	ioPadInit(7);

	ret = initialize_exit_handlers();
	if (ret != 0)
		return -1;

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
	
	return 0;
}
