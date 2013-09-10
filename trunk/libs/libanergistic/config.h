// Copyright 2010 fail0verflow <master@fail0verflow.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef CONFIG_H__
#define CONFIG_H__

#define DEBUG
//#define DEBUG_INSTR
//#define DEBUG_GDB

//#define DEBUG_INSTR_MEM
#define DEBUG_TRACE

//#define FAIL_DUMP_REGS
//#define FAIL_DUMP_LS

//#define STOP_DUMP_REGS
//#define STOP_DUMP_LS

#define	LS_SIZE	256 * 1024
#define	LSLR	(LS_SIZE - 1)
#define DUMP_LS_NAME "ls.b"

#define SPU_ID 0xdeadbabe

#ifndef DEBUG
#define dbgprintf(...)
#else
#include <stdio.h>
#define dbgprintf printf
#endif

#endif
