// Copyright 2010 fail0verflow <master@fail0verflow.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

#include "types.h"
#include "main.h"
#include "config.h"
#include "channel.h"

//Channel names.
static char *ch_names[] = 
{
  "SPU_RdEventStat",        //0
  "SPU_WrEventMask",        //1
  "SPU_WrEventAck",         //2
  "SPU_RdSigNotify1",       //3
  "SPU_RdSigNotify2",       //4
  "UNKNOWN",                //5
  "UNKNOWN",                //6
  "SPU_WrDec",              //7
  "SPU_RdDec",              //8
  "MFC_WrMSSyncReq",        //9
  "UNKNOWN",               //10
  "SPU_RdEventMask",       //11
  "MFC_RdTagMask",         //12
  "SPU_RdMachStat",        //13
  "SPU_WrSRR0",            //14
  "SPU_RdSRR0",            //15
  "MFC_LSA",               //16
  "MFC_EAH",               //17
  "MFC_EAL",               //18
  "MFC_Size",              //19
  "MFC_TagID",             //20
  "MFC_Cmd",               //21
  "MFC_WrTagMask",         //22
  "MFC_WrTagUpdate",       //23
  "MFC_RdTagStat",         //24
  "MFC_RdListStallStat",   //25
  "MFC_WrListStallAck",    //26
  "MFC_RdAtomicStat",      //27
  "SPU_WrOutMbox",         //28
  "SPU_RdInMbox",          //29
  "SPU_WrOutIntrMbox"      //30
};

//MFC channel values.
static u32 _MFC_LSA;
static u32 _MFC_EAH;
static u32 _MFC_EAL;
static u32 _MFC_Size;
static u32 _MFC_TagID;
static u32 _MFC_TagMask;
static u32 _MFC_TagStat;

#define MFC_GET_CMD 0x40
#define MFC_SNDSIG_CMD 0xA0

//Endian swap.
#define _BE(val) ((((val) & 0xff00000000000000ull) >> 56) | \
                  (((val) & 0x00ff000000000000ull) >> 40) | \
                  (((val) & 0x0000ff0000000000ull) >> 24) | \
                  (((val) & 0x000000ff00000000ull) >> 8 ) | \
                  (((val) & 0x00000000ff000000ull) << 8 ) | \
                  (((val) & 0x0000000000ff0000ull) << 24) | \
                  (((val) & 0x000000000000ff00ull) << 40) | \
                  (((val) & 0x00000000000000ffull) << 56))

void handle_mfc_command(u32 cmd)
{
	printf("Local address %08x, EA = %08x:%08x, Size=%08x, TagID=%08x, Cmd=%08x\n",
		MFC_LSA, MFC_EAH, MFC_EAL, MFC_Size, MFC_TagID, cmd);
#ifdef DEBUG_CHANNEL
	getchar();
#endif
		
	switch (cmd)
	{
	case MFC_PUT_CMD:
		printf("MFC_PUT (DMA out of LS)\n");
    {
			FILE *fp = fopen("out.bin", "a+");
			fwrite(ctx->ls + _MFC_LSA, sizeof(u8), _MFC_Size, fp);
			fclose(fp);
		}
		break;
	case MFC_GET_CMD:
		printf("MFC_GET (DMA into LS)\n");
		{
			static int round = 0;
			                         //module debug output address, set both to 0x0 for none
			static u64 data0[2] = {_BE(0xbeef0110dead0000), _BE(0xbeef0220dead0000)};
			static u64 data1[4] = {_BE(0xbeef0110dead0000), _BE(0xbeef0220dead0000), 
			                       _BE(0x0000000200000000), _BE(0x0000000000000000)};
			                                  //^-- 1,2,3,4 (device type/id, pscode, psid)
			if(round == 0)
				memcpy(ctx->ls + _MFC_LSA, &data0, _MFC_Size);
			else if(round == 1)
				memcpy(ctx->ls + _MFC_LSA, &data1, _MFC_Size);
			else if(round == 2)
			{
				//Load EID segment.
				printf("loading EID...");
				FILE *fp = fopen("eid_tmp", "rb");
				fread(ctx->ls + _MFC_LSA, sizeof(u8), _MFC_Size, fp);
				fclose(fp);
				printf("done\n");
			}
			round++;
		}
		break;
	default:
		printf("unknown command\n");
	}
}

