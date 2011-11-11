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

/* Without this, SSE2 intrinsics will be compiled in.
 * Even something as simple as detecting SSE2 capability within DST_InitDecoder()
 * then using that to determine which code path to use, impacts peformance significantly */
//#define NO_SSE2

#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#if !defined(NO_SSE2) && (defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__))
#include <emmintrin.h>
#endif
#include "dst_ac.h"
#include "types.h"
#include "dst_fram.h"
#include "unpack_dst.h"

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

static void FillTable4Bit(int NrOfChannels, int NrOfBitsPerCh, Segment *S, int8_t Table4Bit[MAX_CHANNELS][MAX_DSDBITS_INFRAME])
{
    int BitNr;
    int ChNr;
    int SegNr;
    int Start;
    int End;
    int8_t Val;

    for (ChNr = 0; ChNr < NrOfChannels; ChNr++)
    {
        int8_t *Table4BitCh = Table4Bit[ChNr];
        for (SegNr = 0, Start = 0; SegNr < S->NrOfSegments[ChNr] - 1; SegNr++)
        {
            Val = (int8_t) S->Table4Segment[ChNr][SegNr];
            End = Start + S->Resolution * 8 * S->SegmentLen[ChNr][SegNr];
            for (BitNr = Start; BitNr < End; BitNr++)
            {
                Table4Bit[ChNr][BitNr] = Val;
            }
            Start += S->Resolution * 8 * S->SegmentLen[ChNr][SegNr];
        }

        Val = (int8_t) S->Table4Segment[ChNr][SegNr];
        memset(&Table4BitCh[Start], Val, NrOfBitsPerCh - Start);
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
static const int16_t reverse[128] = { 
    1,  65,  33,  97,  17,  81,  49, 113,   9,  73,  41, 105,  25,  89,  57, 121,
    5,  69,  37, 101,  21,  85,  53, 117,  13,  77,  45, 109,  29,  93,  61, 125,
    3,  67,  35,  99,  19,  83,  51, 115,  11,  75,  43, 107,  27,  91,  59, 123,
    7,  71,  39, 103,  23,  87,  55, 119,  15,  79,  47, 111,  31,  95,  63, 127,
    2,  66,  34,  98,  18,  82,  50, 114,  10,  74,  42, 106,  26,  90,  58, 122,
    6,  70,  38, 102,  22,  86,  54, 118,  14,  78,  46, 110,  30,  94,  62, 126,
    4,  68,  36, 100,  20,  84,  52, 116,  12,  76,  44, 108,  28,  92,  60, 124,
    8,  72,  40, 104,  24,  88,  56, 120,  16,  80,  48, 112,  32,  96,  64, 128 };

static int16_t Reverse7LSBs(int16_t c)
{
    return reverse[(c + (1 << SIZE_PREDCOEF)) & 127];
}

static void LT_PRECOMPUTE(ebunch *D, int16_t LT_ICoefA[2 * MAX_CHANNELS][16][256], uint8_t LT_Status[MAX_CHANNELS][16])
{
    int FilterNr, FilterLength, TableNr, k, i, j, ChNr;

    for (FilterNr = 0; FilterNr < D->FrameHdr.NrOfFilters; FilterNr++)
    {
        FilterLength = D->FrameHdr.PredOrder[FilterNr];
        for (TableNr = 0; TableNr < 16; TableNr++)
        {
            k = FilterLength - TableNr * 8;
            if (k > 8)
            {
                k = 8;
			}
			else if (k < 0)
			{
                k = 0;
            }
            for (i = 0; i < 256; i++)
            {
                int16_t cvalue = 0;
                for (j = 0; j < k; j++)
                {
                    cvalue += (int16_t)(((i >> j) & 1) * 2 - 1) * D->FrameHdr.ICoefA[FilterNr][TableNr * 8 + j];
                }
                LT_ICoefA[FilterNr][TableNr][i] = cvalue;
            }
        }
    }

    for (ChNr = 0; ChNr < D->FrameHdr.NrOfChannels; ChNr++)
    {
        for (TableNr = 0; TableNr < 16; TableNr++)
        {
            LT_Status[ChNr][TableNr] = 0xaa;
        }
    }
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
/*            D->P_one[][], D->AData[], D->ADataLen,                       */
/*                                                                         */
/* post     : D->WM.Pwm                                                    */
/*                                                                         */
/***************************************************************************/

#define FilterTable LT_ICoefA[Filter]
#define ChannelStatus LT_Status[ChNr]

#define LT_CHANNEL_FIR() \
    Predict  = FilterTable[ 0][ChannelStatus[ 0]]; \
    Predict += FilterTable[ 1][ChannelStatus[ 1]]; \
    Predict += FilterTable[ 2][ChannelStatus[ 2]]; \
    Predict += FilterTable[ 3][ChannelStatus[ 3]]; \
    Predict += FilterTable[ 4][ChannelStatus[ 4]]; \
    Predict += FilterTable[ 5][ChannelStatus[ 5]]; \
    Predict += FilterTable[ 6][ChannelStatus[ 6]]; \
    Predict += FilterTable[ 7][ChannelStatus[ 7]]; \
    Predict += FilterTable[ 8][ChannelStatus[ 8]]; \
    Predict += FilterTable[ 9][ChannelStatus[ 9]]; \
    Predict += FilterTable[10][ChannelStatus[10]]; \
    Predict += FilterTable[11][ChannelStatus[11]]; \
    Predict += FilterTable[12][ChannelStatus[12]]; \
    Predict += FilterTable[13][ChannelStatus[13]]; \
    Predict += FilterTable[14][ChannelStatus[14]]; \
    Predict += FilterTable[15][ChannelStatus[15]];

#if !defined(NO_SSE2) && (defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__))
#define LT_CHANNEL_FIR_SSE2() \
{ \
    __m128i p0, p1, s; \
    \
    p0 = _mm_set_epi16( \
        FilterTable[ 7][ChannelStatus[ 7]], \
        FilterTable[ 6][ChannelStatus[ 6]], \
        FilterTable[ 5][ChannelStatus[ 5]], \
        FilterTable[ 4][ChannelStatus[ 4]], \
        FilterTable[ 3][ChannelStatus[ 3]], \
        FilterTable[ 2][ChannelStatus[ 2]], \
        FilterTable[ 1][ChannelStatus[ 1]], \
        FilterTable[ 0][ChannelStatus[ 0]] \
    ); \
    p1 = _mm_set_epi16( \
        FilterTable[15][ChannelStatus[15]], \
        FilterTable[14][ChannelStatus[14]], \
        FilterTable[13][ChannelStatus[13]], \
        FilterTable[12][ChannelStatus[12]], \
        FilterTable[11][ChannelStatus[11]], \
        FilterTable[10][ChannelStatus[10]], \
        FilterTable[ 9][ChannelStatus[ 9]], \
        FilterTable[ 8][ChannelStatus[ 8]] \
    ); \
    s = _mm_add_epi16(p0, p1); \
    s = _mm_add_epi16(s, _mm_shuffle_epi32(s, _MM_SHUFFLE(2, 3, 0, 1))); \
    s = _mm_add_epi16(s, _mm_shuffle_epi32(s, _MM_SHUFFLE(0, 1, 2, 3))); \
    s = _mm_add_epi16(s, _mm_srli_si128(s, 2)); \
    \
    /* SSSE3 specific instructions, that aren't any faster on Nehalem */ \
    /* s = _mm_hadds_epi16(s, s); */ \
    /* s = _mm_hadds_epi16(s, s); */ \
    /* s = _mm_hadds_epi16(s, s); */ \
    \
    Predict = (int16_t)(_mm_cvtsi128_si32(s) & 0xffff); \
}
#endif

#define LT_CHANNEL_BIT(method, bit, oper) \
    for (ChNr = 0; ChNr < NrOfChannels; ChNr++) \
    { \
        /* Calculate output value of the FIR filter */ \
        int16_t Predict; \
        uint8_t Residual; \
        int16_t BitVal; \
        \
        const int Filter = D->FrameHdr.Filter4Bit[ChNr][BitNr]; \
        \
        method; \
        \
        /* Arithmetic decode the incoming bit */ \
        if ((D->FrameHdr.HalfProb[ChNr]/* == 1*/) && (BitNr < D->FrameHdr.NrOfHalfBits[ChNr])) \
        { \
            DST_ACDecodeBit(&AC, &Residual, AC_PROBS / 2, D->AData, D->ADataLen, 0); \
        } \
        else \
        { \
            const int table4bit = D->FrameHdr.Ptable4Bit[ChNr][BitNr]; \
            const int PtableIndex = DST_ACGetPtableIndex(Predict, D->FrameHdr.PtableLen[table4bit]); \
            \
            DST_ACDecodeBit(&AC, &Residual, D->P_one[table4bit][PtableIndex], D->AData, D->ADataLen, 0); \
        } \
        \
        /* Channel bit depends on the predicted bit and BitResidual[][] */ \
        BitVal = ((((uint16_t)Predict) >> 15) ^ Residual) & 1; \
        \
        /* Shift the result into the correct bit position */ \
        MuxedDSD[ChNr] oper (uint8_t)(BitVal << bit); \
        \
        /* Update filter */ \
        { \
            uint32_t* st = (uint32_t*)LT_Status[ChNr]; \
            st[3] = (st[3] << 1) | ((st[2] >> 31) & 1); \
            st[2] = (st[2] << 1) | ((st[1] >> 31) & 1); \
            st[1] = (st[1] << 1) | ((st[0] >> 31) & 1); \
            st[0] = (st[0] << 1) | BitVal; \
        } \
    } \
    BitNr++;

int DST_FramDSTDecode(uint8_t *DSTdata, uint8_t *MuxedDSDdata, int FrameSizeInBytes, int FrameCnt, ebunch *D)
{
    int       retval = 0;
    int       BitNr, BitIndexNr;
    int       ChNr;
    uint8_t   ACError;
    const int NrOfBitsPerCh = D->FrameHdr.NrOfBitsPerCh;
    const int NrOfChannels = D->FrameHdr.NrOfChannels;
    uint8_t   *MuxedDSD = MuxedDSDdata;

    D->FrameHdr.FrameNr       = FrameCnt;
    D->FrameHdr.CalcNrOfBytes = FrameSizeInBytes;
    D->FrameHdr.CalcNrOfBits  = D->FrameHdr.CalcNrOfBytes * 8;

    /* unpack DST frame: segmentation, mapping, arithmatic data */
    UnpackDSTframe(D, DSTdata, MuxedDSDdata);

    if (D->FrameHdr.DSTCoded == 1)
    {
        ACData AC;
        __declspec(align(16)) int16_t LT_ICoefA[2 * MAX_CHANNELS][16][256];
        __declspec(align(16)) uint8_t LT_Status[MAX_CHANNELS][16];

        FillTable4Bit(NrOfChannels, NrOfBitsPerCh, &D->FrameHdr.FSeg, D->FrameHdr.Filter4Bit);
        FillTable4Bit(NrOfChannels, NrOfBitsPerCh, &D->FrameHdr.PSeg, D->FrameHdr.Ptable4Bit);

        LT_PRECOMPUTE(D, LT_ICoefA, LT_Status);

        AC.Init = 1;
        DST_ACDecodeBit(&AC, &ACError, Reverse7LSBs(D->FrameHdr.ICoefA[0][0]), D->AData, D->ADataLen, 0);

        BitIndexNr = 0;

        /* icc predicts the 'else' case to have higher probabiilty */
        if (!D->SSE2 || 1)
        {
            for (BitNr = 0; BitNr < NrOfBitsPerCh - 7; MuxedDSD += NrOfChannels)
            {
                LT_CHANNEL_BIT(LT_CHANNEL_FIR(), 7,  =);
                LT_CHANNEL_BIT(LT_CHANNEL_FIR(), 6, |=);
                LT_CHANNEL_BIT(LT_CHANNEL_FIR(), 5, |=);
                LT_CHANNEL_BIT(LT_CHANNEL_FIR(), 4, |=);
                LT_CHANNEL_BIT(LT_CHANNEL_FIR(), 3, |=);
                LT_CHANNEL_BIT(LT_CHANNEL_FIR(), 2, |=);
                LT_CHANNEL_BIT(LT_CHANNEL_FIR(), 1, |=);
                LT_CHANNEL_BIT(LT_CHANNEL_FIR(), 0, |=);
            }
        }
#if !defined(NO_SSE2) && (defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__))
        else
        {
            for (BitNr = 0; BitNr < NrOfBitsPerCh - 7; MuxedDSD += NrOfChannels)
            {
                LT_CHANNEL_BIT(LT_CHANNEL_FIR_SSE2(), 7,  =);
                LT_CHANNEL_BIT(LT_CHANNEL_FIR_SSE2(), 6, |=);
                LT_CHANNEL_BIT(LT_CHANNEL_FIR_SSE2(), 5, |=);
                LT_CHANNEL_BIT(LT_CHANNEL_FIR_SSE2(), 4, |=);
                LT_CHANNEL_BIT(LT_CHANNEL_FIR_SSE2(), 3, |=);
                LT_CHANNEL_BIT(LT_CHANNEL_FIR_SSE2(), 2, |=);
                LT_CHANNEL_BIT(LT_CHANNEL_FIR_SSE2(), 1, |=);
                LT_CHANNEL_BIT(LT_CHANNEL_FIR_SSE2(), 0, |=);
            }
        }
#endif

        /* Flush the arithmetic decoder */
        DST_ACDecodeBit(&AC, &ACError, 0, D->AData, D->ADataLen, 1);

        if (ACError != 1)
        {
            fprintf(stderr, "ERROR: Arithmetic decoding error!\n");
            retval = 1;
        }
    }

    return retval;
}
