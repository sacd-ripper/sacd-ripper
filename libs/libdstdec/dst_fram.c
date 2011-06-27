/***********************************************************************
MPEG-4 Audio RM Module
Lossless coding of 1-bit oversampled audio - DST (Direct Stream Transfer)

This software was originally developed by:

* Aad Rijnberg 
  Philips Digital Systems Laboratories Eindhoven 
  <aad.rijnberg@philips.com>

* Fons Bruekers
  Philips Research Laboratories Eindhoven
  <fons.bruekers@philips.com>
   
* Eric Knapen
  Philips Digital Systems Laboratories Eindhoven
  <h.w.m.knapen@philips.com> 

And edited by:

* Richard Theelen
  Philips Digital Systems Laboratories Eindhoven
  <r.h.m.theelen@philips.com>

* Maxim Anisiutkin
  ICT Group
  <maxim.anisiutkin@gmail.com>

in the course of development of the MPEG-4 Audio standard ISO-14496-1, 2 and 3.
This software module is an implementation of a part of one or more MPEG-4 Audio
tools as specified by the MPEG-4 Audio standard. ISO/IEC gives users of the
MPEG-4 Audio standards free licence to this software module or modifications
thereof for use in hardware or software products claiming conformance to the
MPEG-4 Audio standards. Those intending to use this software module in hardware
or software products are advised that this use may infringe existing patents.
The original developers of this software of this module and their company,
the subsequent editors and their companies, and ISO/EIC have no liability for
use of this software module or modifications thereof in an implementation.
Copyright is not released for non MPEG-4 Audio conforming products. The
original developer retains full right to use this code for his/her own purpose,
assign or donate the code to a third party and to inhibit third party from
using the code for non MPEG-4 Audio conforming products. This copyright notice
must be included in all copies of derivative works.

Copyright © 2004.

Source file: dst_fram.c (Frame processing of the DST Coding)

Required libraries: <none>

Authors:
RT:  Richard Theelen, PDSL-labs Eindhoven <r.h.m.theelen@philips.com>
MA:  Maxim Anisiutkin, ICT Group <maxim.anisiutkin@gmail.com>

Changes:
08-Mar-2004 RT  Initial version
26-Jun-2011 MA  Improved performance with the unrolled FIR cycle

************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <malloc.h>
#include <stdio.h>
#include "dst_ac.h"
#include "types.h"
#include "dst_fram.h"
#include "UnpackDST.h"

#define FIR_ADD(i) \
    case (i + 1): \
        PredictVal += Status[Pnt + i] * ICoefA[i]; \

#define FIR_ADD16(base) \
    FIR_ADD(base + 0x0f); \
    FIR_ADD(base + 0x0e); \
    FIR_ADD(base + 0x0d); \
    FIR_ADD(base + 0x0c); \
    FIR_ADD(base + 0x0b); \
    FIR_ADD(base + 0x0a); \
    FIR_ADD(base + 0x09); \
    FIR_ADD(base + 0x08); \
    FIR_ADD(base + 0x07); \
    FIR_ADD(base + 0x06); \
    FIR_ADD(base + 0x05); \
    FIR_ADD(base + 0x04); \
    FIR_ADD(base + 0x03); \
    FIR_ADD(base + 0x02); \
    FIR_ADD(base + 0x01); \
    FIR_ADD(base + 0x00); \

#define FIR_RUN() \
    { \
        int Pnt; \
        long PredictVal; \
        char* Status; \
        int* ICoefA; \
        Pnt = D->FirPtrs.Pnt[ChNr]; \
        PredictVal = 0; \
        Status = D->FirPtrs.Status[ChNr]; \
        ICoefA = D->FrameHdr.ICoefA[D->FrameHdr.Filter4Bit[ChNr][BitNr]]; \
        switch (D->FrameHdr.PredOrder[D->FrameHdr.Filter4Bit[ChNr][BitNr]]) { \
            FIR_ADD16(0x70); \
            FIR_ADD16(0x60); \
            FIR_ADD16(0x50); \
            FIR_ADD16(0x40); \
            FIR_ADD16(0x30); \
            FIR_ADD16(0x20); \
            FIR_ADD16(0x10); \
            FIR_ADD16(0x00); \
        } \
        D->PredicVal[ChNr][BitNr] = PredictVal; \
    } \

/***************************************************************************/
/*                                                                         */
/* name     : FillTable4Bit                                                */
/*                                                                         */
/* function : Fill an array that indicates for each bit of each channel    */
/*            which table number must be used.                             */
/*                                                                         */
/* pre      : NrOfChannels, NrOfBitsPerCh, S->NrOfSegments[],              */
/*            S->SegmentLen[][], S->Resolution, S->Table4Segment[][]       */
/*                                                                         */
/* post     : Table4Bit[][]                                                */
/*                                                                         */
/***************************************************************************/

