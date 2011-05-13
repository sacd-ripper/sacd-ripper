#ifndef __RSXUTIL_H__
#define __RSXUTIL_H__

#include <rsx/rsx.h>
#include <sysutil/video.h>

#define CB_SIZE		0x100000
#define HOST_SIZE	(32*1024*1024)

#ifdef __cplusplus
extern "C" {
#endif

extern gcmContextData *context;
extern u32 display_width;
extern u32 display_height;
extern u32 curr_fb;

void set_render_target(u32 index);
void init_screen(void *host_addr,u32 size);
void waitflip();
void flip();

#ifdef __cplusplus
	}
#endif

#endif