void handle_mfc_tag_update(u32 tag)
{
	switch (tag)
	{
	case MFC_TAG_UPDATE_IMMEDIATE:
		printf("-> MFC_TAG_UPDATE_IMMEDIATE\n");
		_MFC_TagStat = _MFC_TagMask;
		break;
	case MFC_TAG_UPDATE_ANY:
		printf("-> MFC_TAG_UPDATE_ANY\n");
		break;
	case MFC_TAG_UPDATE_ALL:
		printf("-> MFC_TAG_UPDATE_ALL\n");
		break;
	default:
		printf("-> UNKNOWN\n");
		break;
	}
	
	_MFC_TagStat = _MFC_TagMask;
}

void channel_wrch(int ch, int reg)
{
	u32 r = ctx->reg[reg][0];
	
	printf("CHANNEL: wrch ch%d(= %s) r%d(= 0x%08x)\n", ch, (ch <= 30 ? ch_names[ch] : "UNKNOWN"), reg, r);
#ifdef DEBUG_CHANNEL
	getchar();
#endif
	
	switch (ch)
	{
	case SPU_WrDec: //write decrementer
		break;		
	case MFC_LSA:
		printf("MFC_LSA %08x\n", r);
		_MFC_LSA = r;
		break;
	case MFC_EAH:
		printf("MFC_EAH %08x\n", r);
		_MFC_EAH = r;
		break;
	case MFC_EAL:
		printf("MFC_EAL %08x\n", r);
		_MFC_EAL = r;
		break;
	case MFC_Size:
		printf("MFC_Size %08x\n", r);
		_MFC_Size = r;
		break;
	case MFC_TagID:
		printf("MFC_TagID %08x\n", r);
		_MFC_TagID = r;
		break;
	case MFC_Cmd:
		printf("MFC_Cmd %08x\n", r);
		handle_mfc_command(r);
		break;
	case MFC_WrTagMask:
		printf("MFC_WrTagMask %08x\n", r);
		_MFC_TagMask = r;
		break;
	case MFC_WrTagUpdate:
		printf("MFC_WrTagUpdate %08x\n", r);
		handle_mfc_tag_update(r);
		break;
	case MFC_WrListStallAck:
		printf("MFC_WrListStallAck %08x\n", r);
		break;
	case MFC_RdAtomicStat:
		printf("MFC_RdAtomicStat %08x\n", r);
		break;
	default:
		printf("UNKNOWN CHANNEL\n");
	}
}

void channel_rdch(int ch, int reg)
{
	u32 r = 0;
	
	printf("CHANNEL: rdch ch%d(= %s) r%d\n", ch, (ch <= 30 ? ch_names[ch] : "UNKNOWN"), reg);
#ifdef DEBUG_CHANNEL
	getchar();
#endif
	
	switch (ch)
	{
	case SPU_RdDec: //read decrementer
		break;
	case MFC_RdTagStat:
		r = _MFC_TagStat;
		printf("MFC_RdTagStat %08x\n", r);
		break;
	case MFC_RdAtomicStat:
		printf("MFC_RdAtomicStat %08x\n", r);
		break;
	}
	
	//Set register
	ctx->reg[reg][0] = r;
	ctx->reg[reg][1] = 0;
	ctx->reg[reg][2] = 0;
	ctx->reg[reg][3] = 0;
}

int channel_rchcnt(int ch)
{
	u32 r = 0;
	
	printf("CHANNEL: rchcnt ch%d(%s)\n", ch, (ch <= 30 ? ch_names[ch] : "UNKNOWN"));
#ifdef DEBUG_CHANNEL
	getchar();
#endif
	
	switch (ch)
	{
	case MFC_WrTagUpdate:
		r = 1;
		break;
	case MFC_RdTagStat:
		r = 1;
		printf("MFC_RdTagStat %08x\n", r);
		break;
	case MFC_RdAtomicStat:
		printf("MFC_RdAtomicStat %08x\n", r);
		break;
	default:
		printf("unknown channel %d\n", ch);
	}
	return r;
}
