// Copyright 2010 fail0verflow <master@fail0verflow.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "config.h"
#include "types.h"
#include "emulate.h"
#include "main.h"
#include "helper.h"
#include <string.h>

#ifndef DEBUG_INSTR_MEM
#define vdbgprintf(...)
#else
#include <stdio.h>
#define vdbgprintf printf
#endif

void reg2ls(u32 r, u32 addr)
{
	addr &= LSLR & 0xfffffff0;
		vdbgprintf("  LS STORE: %05x: %08x %08x %08x %08x\n", addr, ctx->reg[r][0], ctx->reg[r][1], ctx->reg[r][2], ctx->reg[r][3]);
	wbe32(ctx->ls + addr, ctx->reg[r][0]);
	wbe32(ctx->ls + addr + 4, ctx->reg[r][1]);
	wbe32(ctx->ls + addr + 8, ctx->reg[r][2]);
	wbe32(ctx->ls + addr + 12, ctx->reg[r][3]);
}

void ls2reg(u32 r, u32 addr)
{
	addr &= LSLR & 0xfffffff0;
	ctx->reg[r][0] = be32(ctx->ls + addr);
	ctx->reg[r][1] = be32(ctx->ls + addr + 4);
	ctx->reg[r][2] = be32(ctx->ls + addr + 8);
	ctx->reg[r][3] = be32(ctx->ls + addr + 12);
		vdbgprintf("  LS LOAD: %05x: %08x %08x %08x %08x\n", addr, ctx->reg[r][0], ctx->reg[r][1], ctx->reg[r][2], ctx->reg[r][3]);
}

void reg_to_byte(u8 *d, int r)
{
	int i, j;
	for (i = 0; i < 4; ++i)
		for (j = 0; j < 4; ++j)
			*d++ = ctx->reg[r][i] >> (24 - j*8);
}

void byte_to_reg(int r, const u8 *d)
{
	int i, j;
	for (i = 0; i < 4; ++i)
	{
		ctx->reg[r][i] = 0;
		for (j = 0; j < 4; ++j)
			ctx->reg[r][i] |= *d++ << (24 - j*8);
	}
}

void reg_to_half(u16 *d, int r)
{
	int i, j;
	for (i = 0; i < 4; ++i)
		for (j = 0; j < 2; ++j)
		{
			*d++ = ctx->reg[r][i] >> (16 - j*16);
		}
}

void half_to_reg(int r, const u16 *d)
{
	int i, j;
	for (i = 0; i < 4; ++i)
	{
		ctx->reg[r][i] = 0;
		for (j = 0; j < 2; ++j)
			ctx->reg[r][i] |= *d++ << (16 - j*16);
	}
}

void reg_to_Bits(u1 *d, int r)
{
	int i, j;
	for (i = 0; i < 4; ++i)
		for (j = 0; j < 32; ++j)
		{
			*d++ = (ctx->reg[r][i] >> (31 - j)) & 1;
		}
}

void Bits_to_reg(int r, const u1 *d)
{
	int i, j;
	for (i = 0; i < 4; ++i)
	{
		ctx->reg[r][i] = 0;
		for (j = 0; j < 32; ++j)
			ctx->reg[r][i] |= *d++ << (31 - j);
	}
}
void reg_to_float(float *d, int r)
{
	memcpy( d, ctx->reg[r], 16);
}
void float_to_reg(int r, const float *d)
{
	memcpy(ctx->reg[r], d, 16);
}
void reg_to_double(double *d, int r)
{
	u32* tmp = (u32*)d;
	tmp[1] = ctx->reg[r][0];	
	tmp[0] = ctx->reg[r][1];	
	tmp[3] = ctx->reg[r][2];	
	tmp[2] = ctx->reg[r][3];	
}
void double_to_reg(int r, const double *d)
{
	u32* tmp = (u32*)d;
	ctx->reg[r][0] = tmp[1];	
	ctx->reg[r][1] = tmp[0];	
	ctx->reg[r][2] = tmp[3];	
	ctx->reg[r][3] = tmp[2];	
}
