// Copyright 2010 fail0verflow <master@fail0verflow.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef EMULATE_H__
#define EMULATE_H__

#include "types.h"

u32 emulate(void);

typedef int (*spu_instr_rr_t)(u32 ra, u32 rb, u32 rt);
typedef int (*spu_instr_rrr_t)(u32 ra, u32 rb, u32 rc, u32 rt);
typedef int (*spu_instr_ri7_t)(u32 i7, u32 ra, u32 rt);
typedef int (*spu_instr_ri8_t)(u32 i7, u32 ra, u32 rt);
typedef int (*spu_instr_ri10_t)(u32 i10, u32 ra, u32 rt);
typedef int (*spu_instr_ri16_t)(u32 i16, u32 rt);
typedef int (*spu_instr_ri18_t)(u32 i18, u32 rt);
typedef int (*spu_instr_special_t)(u32 instr);

#endif
