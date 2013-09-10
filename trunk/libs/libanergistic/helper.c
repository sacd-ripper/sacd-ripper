// Copyright 2010 fail0verflow <master@fail0verflow.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "config.h"
#include "types.h"
#include "emulate.h"
#include "helper.h"

void dump_regs(spe_ctx_t *ctx)
{
    uint32_t i;
    if (!ctx) return;

    printf("\nRegister dump:\n");
    printf(" pc:\t%08x\n", ctx->pc);
    for (i = 0; i < 128; i++)
        printf("%.3d:\t%08x %08x %08x %08x\n",
        i,
        ctx->reg[i][0],
        ctx->reg[i][1],
        ctx->reg[i][2],
        ctx->reg[i][3]
    );
}

void dump_ls(spe_ctx_t *ctx, const char* filename)
{
    FILE *fp;
    if (!ctx) return;

    dbgprintf("dumping local store to %s\n", filename);
    fp = fopen(filename, "wb");
    fwrite(ctx->ls, LS_SIZE, 1, fp);
    fclose(fp);
}

void fail(spe_ctx_t *ctx, const char *a, ...)
{
    char msg[1024];
    va_list va;

    va_start(va, a);
    vsnprintf(msg, sizeof msg, a, va);
    perror(msg);

#ifdef FAIL_DUMP_REGS
    dump_regs(ctx);
#endif

#ifdef FAIL_DUMP_LS
    dump_ls(ctx, "fail.dmp");
#endif

    exit(1);
}

void reg2ls(spe_ctx_t *ctx, uint32_t r, uint32_t addr)
{
	addr &= LSLR & 0xfffffff0;
		dbgprintf("  LS STORE: %05x: %08x %08x %08x %08x\n", addr, ctx->reg[r][0], ctx->reg[r][1], ctx->reg[r][2], ctx->reg[r][3]);
	wbe32(ctx->ls + addr, ctx->reg[r][0]);
	wbe32(ctx->ls + addr + 4, ctx->reg[r][1]);
	wbe32(ctx->ls + addr + 8, ctx->reg[r][2]);
	wbe32(ctx->ls + addr + 12, ctx->reg[r][3]);
}

void ls2reg(spe_ctx_t *ctx, uint32_t r, uint32_t addr)
{
	addr &= LSLR & 0xfffffff0;
	ctx->reg[r][0] = be32(ctx->ls + addr);
	ctx->reg[r][1] = be32(ctx->ls + addr + 4);
	ctx->reg[r][2] = be32(ctx->ls + addr + 8);
	ctx->reg[r][3] = be32(ctx->ls + addr + 12);
		dbgprintf("  LS LOAD: %05x: %08x %08x %08x %08x\n", addr, ctx->reg[r][0], ctx->reg[r][1], ctx->reg[r][2], ctx->reg[r][3]);
}

void reg_to_byte(spe_ctx_t *ctx, uint8_t *d, int r)
{
	int i, j;
	for (i = 0; i < 4; ++i)
		for (j = 0; j < 4; ++j)
			*d++ = ctx->reg[r][i] >> (24 - j*8);
}

void byte_to_reg(spe_ctx_t *ctx, int r, const uint8_t *d)
{
	int i, j;
	for (i = 0; i < 4; ++i)
	{
		ctx->reg[r][i] = 0;
		for (j = 0; j < 4; ++j)
			ctx->reg[r][i] |= *d++ << (24 - j*8);
	}
}

void reg_to_half(spe_ctx_t *ctx, uint16_t *d, int r)
{
	int i, j;
	for (i = 0; i < 4; ++i)
		for (j = 0; j < 2; ++j)
		{
			*d++ = ctx->reg[r][i] >> (16 - j*16);
		}
}

void half_to_reg(spe_ctx_t *ctx, int r, const uint16_t *d)
{
	int i, j;
	for (i = 0; i < 4; ++i)
	{
		ctx->reg[r][i] = 0;
		for (j = 0; j < 2; ++j)
			ctx->reg[r][i] |= *d++ << (16 - j*16);
	}
}

void reg_to_bits(spe_ctx_t *ctx, uint1_t *d, int r)
{
	int i, j;
	for (i = 0; i < 4; ++i)
		for (j = 0; j < 32; ++j)
		{
			*d++ = (ctx->reg[r][i] >> (31 - j)) & 1;
		}
}

void bits_to_reg(spe_ctx_t *ctx, int r, const uint1_t *d)
{
	int i, j;
	for (i = 0; i < 4; ++i)
	{
		ctx->reg[r][i] = 0;
		for (j = 0; j < 32; ++j)
			ctx->reg[r][i] |= *d++ << (31 - j);
	}
}

void reg_to_float(spe_ctx_t *ctx, float *d, int r)
{
    memcpy( d, ctx->reg[r], 16);
}

void float_to_reg(spe_ctx_t *ctx, int r, const float *d)
{
    memcpy(ctx->reg[r], d, 16);
}

void reg_to_double(spe_ctx_t *ctx, double *d, int r)
{
    uint32_t* tmp = (uint32_t*)d;
    tmp[1] = ctx->reg[r][0];	
    tmp[0] = ctx->reg[r][1];	
    tmp[3] = ctx->reg[r][2];	
    tmp[2] = ctx->reg[r][3];	
}

void double_to_reg(spe_ctx_t *ctx, int r, const double *d)
{
    uint32_t* tmp = (uint32_t*)d;
    ctx->reg[r][0] = tmp[1];	
    ctx->reg[r][1] = tmp[0];	
    ctx->reg[r][2] = tmp[3];	
    ctx->reg[r][3] = tmp[2];	
} 
