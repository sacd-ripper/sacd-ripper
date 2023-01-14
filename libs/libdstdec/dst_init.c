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

Source file: DST_init.c (DST Coder Initialisation)

Required libraries: <none>

Authors:
RT:  Richard Theelen, PDSL-labs Eindhoven <r.h.m.theelen@philips.com>

Changes:
08-Mar-2004 RT  Initial version

************************************************************************/


/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifndef __APPLE__
#include <malloc.h>
#endif
#if !defined(NO_SSE2) && (defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__))
#include <emmintrin.h>
#endif
#include "dst_init.h"
#include "ccp_calc.h"
#include "conststr.h"
#include "types.h"

/*============================================================================*/
/*       STATIC FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* General function for allocating memory for array of any type */
static void *MemoryAllocate(int NrOfElements, int SizeOfElement) 
{
  void *Array;
#if defined(__arm__) || defined(__aarch64__)
  if ((Array = malloc(NrOfElements * SizeOfElement)) == NULL)
  {
    fprintf(stderr,"ERROR: not enough memory available!\n\n");
  }
#else
  if ((Array = _mm_malloc(NrOfElements * SizeOfElement, 16)) == NULL) 
  {
    fprintf(stderr,"ERROR: not enough memory available!\n\n");
  }
#endif
  return Array;
}

static void MemoryFree(void *Array) 
{
#if defined(__arm__) || defined(__aarch64__)
  free(Array);
#else
  _mm_free(Array);
#endif
}

/* General function for allocating memory for array of any type */
static void *AllocateArray(int Dim, int ElementSize, ...) 
{
  void     ***A;   /* stores a pointer to the start of the allocated block for */
                   /* each dimension  */
  void     *AA;    /* contains the start of the allocated nD-array */
  va_list  ap;     /* argument pointer in the variable length list */
  int      i;      /* */
  int      j;      /* */
  int      n;      /* number of entries that are to allocated for a block  */
  int      *Size;  /* Size (in number of entries) for each dimension       */

  /* Retrieve sizes for the different dimensions from the variable arg. list. */
  Size = MemoryAllocate(Dim, sizeof(*Size));
  va_start(ap, ElementSize);
  for (i = 0; i < Dim; i++)
  {
    Size[i] = va_arg(ap, int);
  }
  va_end(ap);
  A = MemoryAllocate(Dim, sizeof(**A));
  /* Allocate for each dimension a contiguous block of memory. */
  for (n = 1, i = 0; i < Dim - 1; i++) 
  {
    n *= Size[i];
    A[i] = MemoryAllocate(n, sizeof(void*));
  }
  n *= Size[i];
  A[i] = MemoryAllocate(n, ElementSize);
  /* Set pointers for each dimension to the correct entries of its lower dim.*/
  for (n = 1, i = 0; i < Dim - 1; i++) 
  {
    n *= Size[i];
    for (j = 0; j < n; j++)
    {
      /* A[i][j] = &A[i+1][j * Size[i+1]]; */
      A[i][j] = &((char *)A[i+1])[j * Size[i+1] * ElementSize];
    }
  }
  AA = A[0];
  MemoryFree(A);
  MemoryFree(Size);
  return AA;
}

/***************************************************************************/
/*                                                                         */
/* name     : AllocateSChar2D                                              */
/*                                                                         */
/* function : Function for allocating memory for a 2D signed char array    */
/*                                                                         */
/* pre      : Rows, Cols                                                   */
/*                                                                         */
/* post     : Pointer to the allocated memory                              */
/*                                                                         */
/***************************************************************************/
/*
static signed char **AllocateSChar2D(int Rows, int Cols)
{
  signed char    **Array;
  int            r;

  if ((Array = MemoryAllocate(Rows, sizeof(*Array))) == NULL)
  {
    fprintf(stderr,"ERROR: not enough memory available!\n\n");
  }

  if ((Array[0] = MemoryAllocate(Rows * Cols, sizeof(**Array))) == NULL)
  {
    fprintf(stderr,"ERROR: not enough memory available!\n\n");
  }

  for (r = 1; r < Rows; r++)
  {
    Array[r] = Array[r - 1] + Cols;
  }

  return Array;
}
*/

