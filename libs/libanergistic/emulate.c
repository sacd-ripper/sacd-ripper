// Copyright 2010 fail0verflow <master@fail0verflow.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "config.h"
#include "types.h"
#include "emulate.h"
#include "emulate-instrs.h"
#include "helper.h"

#define instr_bits(start, end) (instr >> (31 - end)) & ((1 << (end - start + 1)) - 1)

static int emulate_instr(spe_ctx_t *ctx, uint32_t instr)
{
    uint32_t op, ra, rb, rc, rt, ix;

    op = instr_bits(0, 10);
	switch(instr_tbl[op].type) {
		case SPU_INSTR_RR:
            rb = instr_bits(11, 17); 
            ra = instr_bits(18, 24); 
            rt = instr_bits(25, 31);
			return ((spu_instr_rr_t)instr_tbl[op].ptr)(ctx, rt, ra, rb);
			break;
		case SPU_INSTR_RRR:
			rt = instr_bits(4, 10); 
            rb = instr_bits(11, 17); 
            ra = instr_bits(18, 24); 
            rc = instr_bits(25, 31);
			return ((spu_instr_rrr_t)instr_tbl[op].ptr)(ctx, rt, ra, rb, rc);
			break;
		case SPU_INSTR_RI7:
			ix = instr_bits(11, 17); 
            ra = instr_bits(18, 24); 
            rt = instr_bits(25, 31);
			return ((spu_instr_ri7_t)instr_tbl[op].ptr)(ctx, rt, ra, ix);
			break;
        case SPU_INSTR_RI8:
            ix = instr_bits(10, 17);
            ra = instr_bits(18, 24);
            rt = instr_bits(25, 31);
            return ((spu_instr_ri8_t)instr_tbl[op].ptr)(ctx, rt, ra, ix);
            break;
		case SPU_INSTR_RI10:
			ix = instr_bits(8, 17); 
            ra = instr_bits(18, 24); 
            rt = instr_bits(25, 31);
			return ((spu_instr_ri10_t)instr_tbl[op].ptr)(ctx, rt, ra, ix);
			break;
		case SPU_INSTR_RI16:
			ix = instr_bits(9, 24); 
            rt = instr_bits(25, 31);
			return ((spu_instr_ri16_t)instr_tbl[op].ptr)(ctx, rt, ix);
			break;
		case SPU_INSTR_RI18:
			ix = instr_bits(7, 24); 
            rt = instr_bits(25, 31);
			return ((spu_instr_ri18_t)instr_tbl[op].ptr)(ctx, rt, ix);
			break;
		case SPU_INSTR_SPECIAL:
			return ((spu_instr_special_t)instr_tbl[op].ptr)(ctx, instr);
			break;
		case SPU_INSTR_NONE:
		default:
			fail(ctx, "Unknown instruction at %08x: %08x", ctx->pc, instr);
			return 1;
	}
}

uint32_t emulate(spe_ctx_t *ctx)
{
	int res;
	uint32_t instr;
#ifdef DEBUG_INSTR
    uint32_t opc = ctx->pc;
#endif

	instr = be32(ctx->ls + ctx->pc);
#ifdef DEBUG_INSTR
	dbgprintf("%05x: %08x ", ctx->pc, instr);
#endif

#ifdef DEBUG_TRACE
	dbgprintf("%05x: %08x (r1=%08x) ", ctx->pc, instr, ctx->reg[1][0]);
#endif

	res = emulate_instr(ctx, instr);
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
    ctx->count += 1;

	if ((ctx->pc & 3) != 0)
		fail(ctx, "pc is not aligned: %08x", ctx->pc);

	return 0;
}

spe_ctx_t* create_spe_context(void)
{
    spe_ctx_t *ctx = 0;

    ctx = (spe_ctx_t *) malloc(sizeof(spe_ctx_t));
    if (ctx == NULL)
        fail(ctx, "Unable to allocate spe context.");
    memset(ctx, 0, sizeof(spe_ctx_t));

    ctx->ls = (uint8_t *) malloc(LS_SIZE);
    if (ctx->ls == NULL)
        fail(ctx, "Unable to allocate local storage.");
    memset(ctx->ls, 0, LS_SIZE);

    ctx->reg[1][0] = 0x0003fff0;
    ctx->reg[3][1] = 0x10020040;
    ctx->reg[6][0] = 0x00000400;
    ctx->reg[6][1] = 0x00300000;

    ctx->spu_out_cnt = 1; //empty
    ctx->spu_out_intr_cnt = 1; //empty
    ctx->spu_in_cnt = 4; //empty
    ctx->spu_in_rdidx = 0; //oldest value (gets incremented on write)

    return ctx;
}

void destroy_spe_context(spe_ctx_t *ctx)
{
    free(ctx->ls);
    free(ctx);
}

void spe_in_mbox_write(spe_ctx_t *ctx, uint32_t msg)
{
    int done = 0;

    // add message to mbox
    ctx->spu_in_rdidx &= 3;
    ctx->spu_in_mbox[ctx->spu_in_rdidx] = msg;
    ctx->spu_in_cnt--;

    while(done == 0 && ctx->spu_in_cnt < 4) 
    {
        done = emulate(ctx);
    }
}

void spe_out_intr_mbox_status(spe_ctx_t *ctx)
{
    int done = 0;
    while(done == 0 && ctx->mfc.prxy_tag_status < 1) 
    {
        done = emulate(ctx);
    }
    ctx->mfc.prxy_tag_status = 0;
}

uint32_t spe_out_intr_mbox_read(spe_ctx_t *ctx)
{
    int done = 0;
    while(done == 0 && ctx->spu_out_intr_cnt == 1) 
    {
        done = emulate(ctx);
    }
    ctx->spu_out_intr_cnt = 1;
    return ctx->spu_out_intr_mbox;
}
