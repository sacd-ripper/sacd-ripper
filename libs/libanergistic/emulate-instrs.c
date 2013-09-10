// Copyright 2010 fail0verflow <master@fail0verflow.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "config.h"
#include "types.h"
#include "emulate.h"
#include "helper.h"
#include "channel.h"

int instr_cdd(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i7)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i7 = se7(i7);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i7; 
    /* show disassembly*/
    dbgprintf("cdd $r%d,$r%d,%d\n", rt,ra,i7);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int t;

    t = raw[0] + i7;
    t >>= 2;
    t &= 2;

    for (i = 0; i < 4; ++i)
        rtw[i] = (i == t) ? 0x00010203 : (i == (t + 1)) ? 0x04050607 : (0x01010101 * (i * 4) + 0x10111213);

    /* post transform */
    
    }
    return stop;
}
int instr_rotqmbii(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i7)
{
    /* pre transform declare */
    uint1_t rtb[128];uint1_t rab[128];
    int stop = 0;
    /* pre transform */
    reg_to_bits(ctx, rtb, rt);reg_to_bits(ctx, rab, ra);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i7; 
    /* show disassembly*/
    dbgprintf("rotqmbii $r%d,$r%d,0x%x\n", rt,ra,i7);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int shift_count = (-(int32_t)i7) & 7;

    for (i = 0; i < 128; ++i)
    {
        if (i >= shift_count)
            rtb[i] = rab[i - shift_count];
        else
            rtb[i] = 0;
    }

    /* post transform */
    bits_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_clgt(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("clgt $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = -(raw[i] > rbw[i]);

    /* post transform */
    
    }
    return stop;
}
int instr_fcgt(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    float rtf[4];float raf[4];float rbf[4];
    int stop = 0;
    /* pre transform */
    reg_to_float(ctx, rtf, rt);reg_to_float(ctx, raf, ra);reg_to_float(ctx, rbf, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("fcgt $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int itrue = 0xffffffff;
    float ftrue = *(float*)&itrue;
    int i;
    for (i = 0; i < 4; ++i)
        rtf[i] = raf[i]>rbf[i]?ftrue:(float) 0.0;


    /* post transform */
    float_to_reg(ctx, rt, rtf);
    }
    return stop;
}
int instr_nand(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 1;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("nand $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_iohl(spe_ctx_t *ctx, uint32_t rt, uint32_t i16)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)i16; 
    /* show disassembly*/
    dbgprintf("iohl $r%d,0x%x\n", rt,i16);
    /* optional trapping */
    
    /* body */
    {

    rtw[0] |= i16;
    rtw[1] |= i16;
    rtw[2] |= i16;
    rtw[3] |= i16;

    /* post transform */
    
    }
    return stop;
}
int instr_gb(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("gb $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    rtw[0] = ((raw[0] & 1) << 3) | ((raw[1] & 1) << 2) | ((raw[2] & 1) << 1) | (raw[3] & 1);
    rtw[1] = rtw[2] = rtw[3] = 0;

    /* post transform */
    
    }
    return stop;
}
int instr_mpyhh(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("mpyhh $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = (raw[i] >> 16) * (rbw[i] >> 16);

    /* post transform */
    
    }
    return stop;
}
int instr_frds(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    double rtd[2];double rad[2];double rbd[2];
    int stop = 0;
    /* pre transform */
    reg_to_double(ctx, rtd, rt);reg_to_double(ctx, rad, ra);reg_to_double(ctx, rbd, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("frds $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    double dtmp = rad[0];
    float ftmp = (float) dtmp;
    float *ptmp = (float*)&dtmp;
    ptmp[1] = ftmp;
    ptmp[0] = 0.0;
    rtd[0]=dtmp;

    dtmp = rad[1];
    ftmp = (float) dtmp;
    ptmp[0] = ftmp;
    ptmp[1] = 0.0;
    rtd[1] = dtmp;

    /* post transform */
    double_to_reg(ctx, rt, rtd);
    }
    return stop;
}
int instr_cdx(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("cdx $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int t;

    t = raw[0] + rbw[0];
    t >>= 2;
    t &= 2;

    for (i = 0; i < 4; ++i)
        rtw[i] = (i == t) ? 0x00010203 : (i == (t + 1)) ? 0x04050607 : (0x01010101 * (i * 4) + 0x10111213);

    /* post transform */
    
    }
    return stop;
}
int instr_andbi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    uint8_t rtb[16];uint8_t rab[16];
    int stop = 0;
    /* pre transform */
    reg_to_byte(ctx, rtb, rt);reg_to_byte(ctx, rab, ra);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("andbi $r%d,$r%d,0x%x\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 16; ++i)
        rtb[i] = rab[i] & i10;

    /* post transform */
    byte_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_dfms(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    double rtd[2];double rad[2];double rbd[2];
    int stop = 0;
    /* pre transform */
    reg_to_double(ctx, rtd, rt);reg_to_double(ctx, rad, ra);reg_to_double(ctx, rbd, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("dfms $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    rtd[0] = (rad[0]*rbd[0])-rtd[0];
    rtd[1] = (rad[1]*rbd[1])-rtd[1];

    /* post transform */
    double_to_reg(ctx, rt, rtd);
    }
    return stop;
}
int instr_orbi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    
    int stop = 1;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("orbi $r%d,$r%d,0x%x\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_clz(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("clz $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
    {
        int b;
        for (b = 0; b < 32; ++b)
            if (raw[i] & (1<<(31-b)))
                break;
        rtw[i] = b;
    }

    /* post transform */
    
    }
    return stop;
}
int instr_absdb(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 1;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("absdb $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_dfma(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    double rtd[2];double rad[2];double rbd[2];
    int stop = 0;
    /* pre transform */
    reg_to_double(ctx, rtd, rt);reg_to_double(ctx, rad, ra);reg_to_double(ctx, rbd, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("dfma $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    rtd[0] = (rad[0]*rbd[0])+rtd[0];
    rtd[1] = (rad[1]*rbd[1])+rtd[1];

    /* post transform */
    double_to_reg(ctx, rt, rtd);
    }
    return stop;
}
int instr_brhz(spe_ctx_t *ctx, uint32_t rt, uint32_t i16)
{
    /* pre transform declare */
    uint16_t rth[8];
    int stop = 0;
    /* pre transform */
    reg_to_half(ctx, rth, rt);i16 = se16(i16);i16 <<= 2;
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)i16; 
    /* show disassembly*/
    dbgprintf("brhz $r%d,%d\n", rt,i16);
    /* optional trapping */
    
    /* body */
    {
    if (rthp == 0)
        ctx->pc += (i16) - 4;

    /* post transform */
    half_to_reg(ctx, rt, rth);
    }
    return stop;
}
int instr_cntb(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 1;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("cntb $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_stop(spe_ctx_t *ctx, uint32_t opcode)
{
    /* pre transform declare */
    
    int stop = 1;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)opcode; 
    /* show disassembly*/
    dbgprintf("stop %08x\n", opcode);
    /* optional trapping */
    if (ctx->trap) return 1;
    /* body */
    {
    if ((opcode & 0xff00) == 0x2100)
    {
        uint32_t sel = be32(ctx->ls + ctx->pc + 4);
        uint32_t arg = sel & 0xffffff;
        sel >>= 24;

        printf("cell sdk __send_to_ppe(0x%04x, 0x%02x, 0x%06x);\n", opcode & 0xff, sel, arg);
    } else
        printf("####### stop instruction reached: %08x\n", opcode);

    /* post transform */
    
    }
    return stop;
}
int instr_ceqi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i10 = se10(i10);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("ceqi $r%d,$r%d,%d\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    int i;

    for (i = 0; i < 4; ++i)
        rtw[i] = -(raw[i] == i10);

    /* post transform */
    
    }
    return stop;
}
int instr_ceqh(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    uint16_t rth[8];uint16_t rah[8];uint16_t rbh[8];
    int stop = 0;
    /* pre transform */
    reg_to_half(ctx, rth, rt);reg_to_half(ctx, rah, ra);reg_to_half(ctx, rbh, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("ceqh $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;

    for (i = 0; i < 8; ++i)
        rth[i] = -(rah[i] == rbh[i]);

    /* post transform */
    half_to_reg(ctx, rt, rth);
    }
    return stop;
}
int instr_biz(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("biz $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    if(rtw[0] == 0)
        ctx->pc = raw[0] - 4;

    /* post transform */
    
    }
    return stop;
}
int instr_ceq(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("ceq $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = -(raw[i] == rbw[i]);

    /* post transform */
    
    }
    return stop;
}
int instr_ceqb(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    uint8_t rtb[16];uint8_t rab[16];uint8_t rbb[16];
    int stop = 0;
    /* pre transform */
    reg_to_byte(ctx, rtb, rt);reg_to_byte(ctx, rab, ra);reg_to_byte(ctx, rbb, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("ceqb $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 16; ++i)
        rtb[i] = -(rab[i] == rbb[i]);

    /* post transform */
    byte_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_rotqbyi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i7)
{
    /* pre transform declare */
    uint8_t rtb[16];uint8_t rab[16];
    int stop = 0;
    /* pre transform */
    reg_to_byte(ctx, rtb, rt);reg_to_byte(ctx, rab, ra);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i7; 
    /* show disassembly*/
    dbgprintf("rotqbyi $r%d,$r%d,0x%x\n", rt,ra,i7);
    /* optional trapping */
    
    /* body */
    {
    int i;
    i7 &= 0x1f;

    for (i = 0; i < 16; ++i)
        rtb[i] = rab[(i + i7) & 15];

    /* post transform */
    byte_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_nop(spe_ctx_t *ctx, uint32_t opcode)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)opcode; 
    /* show disassembly*/
    dbgprintf("nop %08x\n", opcode);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_sumb(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 1;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("sumb $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_nor(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("nor $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = ~(raw[i] | rbw[i]);

    /* post transform */
    
    }
    return stop;
}
int instr_mpy(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 1;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("mpy $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_dsync(spe_ctx_t *ctx, uint32_t opcode)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)opcode; 
    /* show disassembly*/
    dbgprintf("dsync %08x\n", opcode);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_mpys(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("mpys $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = se16(((raw[i]&0xffff) * (rbw[i]&0xffff)) >> 16);

    /* post transform */
    
    }
    return stop;
}
int instr_gbb(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    uint8_t rtb[16];uint8_t rab[16];uint8_t rbb[16];
    int stop = 0;
    /* pre transform */
    reg_to_byte(ctx, rtb, rt);reg_to_byte(ctx, rab, ra);reg_to_byte(ctx, rbb, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("gbb $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 16; ++i)
        rtb[i] = 0;
    for (i = 0; i < 16; ++i)
        rtb[2 + (i / 8)] |= (rab[i]&1) << ((~i)&7);

    /* post transform */
    byte_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_mpyu(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("mpyu $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = (raw[i] & 0xffff) * (rbw[i] & 0xffff);

    /* post transform */
    
    }
    return stop;
}
int instr_rotmai(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i7)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i7; 
    /* show disassembly*/
    dbgprintf("rotmai $r%d,$r%d,0x%x\n", rt,ra,i7);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int shift_count = (-(int32_t)i7) & 63;
    for (i = 0; i < 4; ++i)
        if (shift_count < 32)
            rtw[i] = ((int32_t)raw[i]) >> shift_count;
        else
            rtw[i] = ((int32_t)raw[i]) >> 31;

    /* post transform */
    
    }
    return stop;
}
int instr_gbh(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 1;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("gbh $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_roti(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i7)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i7; 
    /* show disassembly*/
    dbgprintf("roti $r%d,$r%d,0x%x\n", rt,ra,i7);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int shift_count = i7 & 0x1f;
    for (i = 0; i < 4; ++i)
        rtw[i] = (raw[i] << shift_count) | (raw[i] >> (32 - shift_count));

    /* post transform */
    
    }
    return stop;
}
int instr_mpya(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb, uint32_t rc)
{
    /* pre transform declare */
    
    int stop = 1;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb;(void)rc; 
    /* show disassembly*/
    dbgprintf("mpya $r%d,$r%d,$r%d,$r%d\n", rt,ra,rb,rc);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_rdch(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("rdch $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    if (ctx->trap) return 1;
    /* body */
    {
    channel_rdch(ctx, ra, rt);

    /* post transform */
    
    }
    return stop;
}
int instr_rotm(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("rotm $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
    {
        int shift_count = (-(int32_t)rbw[i]) & 63;
        if (shift_count < 32)
            rtw[i] = raw[i] >> shift_count;
        else
            rtw[i] = 0;
    }

    /* post transform */
    
    }
    return stop;
}
int instr_xsbh(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    uint8_t rtb[16];uint8_t rab[16];uint8_t rbb[16];
    int stop = 0;
    /* pre transform */
    reg_to_byte(ctx, rtb, rt);reg_to_byte(ctx, rab, ra);reg_to_byte(ctx, rbb, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("xsbh $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 16; i += 2)
    {
        rtb[i] = rab[i + 1] & 0x80 ? 0xff : 0;
        rtb[i + 1] = rab[i + 1];
    }

    /* post transform */
    byte_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_ilhu(spe_ctx_t *ctx, uint32_t rt, uint32_t i16)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)i16; 
    /* show disassembly*/
    dbgprintf("ilhu $r%d,0x%x\n", rt,i16);
    /* optional trapping */
    
    /* body */
    {
    uint32_t s;

    s = (i16 << 16);

    rtw[0] = s;
    rtw[1] = s;
    rtw[2] = s;
    rtw[3] = s;

    /* post transform */
    
    }
    return stop;
}
int instr_cgti(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i10 = se10(i10);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("cgti $r%d,$r%d,%d\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = -(((int32_t)raw[i]) > ((int32_t)i10));

    /* post transform */
    
    }
    return stop;
}
int instr_mpyh(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("mpyh $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = ((raw[i] >> 16) * (rbw[i] & 0xffff)) << 16;

    /* post transform */
    
    }
    return stop;
}
int instr_mpyi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i10 = se10(i10);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("mpyi $r%d,$r%d,%d\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = (raw[i] & 0xffff) * i10;

    /* post transform */
    
    }
    return stop;
}
int instr_shl(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("shl $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
    {
        int shift_count = rbw[i] & 0x3f;
        if (shift_count > 31)
            rtw[i] = 0;
        else
            rtw[i] = raw[i] << shift_count;
    }

    /* post transform */
    
    }
    return stop;
}
int instr_brsl(spe_ctx_t *ctx, uint32_t rt, uint32_t i16)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i16 = se16(i16);i16 <<= 2;
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)i16; 
    /* show disassembly*/
    dbgprintf("brsl $r%d,%d\n", rt,i16);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = 0;
    rtw[0] = ctx->pc + 4;
    ctx->pc += (i16) - 4;

    /* post transform */
    
    }
    return stop;
}
int instr_shlqbybi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    uint8_t rtb[16];uint8_t rab[16];uint8_t rbb[16];
    int stop = 0;
    /* pre transform */
    reg_to_byte(ctx, rtb, rt);reg_to_byte(ctx, rab, ra);reg_to_byte(ctx, rbb, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("shlqbybi $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int shift_count = ((rbb[3])>>3) & 0x1f;

    for (i = 0; i < 16; ++i)
    {
        if ((i + shift_count) < 16)
            rtb[i] = rab[i + shift_count];
        else
            rtb[i] = 0;
    }

    /* post transform */
    byte_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_clgthi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    uint16_t rth[8];uint16_t rah[8];
    int stop = 0;
    /* pre transform */
    i10 = se10(i10);reg_to_half(ctx, rth, rt);reg_to_half(ctx, rah, ra);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("clgthi $r%d,$r%d,%d\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 8; ++i)
        rth[i] = -(rah[i] > i10);

    /* post transform */
    half_to_reg(ctx, rt, rth);
    }
    return stop;
}
int instr_sync(spe_ctx_t *ctx, uint32_t opcode)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)opcode; 
    /* show disassembly*/
    dbgprintf("sync %08x\n", opcode);
    /* optional trapping */
    
    /* body */
    {
#ifdef debug_instr
    if ((opcode >> 20) & 1)
        dbgprintf(" sync.c\n");
    else
        dbgprintf(" sync\n");
#endif

    /* post transform */
    
    }
    return stop;
}
int instr_cflts(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i8)
{
    /* pre transform declare */
    float rtf[4];float raf[4];
    int stop = 0;
    /* pre transform */
    reg_to_float(ctx, rtf, rt);reg_to_float(ctx, raf, ra);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i8; 
    /* show disassembly*/
    dbgprintf("cflts $r%d,$r%d,0x%x\n", rt,ra,i8);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = (int)raf[i];

    /* post transform */
    float_to_reg(ctx, rt, rtf);
    }
    return stop;
}
int instr_cfltu(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i8)
{
    /* pre transform declare */
    float rtf[4];float raf[4];
    int stop = 0;
    /* pre transform */
    reg_to_float(ctx, rtf, rt);reg_to_float(ctx, raf, ra);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i8; 
    /* show disassembly*/
    dbgprintf("cfltu $r%d,$r%d,0x%x\n", rt,ra,i8);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = (uint32_t)raf[i];

    /* post transform */
    float_to_reg(ctx, rt, rtf);
    }
    return stop;
}
int instr_heqi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i10 = se10(i10);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("heqi $r%d,$r%d,%d\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    if (i10 == rawp)
        stop = 1;

    /* post transform */
    
    }
    return stop;
}
int instr_cwx(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("cwx $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int t;

    t = raw[0] + rbw[0];
    t >>= 2;
    t &= 3;

    for (i = 0; i < 4; ++i)
        rtw[i] = (i == t) ? 0x00010203 : (0x01010101 * (i * 4) + 0x10111213);

    /* post transform */
    
    }
    return stop;
}
int instr_xor(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("xor $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = raw[i] ^ rbw[i];

    /* post transform */
    
    }
    return stop;
}
int instr_rotqmbi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    uint1_t rtb[128];uint1_t rab[128];uint1_t rbb[128];
    int stop = 0;
    /* pre transform */
    reg_to_bits(ctx, rtb, rt);reg_to_bits(ctx, rab, ra);reg_to_bits(ctx, rbb, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("rotqmbi $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int shift_count = (-(int32_t)rbwp) & 7;

    for (i = 0; i < 128; ++i)
    {
        if (i >= shift_count)
            rtb[i] = rab[i - shift_count];
        else
            rtb[i] = 0;
    }

    /* post transform */
    bits_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_bihz(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    uint16_t rth[8];uint16_t rah[8];uint16_t rbh[8];
    int stop = 0;
    /* pre transform */
    reg_to_half(ctx, rth, rt);reg_to_half(ctx, rah, ra);reg_to_half(ctx, rbh, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("bihz $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    if (rthp == 0)
        ctx->pc = (raw[0] << 2) - 4;

    /* post transform */
    half_to_reg(ctx, rt, rth);
    }
    return stop;
}
int instr_ceqhi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    uint16_t rth[8];uint16_t rah[8];
    int stop = 0;
    /* pre transform */
    i10 = se10(i10);reg_to_half(ctx, rth, rt);reg_to_half(ctx, rah, ra);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("ceqhi $r%d,$r%d,%d\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    int i;

    for (i = 0; i < 8; ++i)
        rth[i] = -(rah[i] == i10);

    /* post transform */
    half_to_reg(ctx, rt, rth);
    }
    return stop;
}
int instr_mpyhhau(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 1;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("mpyhhau $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_avgb(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 1;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("avgb $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_addx(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("addx $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = (rtw[i]&1) + raw[i] + rbw[i];

    /* post transform */
    
    }
    return stop;
}
int instr_rotqmby(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    uint8_t rtb[16];uint8_t rab[16];uint8_t rbb[16];
    int stop = 0;
    /* pre transform */
    reg_to_byte(ctx, rtb, rt);reg_to_byte(ctx, rab, ra);reg_to_byte(ctx, rbb, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("rotqmby $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    uint32_t s = (-(int32_t)rbw[0]) & 0x1f;

    uint32_t i;
    for (i = 0; i < 16; ++i)
        if (i >= s)
            rtb[i] = rab[i - s];
        else
            rtb[i] = 0;

    /* post transform */
    byte_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_mfspr(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("mfspr $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    printf("########## warning #################\n");
    printf(" mfspr $%d, $%d not implemented!\n", rb, rt);
    printf("####################################\n");

    /* post transform */
    
    }
    return stop;
}
int instr_stopd(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 1;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("stopd $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    if (ctx->trap) return 1;
    /* body */
    {
    printf("####### stopd instruction reached\n");
    printf("ra: %08x %08x %08x %08x\n",
        raw[0],
        raw[1],
        raw[2],
        raw[3]);
    printf("rb: %08x %08x %08x %08x\n",
        rbw[0],
        rbw[1],
        rbw[2],
        rbw[3]);
    printf("rc: %08x %08x %08x %08x\n",
        rtw[0],
        rtw[1],
        rtw[2],
        rtw[3]);

    /* post transform */
    
    }
    return stop;
}
int instr_xorhi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    
    int stop = 1;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("xorhi $r%d,$r%d,0x%x\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_cwd(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i7)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i7; 
    /* show disassembly*/
    dbgprintf("cwd $r%d,$r%d,0x%x\n", rt,ra,i7);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int t;

    t = raw[0] + i7;
    t >>= 2;
    t &= 3;

    for (i = 0; i < 4; ++i)
        rtw[i] = (i == t) ? 0x00010203 : (0x01010101 * (i * 4) + 0x10111213);

    /* post transform */
    
    }
    return stop;
}
int instr_bg(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("bg $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i]=raw[i]>rbw[i]?0:1;

    /* post transform */
    
    }
    return stop;
}
int instr_orx(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 1;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("orx $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_bi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("bi $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    ctx->pc = raw[0] - 4;

    /* post transform */
    
    }
    return stop;
}
int instr_csflt(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i8)
{
    /* pre transform declare */
    float rtf[4];float raf[4];
    int stop = 0;
    /* pre transform */
    reg_to_float(ctx, rtf, rt);reg_to_float(ctx, raf, ra);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i8; 
    /* show disassembly*/
    dbgprintf("csflt $r%d,$r%d,0x%x\n", rt,ra,i8);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i){
        int val = raw[i];
        // let's hope the x86s float hw does the right thing here...
        rtf[i] = (float)val;
    }

    /* post transform */
    float_to_reg(ctx, rt, rtf);
    }
    return stop;
}
int instr_cgx(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("cgx $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
    {
        uint64_t r = (uint64_t)(rtw[i] & 1) + (uint64_t) raw[i] + (uint64_t) rbw[i];
        rtw[i] = (r >> 32) & 1;
    }

    /* post transform */
    
    }
    return stop;
}
int instr_sfhi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    
    int stop = 1;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("sfhi $r%d,$r%d,0x%x\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_br(spe_ctx_t *ctx, uint32_t rt, uint32_t i16)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i16 = se16(i16);i16 <<= 2;
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)i16; 
    /* show disassembly*/
    dbgprintf("br $r%d,%d\n", rt,i16);
    /* optional trapping */
    
    /* body */
    {
    ctx->pc += i16 - 4;

    /* post transform */
    
    }
    return stop;
}
int instr_ori(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i10 = se10(i10);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("ori $r%d,$r%d,%d\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int imm = se10(i10);
    for (i = 0; i < 4; ++i)
        rtw[i] = raw[i] | imm;

    /* post transform */
    
    }
    return stop;
}
int instr_andi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i10 = se10(i10);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("andi $r%d,$r%d,%d\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = raw[i] & i10;

    /* post transform */
    
    }
    return stop;
}
int instr_orc(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("orc $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = raw[i] |~ rbw[i];

    /* post transform */
    
    }
    return stop;
}
int instr_frsqest(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    float rtf[4];float raf[4];float rbf[4];
    int stop = 0;
    /* pre transform */
    reg_to_float(ctx, rtf, rt);reg_to_float(ctx, raf, ra);reg_to_float(ctx, rbf, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("frsqest $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    // not quite right, but assume this is followed by a 'fi'
    for (i = 0; i < 4; ++i)
        rtf[i] = (float) (1 / sqrt(fabs(raf[i])));

    /* post transform */
    float_to_reg(ctx, rt, rtf);
    }
    return stop;
}
int instr_ila(spe_ctx_t *ctx, uint32_t rt, uint32_t i18)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)i18; 
    /* show disassembly*/
    dbgprintf("ila $r%d,0x%x\n", rt,i18);
    /* optional trapping */
    
    /* body */
    {

    rtw[0] = i18;
    rtw[1] = i18;
    rtw[2] = i18;
    rtw[3] = i18;

    /* post transform */
    
    }
    return stop;
}
int instr_xswd(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("xswd $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    rtw[0] = (raw[1] & 0x80000000) ? 0xffffffff : 0;
    rtw[1] = raw[1];
    rtw[2] = (raw[3] & 0x80000000) ? 0xffffffff : 0;
    rtw[3] = raw[3];

    /* post transform */
    
    }
    return stop;
}
int instr_ilh(spe_ctx_t *ctx, uint32_t rt, uint32_t i16)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)i16; 
    /* show disassembly*/
    dbgprintf("ilh $r%d,0x%x\n", rt,i16);
    /* optional trapping */
    
    /* body */
    {
    uint32_t s;

    s = (i16 << 16) | i16;

    rtw[0] = s;
    rtw[1] = s;
    rtw[2] = s;
    rtw[3] = s;

    /* post transform */
    
    }
    return stop;
}
int instr_bisl(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("bisl $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = 0;
    rtw[0] = ctx->pc + 4;
    ctx->pc = raw[0] - 4;

    /* post transform */
    
    }
    return stop;
}
int instr_rotqmbyi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i7)
{
    /* pre transform declare */
    uint8_t rtb[16];uint8_t rab[16];
    int stop = 0;
    /* pre transform */
    reg_to_byte(ctx, rtb, rt);reg_to_byte(ctx, rab, ra);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i7; 
    /* show disassembly*/
    dbgprintf("rotqmbyi $r%d,$r%d,0x%x\n", rt,ra,i7);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int shift_count = (-(int32_t)i7) & 0x1f;

    for (i = 0; i < 16; ++i)
    {
        if (i >= shift_count)
            rtb[i] = rab[i - shift_count];
        else
            rtb[i] = 0;
    }

    /* post transform */
    byte_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_bgx(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("bgx $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
    {
        int64_t res = ((uint64_t) raw[i]) - ((uint64_t)rbw[i]) - (uint64_t)(rtw[i] & 1);
        rtw[i] = -(res < 0);
    }

    /* post transform */
    
    }
    return stop;
}
int instr_or(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("or $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = raw[i] | rbw[i];

    /* post transform */
    
    }
    return stop;
}
int instr_hbr(spe_ctx_t *ctx, uint32_t opcode)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)opcode; 
    /* show disassembly*/
    dbgprintf("hbr %08x\n", opcode);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_brz(spe_ctx_t *ctx, uint32_t rt, uint32_t i16)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i16 = se16(i16);i16 <<= 2;
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)i16; 
    /* show disassembly*/
    dbgprintf("brz $r%d,%d\n", rt,i16);
    /* optional trapping */
    
    /* body */
    {
    if (rtw[0] == 0)
        ctx->pc += i16 - 4;

    /* post transform */
    
    }
    return stop;
}
int instr_selb(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb, uint32_t rc)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb;(void)rc; 
    /* show disassembly*/
    dbgprintf("selb $r%d,$r%d,$r%d,$r%d\n", rt,ra,rb,rc);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = (rcw[i] & rbw[i]) | ((~rcw[i]) & raw[i]);

    /* post transform */
    
    }
    return stop;
}
int instr_brhnz(spe_ctx_t *ctx, uint32_t rt, uint32_t i16)
{
    /* pre transform declare */
    uint16_t rth[8];
    int stop = 0;
    /* pre transform */
    reg_to_half(ctx, rth, rt);i16 = se16(i16);i16 <<= 2;
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)i16; 
    /* show disassembly*/
    dbgprintf("brhnz $r%d,%d\n", rt,i16);
    /* optional trapping */
    
    /* body */
    {
    if (rthp != 0)
        ctx->pc += (i16) - 4;

    /* post transform */
    half_to_reg(ctx, rt, rth);
    }
    return stop;
}
int instr_ahi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    uint16_t rth[8];uint16_t rah[8];
    int stop = 0;
    /* pre transform */
    i10 = se10(i10);reg_to_half(ctx, rth, rt);reg_to_half(ctx, rah, ra);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("ahi $r%d,$r%d,%d\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 8; ++i)
        rth[i] = rah[i] + i10;

    /* post transform */
    half_to_reg(ctx, rt, rth);
    }
    return stop;
}
int instr_cg(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("cg $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = ((raw[i] + rbw[i]) < raw[i]) ? 1 : 0;

    /* post transform */
    
    }
    return stop;
}
int instr_hbrr(spe_ctx_t *ctx, uint32_t rt, uint32_t i18)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)i18; 
    /* show disassembly*/
    dbgprintf("hbrr $r%d,0x%x\n", rt,i18);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_mpyui(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i10 = se10(i10);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("mpyui $r%d,$r%d,%d\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    int i;
    i10 &= 0xffff;
    for (i = 0; i < 4; ++i)
        rtw[i] = (raw[i] & 0xffff) * i10;

    /* post transform */
    
    }
    return stop;
}
int instr_xori(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i10 = se10(i10);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("xori $r%d,$r%d,%d\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = raw[i] ^ i10;

    /* post transform */
    
    }
    return stop;
}
int instr_fsmbi(spe_ctx_t *ctx, uint32_t rt, uint32_t i16)
{
    /* pre transform declare */
    uint8_t rtb[16];
    int stop = 0;
    /* pre transform */
    reg_to_byte(ctx, rtb, rt);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)i16; 
    /* show disassembly*/
    dbgprintf("fsmbi $r%d,0x%x\n", rt,i16);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 16; ++i)
        rtb[i] = (i16 & (1<<(15-i))) ? ~0 : 0;

    /* post transform */
    byte_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_dfs(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    double rtd[2];double rad[2];double rbd[2];
    int stop = 0;
    /* pre transform */
    reg_to_double(ctx, rtd, rt);reg_to_double(ctx, rad, ra);reg_to_double(ctx, rbd, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("dfs $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    rtd[0] = rad[0]-rbd[0];
    rtd[1] = rad[1]-rbd[1];

    /* post transform */
    double_to_reg(ctx, rt, rtd);
    }
    return stop;
}
int instr_shufb(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb, uint32_t rc)
{
    /* pre transform declare */
    uint8_t rtb[16];uint8_t rab[16];uint8_t rbb[16];uint8_t rcb[16];
    int stop = 0;
    /* pre transform */
    reg_to_byte(ctx, rtb, rt);reg_to_byte(ctx, rab, ra);reg_to_byte(ctx, rbb, rb);reg_to_byte(ctx, rcb, rc);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb;(void)rc; 
    /* show disassembly*/
    dbgprintf("shufb $r%d,$r%d,$r%d,$r%d\n", rt,ra,rb,rc);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 16; ++i)
    {
        if ((rcb[i] & 0xc0) == 0x80)
            rtb[i] = 0;
        else if ((rcb[i] & 0xe0) == 0xc0)
            rtb[i] = 0xff;
        else if ((rcb[i] & 0xe0) == 0xe0)
            rtb[i] = 0x80;
        else
        {
            int b = rcb[i] & 0x1f;
            if (rcb[i] < 16)
                rtb[i] = rab[b];
            else
                rtb[i] = rbb[b-16];
        }
    }

    /* post transform */
    byte_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_bihnz(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    uint16_t rth[8];uint16_t rah[8];uint16_t rbh[8];
    int stop = 0;
    /* pre transform */
    reg_to_half(ctx, rth, rt);reg_to_half(ctx, rah, ra);reg_to_half(ctx, rbh, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("bihnz $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    if (rthp != 0)
        ctx->pc = (raw[0] << 2) - 4;

    /* post transform */
    half_to_reg(ctx, rt, rth);
    }
    return stop;
}
int instr_bra(spe_ctx_t *ctx, uint32_t rt, uint32_t i16)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i16 = se16(i16);i16 <<= 2;
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)i16; 
    /* show disassembly*/
    dbgprintf("bra $r%d,%d\n", rt,i16);
    /* optional trapping */
    
    /* body */
    {
    ctx->pc = i16 - 4;

    /* post transform */
    
    }
    return stop;
}
int instr_chd(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i7)
{
    /* pre transform declare */
    uint16_t rth[8];uint16_t rah[8];
    int stop = 0;
    /* pre transform */
    i7 = se7(i7);reg_to_half(ctx, rth, rt);reg_to_half(ctx, rah, ra);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i7; 
    /* show disassembly*/
    dbgprintf("chd $r%d,$r%d,%d\n", rt,ra,i7);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int t = raw[0] + i7;

    t >>= 1;
    t &= 7;

    for (i = 0; i < 8; ++i)
        rth[i] = ((i == t) ? 0x0203 : (i * 2 * 0x0101 + 0x1011));

    /* post transform */
    half_to_reg(ctx, rt, rth);
    }
    return stop;
}
int instr_rchcnt(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("rchcnt $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    if (ctx->trap) return 1;
    /* body */
    {
    int i;
    for (i = 1; i < 4; ++i)
        rtw[i] = 0;

    rtw[0] = channel_rchcnt(ctx, ra);

    /* post transform */
    
    }
    return stop;
}
int instr_fsmh(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 1;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("fsmh $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_mpyhhu(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("mpyhhu $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = (raw[i] >> 16) * (rbw[i] >> 16);

    /* post transform */
    
    }
    return stop;
}
int instr_xorbi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    uint8_t rtb[16];uint8_t rab[16];
    int stop = 0;
    /* pre transform */
    reg_to_byte(ctx, rtb, rt);reg_to_byte(ctx, rab, ra);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("xorbi $r%d,$r%d,0x%x\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 16; ++i)
        rtb[i] = rab[i] ^ (i10&0xff);

    /* post transform */
    byte_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_lnop(spe_ctx_t *ctx, uint32_t opcode)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)opcode; 
    /* show disassembly*/
    dbgprintf("lnop %08x\n", opcode);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_fsmb(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 1;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("fsmb $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_fms(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb, uint32_t rc)
{
    /* pre transform declare */
    float rtf[4];float raf[4];float rbf[4];float rcf[4];
    int stop = 0;
    /* pre transform */
    reg_to_float(ctx, rtf, rt);reg_to_float(ctx, raf, ra);reg_to_float(ctx, rbf, rb);reg_to_float(ctx, rcf, rc);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb;(void)rc; 
    /* show disassembly*/
    dbgprintf("fms $r%d,$r%d,$r%d,$r%d\n", rt,ra,rb,rc);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtf[i] = (raf[i] * rbf[i])-rcf[i];

    /* post transform */
    float_to_reg(ctx, rt, rtf);
    }
    return stop;
}
int instr_andc(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("andc $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = raw[i] &~ rbw[i];

    /* post transform */
    
    }
    return stop;
}
int instr_eqv(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 1;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("eqv $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_dfm(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    double rtd[2];double rad[2];double rbd[2];
    int stop = 0;
    /* pre transform */
    reg_to_double(ctx, rtd, rt);reg_to_double(ctx, rad, ra);reg_to_double(ctx, rbd, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("dfm $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    rtd[0] = rad[0]*rbd[0];
    rtd[1] = rad[1]*rbd[1];

    /* post transform */
    double_to_reg(ctx, rt, rtd);
    }
    return stop;
}
int instr_mpyhha(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 1;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("mpyhha $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_rotma(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("rotma $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i,shift_count;
    for (i = 0; i < 4; ++i){
        shift_count = (-(int32_t)rbw[i]) & 63;
        if (shift_count < 32)
            rtw[i] = ((int32_t)raw[i]) >> shift_count;
        else
            rtw[i] = ((int32_t)raw[i]) >> 31;
    }

    /* post transform */
    
    }
    return stop;
}
int instr_chx(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    uint16_t rth[8];uint16_t rah[8];uint16_t rbh[8];
    int stop = 0;
    /* pre transform */
    reg_to_half(ctx, rth, rt);reg_to_half(ctx, rah, ra);reg_to_half(ctx, rbh, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("chx $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int t = raw[0] + rbw[0];

    t >>= 1;
    t &= 7;

    for (i = 0; i < 8; ++i)
        rth[i] = ((i == t) ? 0x0203 : (i * 2 * 0x0101 + 0x1011));

    /* post transform */
    half_to_reg(ctx, rt, rth);
    }
    return stop;
}
int instr_rothmi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i7)
{
    /* pre transform declare */
    uint16_t rth[8];uint16_t rah[8];
    int stop = 0;
    /* pre transform */
    reg_to_half(ctx, rth, rt);reg_to_half(ctx, rah, ra);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i7; 
    /* show disassembly*/
    dbgprintf("rothmi $r%d,$r%d,0x%x\n", rt,ra,i7);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int shift_count = (-(int32_t)i7) & 31;
    for (i = 0; i < 8; ++i)
        if (shift_count < 16)
            rth[i] = rah[i] >> shift_count;
        else
            rth[i] = 0;

    /* post transform */
    half_to_reg(ctx, rt, rth);
    }
    return stop;
}
int instr_rotmi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i7)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i7; 
    /* show disassembly*/
    dbgprintf("rotmi $r%d,$r%d,0x%x\n", rt,ra,i7);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int shift_count = (-(int32_t)i7) & 63;
    for (i = 0; i < 4; ++i)
        if (shift_count < 32)
            rtw[i] = raw[i] >> shift_count;
        else
            rtw[i] = 0;

    /* post transform */
    
    }
    return stop;
}
int instr_clgtbi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    uint8_t rtb[16];uint8_t rab[16];
    int stop = 0;
    /* pre transform */
    i10 = se10(i10);reg_to_byte(ctx, rtb, rt);reg_to_byte(ctx, rab, ra);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("clgtbi $r%d,$r%d,%d\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 16; ++i)
        rtb[i] = -(rab[i] > (i10&0xff));

    /* post transform */
    byte_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_fma(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb, uint32_t rc)
{
    /* pre transform declare */
    float rtf[4];float raf[4];float rbf[4];float rcf[4];
    int stop = 0;
    /* pre transform */
    reg_to_float(ctx, rtf, rt);reg_to_float(ctx, raf, ra);reg_to_float(ctx, rbf, rb);reg_to_float(ctx, rcf, rc);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb;(void)rc; 
    /* show disassembly*/
    dbgprintf("fma $r%d,$r%d,$r%d,$r%d\n", rt,ra,rb,rc);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtf[i] = rcf[i] + (raf[i] * rbf[i]);

    /* post transform */
    float_to_reg(ctx, rt, rtf);
    }
    return stop;
}
int instr_dfnms(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    double rtd[2];double rad[2];double rbd[2];
    int stop = 0;
    /* pre transform */
    reg_to_double(ctx, rtd, rt);reg_to_double(ctx, rad, ra);reg_to_double(ctx, rbd, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("dfnms $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    // almost same as dfms. This is missing in SPU_ISA_1.2. See v.1.1
    rtd[0] = (rad[0]*rbd[0])-rtd[0];
    rtd[1] = (rad[1]*rbd[1])-rtd[1];

    /* post transform */
    double_to_reg(ctx, rt, rtd);
    }
    return stop;
}
int instr_fesd(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    float rtf[4];float raf[4];float rbf[4];
    int stop = 0;
    /* pre transform */
    reg_to_float(ctx, rtf, rt);reg_to_float(ctx, raf, ra);reg_to_float(ctx, rbf, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("fesd $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    double result = raf[0];
    float *tmp = (float*)&result;
    rtf[0] = tmp[1];
    rtf[1] = tmp[0];
    result = raf[2];
    rtf[2] = tmp[1];
    rtf[3] = tmp[0];

    /* post transform */
    float_to_reg(ctx, rt, rtf);
    }
    return stop;
}
int instr_clgtb(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    uint8_t rtb[16];uint8_t rab[16];uint8_t rbb[16];
    int stop = 0;
    /* pre transform */
    reg_to_byte(ctx, rtb, rt);reg_to_byte(ctx, rab, ra);reg_to_byte(ctx, rbb, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("clgtb $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 16; ++i)
        rtb[i] = -(rab[i] > rbb[i]);

    /* post transform */
    byte_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_clgti(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i10 = se10(i10);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("clgti $r%d,$r%d,%d\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = -(raw[i] > i10);

    /* post transform */
    
    }
    return stop;
}
int instr_clgth(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    uint16_t rth[8];uint16_t rah[8];uint16_t rbh[8];
    int stop = 0;
    /* pre transform */
    reg_to_half(ctx, rth, rt);reg_to_half(ctx, rah, ra);reg_to_half(ctx, rbh, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("clgth $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 8; ++i)
        rth[i] = -(rah[i] > rbh[i]);

    /* post transform */
    half_to_reg(ctx, rt, rth);
    }
    return stop;
}
int instr_shli(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i7)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i7; 
    /* show disassembly*/
    dbgprintf("shli $r%d,$r%d,0x%x\n", rt,ra,i7);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int shift_count = i7 & 0x3f;
    for (i = 0; i < 4; ++i)
    {
        if (shift_count > 31)
            rtw[i] = 0;
        else
            rtw[i] = raw[i] << shift_count;
    }

    /* post transform */
    
    }
    return stop;
}
int instr_fnms(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb, uint32_t rc)
{
    /* pre transform declare */
    float rtf[4];float raf[4];float rbf[4];float rcf[4];
    int stop = 0;
    /* pre transform */
    reg_to_float(ctx, rtf, rt);reg_to_float(ctx, raf, ra);reg_to_float(ctx, rbf, rb);reg_to_float(ctx, rcf, rc);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb;(void)rc; 
    /* show disassembly*/
    dbgprintf("fnms $r%d,$r%d,$r%d,$r%d\n", rt,ra,rb,rc);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtf[i] = rcf[i] - (raf[i] * rbf[i]);

    /* post transform */
    float_to_reg(ctx, rt, rtf);
    }
    return stop;
}
int instr_shlqbii(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i7)
{
    /* pre transform declare */
    uint1_t rtb[128];uint1_t rab[128];
    int stop = 0;
    /* pre transform */
    reg_to_bits(ctx, rtb, rt);reg_to_bits(ctx, rab, ra);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i7; 
    /* show disassembly*/
    dbgprintf("shlqbii $r%d,$r%d,0x%x\n", rt,ra,i7);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int shift_count = i7 & 7;

    for (i = 0; i < 128; ++i)
    {
        if (i + shift_count < 128)
            rtb[i] = rab[i + shift_count];
        else
            rtb[i] = 0;
    }

    /* post transform */
    bits_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_ceqbi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    uint8_t rtb[16];uint8_t rab[16];
    int stop = 0;
    /* pre transform */
    i10 = se10(i10);reg_to_byte(ctx, rtb, rt);reg_to_byte(ctx, rab, ra);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("ceqbi $r%d,$r%d,%d\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 16; ++i)
        rtb[i] = -(rab[i] == (i10 & 0xff));

    /* post transform */
    byte_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_dfa(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    double rtd[2];double rad[2];double rbd[2];
    int stop = 0;
    /* pre transform */
    reg_to_double(ctx, rtd, rt);reg_to_double(ctx, rad, ra);reg_to_double(ctx, rbd, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("dfa $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    rtd[0] = rad[0]+rbd[0];
    rtd[1] = rad[1]+rbd[1];

    /* post transform */
    double_to_reg(ctx, rt, rtd);
    }
    return stop;
}
int instr_shlqby(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    uint8_t rtb[16];uint8_t rab[16];uint8_t rbb[16];
    int stop = 0;
    /* pre transform */
    reg_to_byte(ctx, rtb, rt);reg_to_byte(ctx, rab, ra);reg_to_byte(ctx, rbb, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("shlqby $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int shift = rbw[0] & 0x1f;

    for (i = 0; i < 16; ++i)
        rtb[i] = (i + shift) < 16 ? rab[i + shift] : 0;

    /* post transform */
    byte_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_shlqbyi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i7)
{
    /* pre transform declare */
    uint8_t rtb[16];uint8_t rab[16];
    int stop = 0;
    /* pre transform */
    reg_to_byte(ctx, rtb, rt);reg_to_byte(ctx, rab, ra);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i7; 
    /* show disassembly*/
    dbgprintf("shlqbyi $r%d,$r%d,0x%x\n", rt,ra,i7);
    /* optional trapping */
    
    /* body */
    {
    int i;
    i7 &= 0x1f;

    for (i = 0; i < 16; ++i)
        rtb[i] = (i + i7) >= 16 ? 0 : rab[i + i7];

    /* post transform */
    byte_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_fceq(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    float rtf[4];float raf[4];float rbf[4];
    int stop = 0;
    /* pre transform */
    reg_to_float(ctx, rtf, rt);reg_to_float(ctx, raf, ra);reg_to_float(ctx, rbf, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("fceq $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    //note: floats are not accurately modeled!
    int i;
    int itrue = 0xffffffff;
    float ftrue = *(float*)&itrue;
    for (i = 0; i < 4; ++i)
        rtf[i] = raf[i]==rbf[i]?ftrue:(float) 0.0;

    /* post transform */
    float_to_reg(ctx, rt, rtf);
    }
    return stop;
}
int instr_shlqbi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    uint1_t rtb[128];uint1_t rab[128];uint1_t rbb[128];
    int stop = 0;
    /* pre transform */
    reg_to_bits(ctx, rtb, rt);reg_to_bits(ctx, rab, ra);reg_to_bits(ctx, rbb, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("shlqbi $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int shift_count = rbwp & 7;

    for (i = 0; i < 128; ++i)
    {
        if (i + shift_count < 128)
            rtb[i] = rab[i + shift_count];
        else
            rtb[i] = 0;
    }

    /* post transform */
    bits_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_and(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("and $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = raw[i] & rbw[i];

    /* post transform */
    
    }
    return stop;
}
int instr_stqd(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i10 = se10(i10);i10 <<= 4;
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("stqd $r%d,$r%d,%d\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    reg2ls(ctx, rt, i10 + raw[0]);

    /* post transform */
    
    }
    return stop;
}
int instr_cbd(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i7)
{
    /* pre transform declare */
    uint8_t rtb[16];uint8_t rab[16];
    int stop = 0;
    /* pre transform */
    i7 = se7(i7);reg_to_byte(ctx, rtb, rt);reg_to_byte(ctx, rab, ra);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i7; 
    /* show disassembly*/
    dbgprintf("cbd $r%d,$r%d,%d\n", rt,ra,i7);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int t = (raw[0] + i7) & 0xf;

    for (i = 0; i < 16; ++i)
        rtb[i] = ((i == t) ? 0x03 : (i|0x10));

    /* post transform */
    byte_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_stqa(spe_ctx_t *ctx, uint32_t rt, uint32_t i16)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i16 = se16(i16);i16 <<= 2;
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)i16; 
    /* show disassembly*/
    dbgprintf("stqa $r%d,%d\n", rt,i16);
    /* optional trapping */
    
    /* body */
    {
    reg2ls(ctx, rt, i16);

    /* post transform */
    
    }
    return stop;
}
int instr_ai(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i10 = se10(i10);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("ai $r%d,$r%d,%d\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = raw[i] + i10;

    /* post transform */
    
    }
    return stop;
}
int instr_ah(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    uint16_t rth[8];uint16_t rah[8];uint16_t rbh[8];
    int stop = 0;
    /* pre transform */
    reg_to_half(ctx, rth, rt);reg_to_half(ctx, rah, ra);reg_to_half(ctx, rbh, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("ah $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 8; ++i)
        rth[i] = rah[i] + rbh[i];

    /* post transform */
    half_to_reg(ctx, rt, rth);
    }
    return stop;
}
int instr_rotqby(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    uint8_t rtb[16];uint8_t rab[16];uint8_t rbb[16];
    int stop = 0;
    /* pre transform */
    reg_to_byte(ctx, rtb, rt);reg_to_byte(ctx, rab, ra);reg_to_byte(ctx, rbb, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("rotqby $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int shift = rbw[0] & 0xf;

    for (i = 0; i < 16; ++i)
        rtb[i] = rab[(i + shift) & 15];

    /* post transform */
    byte_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_hbra(spe_ctx_t *ctx, uint32_t rt, uint32_t i18)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)i18; 
    /* show disassembly*/
    dbgprintf("hbra $r%d,0x%x\n", rt,i18);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_stqr(spe_ctx_t *ctx, uint32_t rt, uint32_t i16)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i16 = se16(i16);i16 <<= 2;
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)i16; 
    /* show disassembly*/
    dbgprintf("stqr $r%d,%d\n", rt,i16);
    /* optional trapping */
    
    /* body */
    {
    reg2ls(ctx, rt, ctx->pc + i16);

    /* post transform */
    
    }
    return stop;
}
int instr_il(spe_ctx_t *ctx, uint32_t rt, uint32_t i16)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i16 = se16(i16);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)i16; 
    /* show disassembly*/
    dbgprintf("il $r%d,%d\n", rt,i16);
    /* optional trapping */
    
    /* body */
    {
    rtw[0] = i16;
    rtw[1] = i16;
    rtw[2] = i16;
    rtw[3] = i16;

    /* post transform */
    
    }
    return stop;
}
int instr_cbx(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    uint8_t rtb[16];uint8_t rab[16];uint8_t rbb[16];
    int stop = 0;
    /* pre transform */
    reg_to_byte(ctx, rtb, rt);reg_to_byte(ctx, rab, ra);reg_to_byte(ctx, rbb, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("cbx $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int t = (raw[0] + rbw[0]) & 0xf;

    for (i = 0; i < 16; ++i)
        rtb[i] = ((i == t) ? 0x03 : (i|0x10));

    /* post transform */
    byte_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_mtspr(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("mtspr $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    printf("########## warning #################\n");
    printf(" mtspr $%d, $%d not implemented!\n", rb, rt);
    printf("####################################\n");

    /* post transform */
    
    }
    return stop;
}
int instr_stqx(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("stqx $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    reg2ls(ctx, rt, raw[0] + rbw[0]);

    /* post transform */
    
    }
    return stop;
}
int instr_cgt(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("cgt $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = -(((int32_t)raw[i]) > ((int32_t)rbw[i]));

    /* post transform */
    
    }
    return stop;
}
int instr_lqx(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("lqx $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    ls2reg(ctx, rt, raw[0] + rbw[0]);

    /* post transform */
    
    }
    return stop;
}
int instr_rotqbybi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("rotqbybi $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    uint32_t shift_count = (rbw[0] & 0x7f) >> 3;
    while (shift_count--)
    {
        raw[0] = (raw[0] << 8) | (raw[1] >> 24);
        raw[1] = (raw[1] << 8) | (raw[2] >> 24);
        raw[2] = (raw[2] << 8) | (raw[3] >> 24);
        raw[3] = (raw[3] << 8) | (raw[0] >> 24);
    }

    /* post transform */
    
    }
    return stop;
}
int instr_lqr(spe_ctx_t *ctx, uint32_t rt, uint32_t i16)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i16 = se16(i16);i16 <<= 2;
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)i16; 
    /* show disassembly*/
    dbgprintf("lqr $r%d,%d\n", rt,i16);
    /* optional trapping */
    
    /* body */
    {
    ls2reg(ctx, rt, ctx->pc + i16);

    /* post transform */
    
    }
    return stop;
}
int instr_rotqbi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("rotqbi $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    uint32_t shift_count = (rbw[0] & 0x7f) & 7;
    rtw[0] = raw[0];
    rtw[1] = raw[1];
    rtw[2] = raw[2];
    rtw[3] = raw[3];
    while (shift_count--)
    {
        uint32_t t = (rtw[1] >> 31) | 2 * rtw[0];
        rtw[1] = (rtw[2] >> 31) | 2 * rtw[1];
        rtw[2] = (rtw[3] >> 31) | 2 * rtw[2];
        rtw[3] = (rtw[0] >> 31) | 2 * rtw[3];
        rtw[0] = t;
    }

    /* post transform */
    
    }
    return stop;
}
int instr_wrch(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("wrch $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    if (ctx->trap) return 1;
    /* body */
    {
    channel_wrch(ctx, ra, rt);

    /* post transform */
    
    }
    return stop;
}
int instr_lqd(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i10 = se10(i10);i10 <<= 4;
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("lqd $r%d,$r%d,%d\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    ls2reg(ctx, rt, i10 + raw[0]);

    /* post transform */
    
    }
    return stop;
}
int instr_cgthi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    uint16_t rth[8];uint16_t rah[8];
    int stop = 0;
    /* pre transform */
    i10 = se10(i10);reg_to_half(ctx, rth, rt);reg_to_half(ctx, rah, ra);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("cgthi $r%d,$r%d,%d\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 8; ++i)
        rth[i] = -(((int16_t)rah[i]) > ((int16_t)i10));

    /* post transform */
    half_to_reg(ctx, rt, rth);
    }
    return stop;
}
int instr_rotqmbybi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    uint8_t rtb[16];uint8_t rab[16];uint8_t rbb[16];
    int stop = 0;
    /* pre transform */
    reg_to_byte(ctx, rtb, rt);reg_to_byte(ctx, rab, ra);reg_to_byte(ctx, rbb, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("rotqmbybi $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int shift_count = (0-((rbb[3])>>3)) & 0x1f;

    for (i = 0; i < 16; ++i)
    {
        if (i >= shift_count)
            rtb[i] = rab[i - shift_count];
        else
            rtb[i] = 0;
    }

    /* post transform */
    byte_to_reg(ctx, rt, rtb);
    }
    return stop;
}
int instr_lqa(spe_ctx_t *ctx, uint32_t rt, uint32_t i16)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i16 = se16(i16);i16 <<= 2;
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)i16; 
    /* show disassembly*/
    dbgprintf("lqa $r%d,%d\n", rt,i16);
    /* optional trapping */
    
    /* body */
    {
    ls2reg(ctx, rt, i16);

    /* post transform */
    
    }
    return stop;
}
int instr_fs(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    float rtf[4];float raf[4];float rbf[4];
    int stop = 0;
    /* pre transform */
    reg_to_float(ctx, rtf, rt);reg_to_float(ctx, raf, ra);reg_to_float(ctx, rbf, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("fs $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtf[i] = raf[i] - rbf[i];

    /* post transform */
    float_to_reg(ctx, rt, rtf);
    }
    return stop;
}
int instr_brasl(spe_ctx_t *ctx, uint32_t rt, uint32_t i16)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i16 = se16(i16);i16 <<= 2;
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)i16; 
    /* show disassembly*/
    dbgprintf("brasl $r%d,%d\n", rt,i16);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = 0;
    rtw[0] = ctx->pc + 4;
    ctx->pc = (i16) - 4;

    /* post transform */
    
    }
    return stop;
}
int instr_sfx(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("sfx $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = rbw[i] - raw[i] - ((rtw[i]&1)?0:1);

    /* post transform */
    
    }
    return stop;
}
int instr_brnz(spe_ctx_t *ctx, uint32_t rt, uint32_t i16)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i16 = se16(i16);i16 <<= 2;
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)i16; 
    /* show disassembly*/
    dbgprintf("brnz $r%d,%d\n", rt,i16);
    /* optional trapping */
    
    /* body */
    {
    if (rtw[0] != 0)
        ctx->pc += i16 - 4;

    /* post transform */
    
    }
    return stop;
}
int instr_andhi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    uint16_t rth[8];uint16_t rah[8];
    int stop = 0;
    /* pre transform */
    reg_to_half(ctx, rth, rt);reg_to_half(ctx, rah, ra);i10 = se10(i10);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("andhi $r%d,$r%d,%d\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 8; ++i)
        rth[i] = rah[i] & i10;

    /* post transform */
    half_to_reg(ctx, rt, rth);
    }
    return stop;
}
int instr_fa(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    float rtf[4];float raf[4];float rbf[4];
    int stop = 0;
    /* pre transform */
    reg_to_float(ctx, rtf, rt);reg_to_float(ctx, raf, ra);reg_to_float(ctx, rbf, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("fa $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtf[i] = raf[i] + rbf[i];

    /* post transform */
    float_to_reg(ctx, rt, rtf);
    }
    return stop;
}
int instr_orhi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    
    int stop = 1;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("orhi $r%d,$r%d,0x%x\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {

    /* post transform */
    
    }
    return stop;
}
int instr_sfh(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    uint16_t rth[8];uint16_t rah[8];uint16_t rbh[8];
    int stop = 0;
    /* pre transform */
    reg_to_half(ctx, rth, rt);reg_to_half(ctx, rah, ra);reg_to_half(ctx, rbh, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("sfh $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 8; ++i)
        rth[i] = rbh[i] - rah[i];

    /* post transform */
    half_to_reg(ctx, rt, rth);
    }
    return stop;
}
int instr_sfi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i10)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    i10 = se10(i10);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i10; 
    /* show disassembly*/
    dbgprintf("sfi $r%d,$r%d,%d\n", rt,ra,i10);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int imm = se10(i10);
    for (i = 0; i < 4; ++i)
        rtw[i] = imm - raw[i];

    /* post transform */
    
    }
    return stop;
}
int instr_xshw(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    uint16_t rth[8];uint16_t rah[8];uint16_t rbh[8];
    int stop = 0;
    /* pre transform */
    reg_to_half(ctx, rth, rt);reg_to_half(ctx, rah, ra);reg_to_half(ctx, rbh, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("xshw $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 8; i += 2)
    {
        rth[i] = (rah[i + 1] & 0x8000) ? 0xffff : 0;
        rth[i + 1] = rah[i + 1];
    }

    /* post transform */
    half_to_reg(ctx, rt, rth);
    }
    return stop;
}
int instr_fi(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    float rtf[4];float raf[4];float rbf[4];
    int stop = 0;
    /* pre transform */
    reg_to_float(ctx, rtf, rt);reg_to_float(ctx, raf, ra);reg_to_float(ctx, rbf, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("fi $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    // not quite right, but assume this was preceeded by a 'fr*est'
    for (i = 0; i < 4; ++i)
        rtf[i] = rbf[i];

    /* post transform */
    float_to_reg(ctx, rt, rtf);
    }
    return stop;
}
int instr_fm(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    float rtf[4];float raf[4];float rbf[4];
    int stop = 0;
    /* pre transform */
    reg_to_float(ctx, rtf, rt);reg_to_float(ctx, raf, ra);reg_to_float(ctx, rbf, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("fm $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtf[i] = raf[i] * rbf[i];


    /* post transform */
    float_to_reg(ctx, rt, rtf);
    }
    return stop;
}
int instr_a(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("a $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = raw[i] + rbw[i];

    /* post transform */
    
    }
    return stop;
}
int instr_frest(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    float rtf[4];float raf[4];float rbf[4];
    int stop = 0;
    /* pre transform */
    reg_to_float(ctx, rtf, rt);reg_to_float(ctx, raf, ra);reg_to_float(ctx, rbf, rb);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("frest $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    // not quite right, but assume this is followed by a 'fi'
    for (i = 0; i < 4; ++i)
        rtf[i] = 1/raf[1];

    /* post transform */
    float_to_reg(ctx, rt, rtf);
    }
    return stop;
}
int instr_fsm(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("fsm $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = (rawp & (8>>i)) ? ~0 : 0;

    /* post transform */
    
    }
    return stop;
}
int instr_binz(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("binz $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    if(rtw[0] != 0)
        ctx->pc = raw[0] - 4;

    /* post transform */
    
    }
    return stop;
}
int instr_sf(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t rb)
{
    /* pre transform declare */
    
    int stop = 0;
    /* pre transform */
    
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)rb; 
    /* show disassembly*/
    dbgprintf("sf $r%d,$r%d,$r%d\n", rt,ra,rb);
    /* optional trapping */
    
    /* body */
    {
    int i;
    for (i = 0; i < 4; ++i)
        rtw[i] = rbw[i] - raw[i];

    /* post transform */
    
    }
    return stop;
}
int instr_rotqbii(spe_ctx_t *ctx, uint32_t rt, uint32_t ra, uint32_t i7)
{
    /* pre transform declare */
    uint1_t rtb[128];uint1_t rab[128];
    int stop = 0;
    /* pre transform */
    reg_to_bits(ctx, rtb, rt);reg_to_bits(ctx, rab, ra);
    /* ignore unused arguments */
    (void)*ctx;(void)rt;(void)ra;(void)i7; 
    /* show disassembly*/
    dbgprintf("rotqbii $r%d,$r%d,0x%x\n", rt,ra,i7);
    /* optional trapping */
    
    /* body */
    {
    int i;
    int shift_count = i7 & 7;

    for (i = 0; i < 128; ++i)
    {
        rtb[i] = rab[(i + shift_count) & 127];
    }

    /* post transform */
    bits_to_reg(ctx, rt, rtb);
    }
    return stop;
}

