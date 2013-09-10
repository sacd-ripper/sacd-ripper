// Copyright 2010 fail0verflow <master@fail0verflow.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

#include "types.h"
#include "config.h"
#include "channel.h"

#define MFC_GET_CMD 0x40
#define MFC_PUT_CMD 0x20

/// <summary>
/// A list of all documented MFC channels
/// </summary>
enum mfc_channels
{
    /// <summary>
    /// Write multisource synchronization request
    /// </summary>
    MFC_WrMSSyncReq = 9,
    /// <summary>
    /// Read tag mask
    /// </summary>
    MFC_RdTagMask = 12,
    /// <summary>
    /// Write local memory address command parameter
    /// </summary>
    MFC_LSA = 16,
    /// <summary>
    /// Write high order DMA effective address command parameter
    /// </summary>
    MFC_EAH = 17,
    /// <summary>
    /// Write low order DMA effective address command parameter
    /// </summary>
    MFC_EAL = 18,
    /// <summary>
    /// Write DMA transfer size command parameter
    /// </summary>
    MFC_Size = 19,
    /// <summary>
    /// Write tag identifier command parameter
    /// </summary>
    MFC_TagID = 20,
    /// <summary>
    /// Write and enqueue DMA command with associated class ID
    /// </summary>
    MFC_Cmd = 21,
    /// <summary>
    /// Write tag mask
    /// </summary>
    MFC_WrTagMask = 22,
    /// <summary>
    /// Write request for conditional or unconditional tag status update
    /// </summary>
    MFC_WrTagUpdate = 23,
    /// <summary>
    /// Read tag status with mask applied
    /// </summary>
    MFC_WrTagStat = 24,
    /// <summary>
    /// Read DMA list stall-and-notify status
    /// </summary>
    MFC_RdListStallStat = 25,
    /// <summary>
    /// Write DMA list stall-and-notify acknowledge
    /// </summary>
    MFC_WrListStallAck = 26,
    /// <summary>
    /// Read completion status of last completed immediate MFC atomic update command (see the Synergistic Processor Unit Channels section of Cell Broadband Engine Architecture) 
    /// </summary>
    MFC_RdAtomicStat = 27
};

enum channels
{
    /// <summary>
    /// Read event status with mask applied 
    /// </summary>
    SPU_RdEventStat = 0,
    /// <summary>
    /// Write event mask 
    /// </summary>
    SPU_WrEventMask = 1,
    /// <summary>
    /// Write end of event processing 
    /// </summary>
    SPU_WrEventAck = 2,
    /// <summary>
    /// Signal notification 1
    /// </summary>
    SPU_RdSigNotify1 = 3,
    /// <summary>
    /// Signal notification 2 
    /// </summary>
    SPU_RdSigNotify2 = 4,
    /// <summary>
    /// Write decrementer count 
    /// </summary>
    SPU_WrDec = 7,
    /// <summary>
    /// Read decrementer count
    /// </summary>
    SPU_RdDec = 8,
    /// <summary>
    /// Read event mask 
    /// </summary>
    SPU_RdEventMask = 11,
    /// <summary>
    /// Read SPU run status 
    /// </summary>
    SPU_RdMachStat = 13,
    /// <summary>
    /// Write SPU machine state save/restore register 0 (SRR0) 
    /// </summary>
    SPU_WrSRR0 = 14,
    /// <summary>
    /// Read SPU machine state save/restore register 0 (SRR0)
    /// </summary>
    SPU_RdSRR0 = 15,
    /// <summary>
    /// Write outbound mailbox contents 
    /// </summary>
    SPU_WrOutMbox = 28,
    /// <summary>
    /// Read inbound mailbox contents 
    /// </summary>
    SPU_RdInMbox = 29,
    /// <summary>
    /// Write outbound interrupt mailbox contents (interrupting PPU) 
    /// </summary>
    SPU_WrOutIntrMbox = 30,
    /// <summary>
    /// Read random number
    /// </summary>
    SPU_RdRand = 74,
};

void handle_mfc_command(spe_ctx_t *ctx, uint32_t cmd)
{
    spe_mfc_command_area_t *mfc = &ctx->mfc;
    dbgprintf("Local address %08x, EA = %08x:%08x, Size=%08x, TagID=%08x, Cmd=%08x\n",
		mfc->mfc_lsa, mfc->mfc_eah, mfc->mfc_eal, mfc->mfc_size_tag, mfc->mfc_class_id_cmd, cmd);

    switch (cmd)
	{
	case MFC_GET_CMD:
		dbgprintf("MFC_GET (DMA into LS)\n");
        memcpy(ctx->ls + mfc->mfc_lsa, (void *) mfc->mfc_eal, mfc->mfc_size_tag);
        break;
    case MFC_PUT_CMD:
        dbgprintf("MFC_PUT (DMA out of LS)\n");
        memcpy((void *) mfc->mfc_eal, ctx->ls + mfc->mfc_lsa, mfc->mfc_size_tag);
        break;
	default:
		dbgprintf("unknown command\n");
	}
}

