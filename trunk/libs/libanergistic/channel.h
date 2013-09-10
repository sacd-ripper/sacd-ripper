// Copyright 2010 fail0verflow <master@fail0verflow.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef CHANNELS_H__
#define CHANNELS_H__

//SPU channels.
#define SPU_RdEventStat 0
#define SPU_WrEventMask 1
#define SPU_WrEventAck 2
#define SPU_RdSigNotify1 3
#define SPU_RdSigNotify2 4
#define SPU_WrDec 7
#define SPU_RdDec 8
#define SPU_RdEventMask 11
#define SPU_RdMachStat 13
#define SPU_WrSRR0 14
#define SPU_RdSRR0 15
#define SPU_WrOutMbox 28
#define SPU_RdInMbox 29
#define SPU_WrOutIntrMbox 30

//MFC channels.
#define MFC_WrMSSyncReq 9
#define MFC_RdTagMask 12
#define MFC_LSA 16
#define MFC_EAH 17
#define MFC_EAL 18
#define MFC_Size 19
#define MFC_TagID 20
#define MFC_Cmd 21
#define MFC_WrTagMask 22
#define MFC_WrTagUpdate 23
#define MFC_RdTagStat 24
#define MFC_RdListStallStat 25
#define MFC_WrListStallAck 26
#define MFC_RdAtomicStat 27

//MFC DMA commands.
#define MFC_PUT_CMD 0x20
#define MFC_GET_CMD 0x40

//MFC tag update commands.
#define MFC_TAG_UPDATE_IMMEDIATE 0
#define MFC_TAG_UPDATE_ANY 1
#define MFC_TAG_UPDATE_ALL 2

void channel_wrch(int ch, int reg);
void channel_rdch(int ch, int reg);
int channel_rchcnt(int ch);

#endif