static void FillTable4Bit(int NrOfChannels, int NrOfBitsPerCh, Segment *S, int Table4Bit[MAX_CHANNELS][MAX_DSDBITS_INFRAME])
{
    int BitNr;
    int ChNr;
    int SegNr;
    int Start;
  
    for (ChNr = 0; ChNr < NrOfChannels; ChNr++)
    {
        for (SegNr = 0, Start = 0; SegNr < S->NrOfSegments[ChNr] - 1; SegNr++)
        {
            for (BitNr = Start; BitNr < Start + S->Resolution * 8 * S->SegmentLen[ChNr][SegNr]; BitNr++)
            {
                Table4Bit[ChNr][BitNr] = S->Table4Segment[ChNr][SegNr];
            }
            Start += S->Resolution * 8 * S->SegmentLen[ChNr][SegNr];
        }
        for (BitNr = Start; BitNr < NrOfBitsPerCh; BitNr++)
        {
            Table4Bit[ChNr][BitNr] = S->Table4Segment[ChNr][SegNr];
        }
    }
}

/***************************************************************************/
/*                                                                         */
/* name     : Reverse7LSBs                                                 */
/*                                                                         */
/* function : Take the 7 LSBs of a number consisting of SIZE_PREDCOEF bits */
/*            (2's complement), reverse the bit order and add 1 to it.     */
/*                                                                         */
/* pre      : c                                                            */
/*                                                                         */
/* post     : Returns the translated number                                */
/*                                                                         */
/***************************************************************************/

static int Reverse7LSBs(int c)
{
    int LSBs;
    int i;
    int p;
  
    if (c >= 0)
    {
        LSBs = c & 127;
    }
    else
    {
        LSBs = ((1 << SIZE_PREDCOEF) + c) & 127;
    }
    for (i = 0, p = 1; i < 7; i++)
    {
        if ((LSBs & (1 << i)) != 0)
        {
            p += 1 << (6 - i);
        }
    }
    return p;
}


/***************************************************************************/
/*                                                                         */
/* name     : DST_FramDSTDecode                                            */
/*                                                                         */
/* function : DST decode a complete frame (all channels)     .             */
/*                                                                         */
/* pre      : D->CodOpt  : .NrOfBitsPerCh, .NrOfChannels,                  */
/*            D->FrameHdr: .PredOrder[], .NrOfHalfBits[], .ICoefA[][],     */
/*                         .NrOfFilters, .NrOfPtables, .FrameNr            */
/*            D->BitResidual[][], D->P_one[][], D->AData[], D->ADataLen,   */
/*                                                                         */
/* post     : D->WM.Pwm, D->BitStream11[][]                                */
/*                                                                         */
/***************************************************************************/