void channel_wrch(spe_ctx_t *ctx, int ch, int reg)
{
    spe_mfc_command_area_t *mfc = &ctx->mfc;
	uint32_t r = ctx->reg[reg][0];
    dbgprintf("CHANNEL: wrch ch%d r%d\n", ch, reg);
	
	switch (ch)
	{
    case 7:
        break;
	case MFC_LSA:
		dbgprintf("MFC_LSA %08x\n", r);
		mfc->mfc_lsa = r;
		break;
	case MFC_EAH:
		dbgprintf("MFC_EAH %08x\n", r);
		mfc->mfc_eah = r;
		break;
	case MFC_EAL:
		dbgprintf("MFC_EAL %08x\n", r);
		mfc->mfc_eal = r;
		break;
	case MFC_Size:
		dbgprintf("MFC_Size %08x\n", r);
		mfc->mfc_size_tag = r;
		break;
	case MFC_TagID:
		dbgprintf("MFC_TagID %08x\n", r);
		mfc->mfc_class_id_cmd = r;
		break;
	case MFC_Cmd:
		dbgprintf("MFC_Cmd %08x\n", r);
		handle_mfc_command(ctx, r);
		break;
	case MFC_WrTagMask:
		dbgprintf("MFC_WrTagMask %08x\n", r);
		mfc->prxy_query_mask = r;
		break;
	case MFC_WrTagUpdate:
		dbgprintf("MFC_WrTagUpdate %08x\n", r);
		mfc->prxy_tag_status = mfc->prxy_query_mask;
        break;
	case MFC_WrListStallAck:
		dbgprintf("MFC_WrListStallAck %08x\n", r);
		break;
	case MFC_RdAtomicStat:
		dbgprintf("MFC_RdAtomicStat %08x\n", r);
		break;
    case SPU_WrOutMbox:
        dbgprintf("SPU_WrOutMbox %08x\n", r);
        //1 entry
        if (ctx->spu_out_cnt == 1)
        {
            ctx->spu_out_mbox = r;
            ctx->spu_out_cnt = 0;
        }
        break;
    case SPU_WrOutIntrMbox:
        dbgprintf("SPU_WrOutIntrMbox %08x\n", r);
        //1 entry, ppu gets an interrupt if this one is written
        if (ctx->spu_out_intr_cnt == 1)
        {
            ctx->spu_out_intr_mbox = r;
            ctx->spu_out_intr_cnt = 0;
        }
        break;
	default:
		dbgprintf("UNKNOWN CHANNEL\n");
	}
}

void channel_rdch(spe_ctx_t *ctx, int ch, int reg)
{
    spe_mfc_command_area_t *mfc = &ctx->mfc;
    uint32_t r = 0;
	dbgprintf("CHANNEL: rdch ch%d r%d\n", ch, reg);
	
	switch (ch)
	{
	case MFC_WrTagStat:
        r = mfc->prxy_tag_status;
		dbgprintf("MFC_WrTagStat %08x\n", r);
		break;
	case MFC_RdAtomicStat:
        //r = mfc_atomicstat;
		dbgprintf("MFC_RdAtomicStat %08x\n", r);
		break;
    case SPU_RdInMbox:
        dbgprintf("SPU_RdInMbox contains %d items\n", ctx->spu_in_cnt);
        //4 entries, returns the oldest written
        if (ctx->spu_in_cnt < 4)
        {
            r = ctx->spu_in_mbox[ctx->spu_in_rdidx]; //get oldest entry
            dbgprintf("SPU_RdInMbox: setting to %08x\n", r);
            ctx->spu_in_rdidx++; // next
            ctx->spu_in_rdidx &= 3; // wrap around
            ctx->spu_in_cnt++; //one less entry

            ctx->reg[reg][0] = r;
            ctx->reg[reg][1] = r;
            ctx->reg[reg][2] = r;
            ctx->reg[reg][3] = r;
            return;
        }
        break;
    case SPU_RdRand:
        r = rand();
        break;
    default:
        dbgprintf("UNKNOWN CHANNEL\n");
	}
    
    ctx->reg[reg][0] = r;
	ctx->reg[reg][1] = 0;
	ctx->reg[reg][2] = 0;
	ctx->reg[reg][3] = 0;
}

int channel_rchcnt(spe_ctx_t *ctx, int ch)
{
	uint32_t r = 0;

    switch (ch)
	{
	case MFC_WrTagUpdate:
		r = 1;
		break;
	case MFC_WrTagStat:
		r = 1;
		dbgprintf("MFC_WrTagStat %08x\n", r);
		break;
	case MFC_RdAtomicStat:
        r = 1;
		dbgprintf("MFC_RdAtomicStat %08x\n", r);
		break;
    case SPU_WrOutMbox:
        //1 entry, return 0 if full, 1 if empty (not written before)
        r = ctx->spu_out_cnt;
        dbgprintf("SPU_WrOutMbox %08x\n", r);
        break;
    case SPU_RdInMbox:
        //4 entries, return 0 if empty? dunno
        r = ctx->spu_in_cnt;
        dbgprintf("SPU_RdInMbox: ");
        break;
    case SPU_WrOutIntrMbox:
        //1 entry, return 0 if full, 1 if empty (not written before)
        r = ctx->spu_out_intr_cnt;
        dbgprintf("SPU_WrOutIntrMbox %08x\n", r);
        break;
    case SPU_RdRand:
        r = 1;
        break;
	default:
		dbgprintf("unknown channel %d\n", ch);
	}
	return r;
}