/* Function for allocating memory for a 2D unsigned char array */
/*
static unsigned char **AllocateUChar2D (int Rows, int Cols) 
{
  unsigned char  **Array;
  int            r;

  if ((Array = MemoryAllocate(Rows, sizeof(*Array))) == NULL) 
  {
    fprintf(stderr,"ERROR: not enough memory available!\n\n");
  }
  if ((Array[0] = MemoryAllocate(Rows * Cols, sizeof(**Array))) == NULL)
  {
    fprintf(stderr,"ERROR: not enough memory available!\n\n");
  }
  for (r = 1; r < Rows; r++) 
  {
    Array[r] = Array[r - 1] + Cols;
  }
  return Array;
}
*/

/* Release memory for all dynamic variables of the decoder.*/
static void FreeDecMemory (ebunch * D) 
{
  MemoryFree(D->FrameHdr.ICoefA[0]);
  MemoryFree(D->FrameHdr.ICoefA);

  MemoryFree(D->StrFilter.m[0]);
  MemoryFree(D->StrFilter.m);
  MemoryFree(D->StrFilter.Data[0]);
  MemoryFree(D->StrFilter.Data);
  MemoryFree(D->StrFilter.CPredOrder);
  MemoryFree(D->StrFilter.CPredCoef[0]);
  MemoryFree(D->StrFilter.CPredCoef);
  MemoryFree(D->StrFilter.Coded);
  MemoryFree(D->StrFilter.BestMethod);
  MemoryFree(D->StrFilter.DataLen);
  MemoryFree(D->StrPtable.m[0]);
  MemoryFree(D->StrPtable.m);
  MemoryFree(D->StrPtable.Data[0]);
  MemoryFree(D->StrPtable.Data);
  MemoryFree(D->StrPtable.CPredOrder);
  MemoryFree(D->StrPtable.CPredCoef[0]);
  MemoryFree(D->StrPtable.CPredCoef);
  MemoryFree(D->StrPtable.Coded);
  MemoryFree(D->StrPtable.BestMethod);
  MemoryFree(D->StrPtable.DataLen);
  MemoryFree(D->P_one[0]);
  MemoryFree(D->P_one);
  MemoryFree(D->AData);
}

/* Allocate memory for all dynamic variables of the decoder. */
static void AllocateDecMemory (ebunch * D)
{
  D->FrameHdr.ICoefA = AllocateArray(2,sizeof(**D->FrameHdr.ICoefA),D->FrameHdr.MaxNrOfFilters, (1<<SIZE_CODEDPREDORDER));

  D->StrFilter.Coded = MemoryAllocate(D->FrameHdr.MaxNrOfFilters, sizeof(*D->StrFilter.Coded));
  D->StrFilter.BestMethod = MemoryAllocate(D->FrameHdr.MaxNrOfFilters, sizeof(*D->StrFilter.BestMethod));
  D->StrFilter.m = AllocateArray(2, sizeof(**D->StrFilter.m), D->FrameHdr.MaxNrOfFilters, NROFFRICEMETHODS);
  D->StrFilter.Data = AllocateArray(2, sizeof(**D->StrFilter.Data), D->FrameHdr.MaxNrOfFilters, (1<<SIZE_CODEDPREDORDER) * SIZE_PREDCOEF);
  D->StrFilter.DataLen = MemoryAllocate(D->FrameHdr.MaxNrOfFilters, sizeof(*D->StrFilter.DataLen));
  D->StrFilter.CPredOrder = MemoryAllocate(NROFFRICEMETHODS, sizeof(*D->StrFilter.CPredOrder));
  D->StrFilter.CPredCoef = AllocateArray(2, sizeof(**D->StrFilter.CPredCoef), NROFFRICEMETHODS, MAXCPREDORDER);
  D->StrPtable.Coded = MemoryAllocate(D->FrameHdr.MaxNrOfPtables, sizeof(*D->StrPtable.Coded));
  D->StrPtable.BestMethod = MemoryAllocate(D->FrameHdr.MaxNrOfPtables, sizeof(*D->StrPtable.BestMethod));
  D->StrPtable.m  = AllocateArray(2, sizeof(**D->StrPtable.m), D->FrameHdr.MaxNrOfPtables, NROFPRICEMETHODS);
  D->StrPtable.Data = AllocateArray(2, sizeof(**D->StrPtable.Data), D->FrameHdr.MaxNrOfPtables, AC_BITS * AC_HISMAX);
  D->StrPtable.DataLen = MemoryAllocate(D->FrameHdr.MaxNrOfPtables, sizeof(*D->StrPtable.DataLen));
  D->StrPtable.CPredOrder = MemoryAllocate(NROFPRICEMETHODS, sizeof(*D->StrPtable.CPredOrder));
  D->StrPtable.CPredCoef = AllocateArray(2, sizeof(**D->StrPtable.CPredCoef), NROFPRICEMETHODS, MAXCPREDORDER);
  D->P_one = AllocateArray(2, sizeof(**D->P_one), D->FrameHdr.MaxNrOfPtables, AC_HISMAX);
  D->AData = MemoryAllocate(D->FrameHdr.BitStreamLen,  sizeof(*D->AData));
}