int DST_FramDSTDecode(uint8_t *DSTdata, uint8_t *MuxedDSDdata, int FrameSizeInBytes, int FrameCnt, ebunch *D)
{
    int     retval = 0;
    int     BitNr,  BitIndexNr;
    int     ByteNr, ByteIndexNr;
    uint8_t BitMask[RESOL];
    int     ChNr;
    uint8_t ACError;
    int     i;
    int8_t  PredicBit;
    int     PtableIndex;

    D->FrameHdr.FrameNr       = FrameCnt;
    D->FrameHdr.CalcNrOfBytes = FrameSizeInBytes;
    D->FrameHdr.CalcNrOfBits  = D->FrameHdr.CalcNrOfBytes * 8;
    /* unpack DST frame: segmentation, mapping, arithmatic data */
    UnpackDSTframe(D, DSTdata, MuxedDSDdata);
    if (D->FrameHdr.DSTCoded == 1)
    {
        FillTable4Bit(D->FrameHdr.NrOfChannels, D->FrameHdr.NrOfBitsPerCh, &D->FrameHdr.FSeg, D->FrameHdr.Filter4Bit);
        FillTable4Bit(D->FrameHdr.NrOfChannels, D->FrameHdr.NrOfBitsPerCh, &D->FrameHdr.PSeg, D->FrameHdr.Ptable4Bit);
        D->DstXbits.PBit = Reverse7LSBs(D->FrameHdr.ICoefA[0][0]);
        DST_ACDecodeBit(&D->DstXbits.Bit, D->DstXbits.PBit, D->AData, D->ADataLen, 0);
        /* Initialise the Pnt and Status pointers for each channel */
        for (ChNr = 0; ChNr < D->FrameHdr.NrOfChannels; ChNr++)
        {
            for (i = 0; i < (1 << SIZE_CODEDPREDORDER); i++)
            {
                D->FirPtrs.Status[ChNr][i] = 2 * (i & 1) - 1;
                D->FirPtrs.Status[ChNr][i + (1 << SIZE_CODEDPREDORDER)] = D->FirPtrs.Status[ChNr][i];
            }
            D->FirPtrs.Pnt[ChNr] = 0;
        }
        /* Deinterleaving of the channels is incorporated in these two loops */
        for (BitNr = 0; BitNr < D->FrameHdr.NrOfBitsPerCh; BitNr++)
        {
            for (ChNr = 0; ChNr < D->FrameHdr.NrOfChannels; ChNr++)
            {
                /* FIR unrolled loop */
                FIR_RUN();
                /* Arithmetic decode the incoming bit */
                if ((D->FrameHdr.HalfProb[ChNr] == 1) && (BitNr < D->FrameHdr.NrOfHalfBits[ChNr]))
                {
                    DST_ACDecodeBit(&(D->BitResidual[ChNr][BitNr]), AC_PROBS / 2, D->AData, D->ADataLen, 0);
                }
                else
                {
                    PtableIndex = DST_ACGetPtableIndex(D->PredicVal[ChNr][BitNr], D->FrameHdr.PtableLen[D->FrameHdr.Ptable4Bit[ChNr][BitNr]]);
                    DST_ACDecodeBit(&(D->BitResidual[ChNr][BitNr]), D->P_one[D->FrameHdr.Ptable4Bit[ChNr][BitNr]][PtableIndex], D->AData, D->ADataLen, 0);
                }
                /* Channel bit depends on the predicted bit and BitResidual[][] */
                if (D->PredicVal[ChNr][BitNr] >= 0)
                {
                    PredicBit = 1;
                }
                else
                {
                    PredicBit = -1;
                }
                if (D->BitResidual[ChNr][BitNr] == 1)
                {
                    D->BitStream11[ChNr][BitNr] = (int8_t)(-PredicBit);
                }
                else
                {
                    D->BitStream11[ChNr][BitNr] = (int8_t)(+PredicBit);
                }
                /* Update filter */
                D->FirPtrs.Pnt[ChNr]--;
                if (D->FirPtrs.Pnt[ChNr] < 0)
                {
                    D->FirPtrs.Pnt[ChNr] += (1 << SIZE_CODEDPREDORDER);
                }
                D->FirPtrs.Status[ChNr][D->FirPtrs.Pnt[ChNr]] = D->BitStream11[ChNr][BitNr];
                D->FirPtrs.Status[ChNr][D->FirPtrs.Pnt[ChNr] + (1 << SIZE_CODEDPREDORDER)] = D->BitStream11[ChNr][BitNr];
            }
        }
        /* Flush the arithmetic decoder */
        DST_ACDecodeBit(&ACError, 0, D->AData, D->ADataLen, 1);
        if (ACError != 0)
        {
            fprintf(stderr, "ERROR: Arithmetic decoding error!\n");
            retval = 1;
        }

        /* Reshuffle bits from BitStream11 to DsdFrame such, that DsdFrame is
         * arranged according the DSDIFF (DSD) output format
         */
        if (retval == 0)
        {
            /* Fill BitMask array (1, 2, 4, 8, 16, 32, 64, 128) */
            for (BitNr = 0; BitNr < 8; BitNr++)
            {
                BitMask[BitNr] = (uint8_t)(1 << BitNr);
            }
            for  (ChNr = 0; ChNr < D->FrameHdr.NrOfChannels; ChNr++)
            {
                for (ByteNr = 0; ByteNr < D->FrameHdr.NrOfBitsPerCh/8; ByteNr++)
                {
                    ByteIndexNr = ByteNr * D->FrameHdr.NrOfChannels + ChNr;
                    /* Reset all bits of the byte */
                    MuxedDSDdata[ByteIndexNr] = 0;
                    /* Set the apprioprate bits of each DF[] */
                    BitIndexNr = ByteNr * 8;
                    for (BitNr = 7; BitNr >= 0; BitNr--, BitIndexNr++)
                    {
                        if (D->BitStream11[ChNr][BitIndexNr] == 1)
                        {
                            MuxedDSDdata[ByteIndexNr] |= BitMask[BitNr];
                        }
                    }
                }
            }
        }
    }
    return retval;
}
