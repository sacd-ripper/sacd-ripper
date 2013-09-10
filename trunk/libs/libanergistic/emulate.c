// Copyright 2010 fail0verflow <master@fail0verflow.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>

#include "config.h"
#include "types.h"
#include "emulate.h"
#include "main.h"
#include "emulate-instrs.h"
#include "helper.h"
#include "gdb.h"

static u32 instr;

static u32 op;
static u32 ra;
static u32 rb;
static u32 rc;
static u32 rt;
static u32 ix;

#define instr_bits(start, end) (instr >> (31 - end)) & ((1 << (end - start + 1)) - 1)

static void decode_rr(void)
{
	rb = instr_bits(11, 17);
	ra = instr_bits(18, 24);
	rt = instr_bits(25, 31);
}

static void decode_rrr(void)
{
	rt = instr_bits(4, 10);
	rb = instr_bits(11, 17);
	ra = instr_bits(18, 24);
	rc = instr_bits(25, 31);
}

static void decode_ri7(void)
{
	ix = instr_bits(11, 17);
	ra = instr_bits(18, 24);
	rt = instr_bits(25, 31);
}
static void decode_ri8(void)
{
	ix = instr_bits(10, 17);
	ra = instr_bits(18, 24);
	rt = instr_bits(25, 31);
}

static void decode_ri10(void)
{
	ix = instr_bits(8, 17);
	ra = instr_bits(18, 24);
	rt = instr_bits(25, 31);
}

static void decode_ri16(void)
{
	ix = instr_bits(9, 24);
	rt = instr_bits(25, 31);
}

static void decode_ri18(void)
{
	ix = instr_bits(7, 24);
	rt = instr_bits(25, 31);
}

static int emulate_instr(void)
{
	op = instr_bits(0, 10);
	
	switch(instr_tbl[op].type) {
		case SPU_INSTR_RR:
			decode_rr();
			return ((spu_instr_rr_t)instr_tbl[op].ptr)(rt, ra, rb);
			break;
		case SPU_INSTR_RRR:
			decode_rrr();
			return ((spu_instr_rrr_t)instr_tbl[op].ptr)(rt, ra, rb, rc);
			break;
		case SPU_INSTR_RI7:
			decode_ri7();
			return ((spu_instr_ri7_t)instr_tbl[op].ptr)(rt, ra, ix);
			break;
		case SPU_INSTR_RI8:
			decode_ri8();
			return ((spu_instr_ri8_t)instr_tbl[op].ptr)(rt, ra, ix);
			break;
		case SPU_INSTR_RI10:
			decode_ri10();
			return ((spu_instr_ri10_t)instr_tbl[op].ptr)(rt, ra, ix);
			break;
		case SPU_INSTR_RI16:
			decode_ri16();
			return ((spu_instr_ri16_t)instr_tbl[op].ptr)(rt, ix);
			break;
		case SPU_INSTR_RI18:
			decode_ri18();
			return ((spu_instr_ri18_t)instr_tbl[op].ptr)(rt, ix);
			break;
		case SPU_INSTR_SPECIAL:
			return ((spu_instr_special_t)instr_tbl[op].ptr)(instr);
			break;
		case SPU_INSTR_NONE:
		default:
			fail("Unknown instruction at %08x: %08x", ctx->pc, instr);
			return 1;
	}
}

u32 emulate(void)
{
	int res;

	u32 opc = ctx->pc;

	instr = be32(ctx->ls + ctx->pc);
#ifdef DEBUG_INSTR
	dbgprintf("%05x: %08x ", ctx->pc, instr);
#endif

	if (gdb_bp_x(ctx->pc)) {
#ifdef DEBUG_GDB
		printf("------------------------------------------ break %08x\n", ctx->pc);
#endif
		ctx->paused = 1;
		gdb_signal(SIGTRAP);
		return 0;
	}

#ifdef DEBUG_TRACE
	dbgprintf("%05x: %08x (r1=%08x) ", ctx->pc, instr, ctx->reg[1][0]);
#endif

	res = emulate_instr();
	if (res != 0)
		return res;

#ifdef DEBUG_TRACE
	dbgprintf("%05x: ", ctx->pc);
	dbgprintf("rt:\t%08x %08x %08x %08x ",
			rtw[0],
			rtw[1],
			rtw[2],
			rtw[3]
			);
	dbgprintf("ra:\t%08x %08x %08x %08x ",
			raw[0],
			raw[1],
			raw[2],
			raw[3]
			);
	if ((instr_tbl[op].type == SPU_INSTR_RR) || (instr_tbl[op].type == SPU_INSTR_RRR))
	{
		dbgprintf("rb:\t%08x %08x %08x %08x ",
				rbw[0],
				rbw[1],
				rbw[2],
				rbw[3]
				);
	}
	if (instr_tbl[op].type == SPU_INSTR_RRR)
	{
		dbgprintf("rc:\t%08x %08x %08x %08x",
				rcw[0],
				rcw[1],
				rcw[2],
				rcw[3]
				);
	}
	printf("\n");
#endif

#ifdef DEBUG_INSTR
	if (ctx->pc != opc)
		dbgprintf("...\n");
#endif

	ctx->pc += 4;
	ctx->pc &= LSLR;

	if ((ctx->pc & 3) != 0)
		fail("pc is not aligned: %08x", ctx->pc);

//	dbgprintf("\n\n", count);
	return 0;
}
