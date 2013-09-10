// Copyright 2010 fail0verflow <master@fail0verflow.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef HELPER_H__
#define HELPER_H__

#include "types.h"

void dump_regs(spe_ctx_t *ctx);
void dump_ls(spe_ctx_t *ctx, const char* filename);
void fail(spe_ctx_t *ctx, const char *a, ...);

void reg2ls(spe_ctx_t *ctx, uint32_t r, uint32_t addr);
void ls2reg(spe_ctx_t *ctx, uint32_t r, uint32_t addr);
void get_mask_byte(spe_ctx_t *ctx, uint32_t rt, uint32_t t);
void get_mask_hword(spe_ctx_t *ctx, uint32_t rt, uint32_t t);
void get_mask_word(spe_ctx_t *ctx, uint32_t rt, uint32_t t);
void get_mask_dword(spe_ctx_t *ctx, uint32_t rt, uint32_t t);
void reg_to_byte(spe_ctx_t *ctx, uint8_t *d, int r);
void byte_to_reg(spe_ctx_t *ctx, int r, const uint8_t *d);
void reg_to_half(spe_ctx_t *ctx, uint16_t *d, int r);
void half_to_reg(spe_ctx_t *ctx, int r, const uint16_t *d);
void reg_to_bits(spe_ctx_t *ctx, uint1_t *d, int r);
void bits_to_reg(spe_ctx_t *ctx, int r, const uint1_t *d);
void reg_to_float(spe_ctx_t *ctx, float *d, int r);
void float_to_reg(spe_ctx_t *ctx, int r, const float *d);
void reg_to_double(spe_ctx_t *ctx, double *d, int r);
void double_to_reg(spe_ctx_t *ctx, int r, const double *d);
#define rtw ctx->reg[rt]
#define raw ctx->reg[ra]
#define rbw ctx->reg[rb]
#define rcw ctx->reg[rc]
#define rtwp rtw[0]
#define rawp raw[0]
#define rbwp rbw[0]
#define rcwp rcw[0]
#define rthp rth[1]
#define rahp rah[1]
#define rbhp rbh[1]
#define rchp rch[1]
#define rtbp rtb[3]
#define rabp rab[3]
#define rbbp rbb[3]
#define rcbp rcb[3]

#endif