/***************************************************************************/
/*                                                                         */
/* name     : DST_InitDecoder                                              */
/*                                                                         */
/* function : Complete initialisation of the DST decoder.                  */
/*                                                                         */
/* pre      : argc, argv[][]                                               */
/*                                                                         */
/* post     : D->CodOpt: .Format, .Mask, .InStreamName, .JobName,          */
/*                       .OutFileName, .DebugName, .Debug, .LengthName,    */
/*                       .Length                                           */
/*            D->StrFilter.TableType, D->StrPtable.TableType               */
/*                                                                         */
/*            Memory allocated for:                                        */
/*              D->FirPtrs    : .Pnt,                                      */
/*              D->FrameHdr   : .PredOrder, .ICoefA,                       */
/*                              .FSeg.NrOfSegments, .FSeg.SegmentLen,      */
/*                              .FSeg.Table4Segment, .Filter4Bit,          */
/*                              .PSeg.NrOfSegments, .PSeg.SegmentLen,      */
/*                              .PSeg.Table4Segment, .Ptable4Bit,          */
/*              D->DsdFrame,                                               */
/*              D->PredicVal, D->P_one, D->AData                           */
/*                                                                         */
/***************************************************************************/

int DST_InitDecoder(ebunch * D, int NrOfChannels, int SampleRate) 
{
  int  retval = 0;

  memset(D, 0, sizeof(ebunch));

  D->FrameHdr.NrOfChannels   = NrOfChannels;
  /*  64FS =>  4704 */
  /* 128FS =>  9408 */
  /* 256FS => 18816 */
  D->FrameHdr.MaxFrameLen    = (588 * SampleRate / 8); 
  D->FrameHdr.ByteStreamLen  = D->FrameHdr.MaxFrameLen   * D->FrameHdr.NrOfChannels;
  D->FrameHdr.BitStreamLen   = D->FrameHdr.ByteStreamLen * RESOL;
  D->FrameHdr.NrOfBitsPerCh  = D->FrameHdr.MaxFrameLen   * RESOL;
  D->FrameHdr.MaxNrOfFilters = 2 * D->FrameHdr.NrOfChannels;
  D->FrameHdr.MaxNrOfPtables = 2 * D->FrameHdr.NrOfChannels;

  D->FrameHdr.FrameNr = 0;
  D->StrFilter.TableType = FILTER;
  D->StrPtable.TableType = PTABLE;

  if (retval==0) 
  {
    AllocateDecMemory(D);
  }

  if (retval==0) 
  {
    retval = CCP_CalcInit(&D->StrFilter);
  }
  if (retval==0) 
  {
    retval = CCP_CalcInit(&D->StrPtable);
  }

  D->SSE2 = 0;
#if !defined(NO_SSE2) && (defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__))
  {
    int CPUInfo[4];
#if defined(__i386__) || defined(__x86_64__)
#define cpuid(type, a, b, c, d) \
    __asm__ ("cpuid":\
    "=a" (a), "=b" (b), "=c" (c), "=d" (d) : "a" (type));

    cpuid(1, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
#else
    __cpuid(CPUInfo, 1);
#endif

    D->SSE2 = (CPUInfo[3] & (1L << 26)) ? 1 : 0;
  }
#endif

  return(retval);
}

/***************************************************************************/
/*                                                                         */
/* name     : DST_CloseDecoder                                             */
/*                                                                         */
/* function : Complete termination of the DST decoder.                     */
/*                                                                         */
/* pre      : Complete D-structure                                         */
/*                                                                         */
/* post     : None                                                         */
/*                                                                         */
/***************************************************************************/

int DST_CloseDecoder(ebunch * D)
{
  int retval = 0;
  /* Free the memory that was used for the arrays */
  FreeDecMemory(D);

  return(retval);
}

