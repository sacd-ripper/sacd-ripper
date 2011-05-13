#include <sysutil/sysutil.h>

#include "rsxutil.h"
#include "exit_handler.h"

static int receive_exit_request = 0;

static void sysutil_exit_callback(u64 status, u64 param, void *usrdata) {
	switch(status) {
		case SYSUTIL_EXIT_GAME:
			receive_exit_request = 1;
			break;
		case SYSUTIL_DRAW_BEGIN:
		case SYSUTIL_DRAW_END:
			break;
		default:
			break;
	}
}

static void program_exit_callback() {
	sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);

	gcmSetWaitFlip(context);
	rsxFinish(context,1);
}

int initialize_exit_handlers() {
	int ret;	

	ret = atexit(program_exit_callback);
	if (ret != 0)
		return ret;
		
	ret = sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, sysutil_exit_callback, NULL);
	if (ret != 0)
		return ret;

	return 0;	
}

int user_requested_exit() {
	return receive_exit_request;
}
