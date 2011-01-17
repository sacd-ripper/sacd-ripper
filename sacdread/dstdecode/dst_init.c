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
#include "dst_init.h"
#include "ccp_calc.h"
#include "conststr.h"
#include "types.h"

/*============================================================================*/
/*       STATIC FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* General function for allocating memory for array of any type */
static void *memory_allocate (int NrOfElements, int SizeOfElement) 
{
  void *Array;

  if ((Array = malloc(NrOfElements * SizeOfElement)) == NULL) 
  {
    fprintf(stderr,"ERROR: not enough memory available!\n\n");
    exit(1);
  }
  return Array;
}

/* General function for allocating memory for array of any type */
static void *allocate_array (int Dim, int ElementSize, ...) 
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
  Size = memory_allocate(Dim, sizeof(*Size));
  va_start(ap, ElementSize);
  for (i = 0; i < Dim; i++) 
  {
    Size[i] = va_arg(ap, int);
  }
  va_end(ap);  
  A = memory_allocate(Dim, sizeof(**A));
  /* Allocate for each dimension a contiguous block of memory. */
  for (n = 1, i = 0; i < Dim - 1; i++) 
  {
    n *= Size[i];
    A[i] = memory_allocate(n, sizeof(void*));
  }
  n *= Size[i];
  A[i] = memory_allocate(n, ElementSize);  
  /* Set pointers for each dimension to the correct entries of its lower dim.*/
  for (n = 1, i = 0; i < Dim - 1; i++) 
  {
    n *= Size[i];
    for (j = 0; j < n; j++) 
    {
      A[i][j] = &A[i+1][j * Size[i+1]];
    }
  }  
  AA = A[0];  
  free(A);
  free(Size);
  return AA;
}

/***************************************************************************/
/*                                                                         */
/* name     : allocate_2d_int8_array                                       */
/*                                                                         */
/* function : Function for allocating memory for a 2D signed char array    */
/*                                                                         */
/* pre      : Rows, Cols                                                   */
/*                                                                         */
/* post     : Pointer to the allocated memory                              */
/*                                                                         */
/***************************************************************************/

static signed char **allocate_2d_int8_array(int Rows, int Cols)
{
  signed char    **Array;
  int            r;

  if ((Array = malloc(Rows * sizeof(*Array))) == NULL)
  {
    fprintf(stderr,"ERROR: not enough memory available!\n\n");
    exit(1);
  }

  if ((Array[0] = malloc(Rows * Cols * sizeof(**Array))) == NULL)
  {
    fprintf(stderr,"ERROR: not enough memory available!\n\n");
    exit(1);
  }
  
  for (r = 1; r < Rows; r++)
  {
    Array[r] = Array[r - 1] + Cols;
  }

  return Array;
}


/* Function for allocating memory for a 2D unsigned char array */
static unsigned char **allocate_2d_uint8_array (int Rows, int Cols) 
{
  unsigned char  **Array;
  int            r;

  if ((Array = malloc(Rows * sizeof(*Array))) == NULL) 
  {
    fprintf(stderr,"ERROR: not enough memory available!\n\n");
    exit(1);
  }
  if ((Array[0] = malloc(Rows * Cols * sizeof(**Array))) == NULL) {
    fprintf(stderr,"ERROR: not enough memory available!\n\n");
    exit(1);
  }
  for (r = 1; r < Rows; r++) 
  {
    Array[r] = Array[r - 1] + Cols;
  }
  return Array;
}

/* Release memory for all dynamic variables of the decoder.*/
void free_decoder_memory (ebunch_t * D) 
{
  free(D->FirPtrs.Pnt);
  free(D->FirPtrs.Status[0]);
  free(D->FirPtrs.Status);
  
  free(D->FrameHdr.NrOfHalfBits);
  free(D->FrameHdr.HalfProb);
  free(D->FrameHdr.PredOrder);
  free(D->FrameHdr.PtableLen);
  free(D->FrameHdr.ICoefA[0]);
  free(D->FrameHdr.ICoefA);
  free(D->FrameHdr.FSeg.SegmentLen[0]);
  free(D->FrameHdr.FSeg.SegmentLen);
  free(D->FrameHdr.FSeg.NrOfSegments);
  free(D->FrameHdr.FSeg.Table4Segment[0]);
  free(D->FrameHdr.FSeg.Table4Segment);
  free(D->FrameHdr.Filter4Bit[0]);
  free(D->FrameHdr.Filter4Bit);
  free(D->FrameHdr.PSeg.SegmentLen[0]);
  free(D->FrameHdr.PSeg.SegmentLen);
  free(D->FrameHdr.PSeg.NrOfSegments);
  free(D->FrameHdr.PSeg.Table4Segment[0]);
  free(D->FrameHdr.PSeg.Table4Segment);
  free(D->FrameHdr.Ptable4Bit[0]);
  free(D->FrameHdr.Ptable4Bit);
  free(D->StrFilter.m[0]);
  free(D->StrFilter.m);
  free(D->StrFilter.L1[0]);
  free(D->StrFilter.L1);
  free(D->StrFilter.Data[0]);
  free(D->StrFilter.Data);
  free(D->StrFilter.C[0][0]);
  free(D->StrFilter.C[0]);
  free(D->StrFilter.C);
  free(D->StrFilter.CPredOrder);
  free(D->StrFilter.CPredCoef[0]);
  free(D->StrFilter.CPredCoef);
  free(D->StrFilter.Coded);
  free(D->StrFilter.BestMethod);
  free(D->StrFilter.DataLen);
  free(D->StrPtable.m[0]);
  free(D->StrPtable.m);
  free(D->StrPtable.L1[0]);
  free(D->StrPtable.L1);
  free(D->StrPtable.Data[0]);
  free(D->StrPtable.Data);
  free(D->StrPtable.C[0][0]);
  free(D->StrPtable.C[0]);
  free(D->StrPtable.C);
  free(D->StrPtable.CPredOrder);
  free(D->StrPtable.CPredCoef[0]);
  free(D->StrPtable.CPredCoef);
  free(D->StrPtable.Coded);
  free(D->StrPtable.BestMethod);
  free(D->StrPtable.DataLen);
  free(D->BitStream11[0]);
  free(D->BitStream11);
  free(D->BitResidual[0]);
  free(D->BitResidual);
  free(D->PredicVal[0]);
  free(D->PredicVal);
  free(D->P_one[0]);
  free(D->P_one);
  free(D->AData);
}

/* Allocate memory for all dynamic variables of the decoder. */
void allocate_decoder_memory (ebunch_t * D)
{
  D->FirPtrs.Pnt    = memory_allocate(D->FrameHdr.NrOfChannels, sizeof(*D->FirPtrs.Pnt));
  D->FirPtrs.Status = allocate_array(2, sizeof(**D->FirPtrs.Status), 
     D->FrameHdr.NrOfChannels, (1 << SIZE_CODEDPREDORDER));
  
  D->FrameHdr.HalfProb = 
    memory_allocate(D->FrameHdr.NrOfChannels, sizeof(*D->FrameHdr.HalfProb));
  D->FrameHdr.NrOfHalfBits = 
    memory_allocate(D->FrameHdr.NrOfChannels, sizeof(*D->FrameHdr.NrOfHalfBits));  
  D->FrameHdr.PredOrder = 
    memory_allocate(D->FrameHdr.MaxNrOfFilters, sizeof(*D->FrameHdr.PredOrder));  
  D->FrameHdr.PtableLen = 
    memory_allocate(D->FrameHdr.MaxNrOfPtables, sizeof(*D->FrameHdr.PtableLen));  
  D->FrameHdr.ICoefA = 
    allocate_array (
    3,sizeof(**D->FrameHdr.ICoefA),D->FrameHdr.MaxNrOfFilters,
    (1<<SIZE_CODEDPREDORDER),2
      );
  D->FrameHdr.FSeg.NrOfSegments = 
    memory_allocate (
    D->FrameHdr.NrOfChannels, sizeof(*D->FrameHdr.FSeg.NrOfSegments)
      );
  D->FrameHdr.FSeg.SegmentLen = 
    allocate_array (
    2,sizeof(**D->FrameHdr.FSeg.SegmentLen), D->FrameHdr.NrOfChannels, 
    MAXNROF_FSEGS
      );
  D->FrameHdr.FSeg.Table4Segment = 
    allocate_array (
    2,sizeof(**D->FrameHdr.FSeg.Table4Segment),
    D->FrameHdr.NrOfChannels, MAXNROF_FSEGS
    );
  D->FrameHdr.Filter4Bit = 
    allocate_array (
    2, sizeof(**D->FrameHdr.Filter4Bit),
    D->FrameHdr.NrOfChannels, D->FrameHdr.NrOfBitsPerCh
      );
  D->FrameHdr.PSeg.NrOfSegments = 
    memory_allocate (
    D->FrameHdr.NrOfChannels, sizeof(*D->FrameHdr.PSeg.NrOfSegments)
    );
  D->FrameHdr.PSeg.SegmentLen = 
    allocate_array (
    2,sizeof(**D->FrameHdr.PSeg.SegmentLen), D->FrameHdr.NrOfChannels, 
    MAXNROF_PSEGS
      );
  D->FrameHdr.PSeg.Table4Segment = 
    allocate_array (
    2,sizeof(**D->FrameHdr.PSeg.Table4Segment), 
    D->FrameHdr.NrOfChannels, MAXNROF_PSEGS
    );
  D->FrameHdr.Ptable4Bit = 
    allocate_array (
    2, sizeof(**D->FrameHdr.Ptable4Bit),
    D->FrameHdr.NrOfChannels, D->FrameHdr.NrOfBitsPerCh
    );
  D->StrFilter.Coded = 
    memory_allocate (
    D->FrameHdr.MaxNrOfFilters, sizeof(*D->StrFilter.Coded)
    );
  D->StrFilter.BestMethod = 
    memory_allocate (
    D->FrameHdr.MaxNrOfFilters, sizeof(*D->StrFilter.BestMethod)
    );
  D->StrFilter.m = 
    allocate_array (
    2, sizeof(**D->StrFilter.m),
    D->FrameHdr.MaxNrOfFilters, NROFFRICEMETHODS
    );
  D->StrFilter.L1 = 
    allocate_array (
    2, sizeof(**D->StrFilter.L1),
    D->FrameHdr.MaxNrOfFilters, NROFFRICEMETHODS
    );
  D->StrFilter.Data = 
    allocate_array (
    2, sizeof(**D->StrFilter.Data),
    D->FrameHdr.MaxNrOfFilters, (1<<SIZE_CODEDPREDORDER) * SIZE_PREDCOEF
    );
  D->StrFilter.DataLen = 
    memory_allocate (
    D->FrameHdr.MaxNrOfFilters, sizeof(*D->StrFilter.DataLen)
    );
  D->StrFilter.C = 
    allocate_array (
    3, sizeof(***D->StrFilter.C),
    D->FrameHdr.MaxNrOfFilters, NROFFRICEMETHODS, 1<<SIZE_CODEDPREDORDER
    );
  D->StrFilter.CPredOrder = 
    memory_allocate (
    NROFFRICEMETHODS, sizeof(*D->StrFilter.CPredOrder)
    );
  D->StrFilter.CPredCoef = 
    allocate_array (
    2, sizeof(**D->StrFilter.CPredCoef),
    NROFFRICEMETHODS, MAXCPREDORDER
    );
  D->StrPtable.Coded = 
    memory_allocate (
    D->FrameHdr.MaxNrOfPtables, sizeof(*D->StrPtable.Coded)
    );
  D->StrPtable.BestMethod = 
    memory_allocate (
    D->FrameHdr.MaxNrOfPtables, sizeof(*D->StrPtable.BestMethod)
    );
  D->StrPtable.m  = 
    allocate_array (
    2, sizeof(**D->StrPtable.m),
    D->FrameHdr.MaxNrOfPtables, NROFPRICEMETHODS
    );
  D->StrPtable.L1 = 
    allocate_array (
    2, sizeof(**D->StrPtable.L1),
    D->FrameHdr.MaxNrOfPtables, NROFPRICEMETHODS
    );
  D->StrPtable.Data = 
    allocate_array (
    2, sizeof(**D->StrPtable.Data),
    D->FrameHdr.MaxNrOfPtables, AC_BITS * AC_HISMAX
    );
  D->StrPtable.DataLen = 
    memory_allocate (
    D->FrameHdr.MaxNrOfPtables, sizeof(*D->StrPtable.DataLen)
    );
  D->StrPtable.C = 
    allocate_array (
    3, sizeof(***D->StrPtable.C),
    D->FrameHdr.MaxNrOfPtables, NROFPRICEMETHODS, AC_HISMAX
    );
  D->StrPtable.CPredOrder = 
    memory_allocate (
    NROFPRICEMETHODS, sizeof(*D->StrPtable.CPredOrder)
    );
  D->StrPtable.CPredCoef = 
    allocate_array (
    2, sizeof(**D->StrPtable.CPredCoef),
    NROFPRICEMETHODS, MAXCPREDORDER
    );
  D->BitStream11 = allocate_2d_int8_array(D->FrameHdr.NrOfChannels, D->FrameHdr.NrOfBitsPerCh);
  D->BitResidual = allocate_2d_uint8_array (D->FrameHdr.NrOfChannels, D->FrameHdr.NrOfBitsPerCh);
  D->PredicVal = allocate_array (2, sizeof(**D->PredicVal), D->FrameHdr.NrOfChannels, D->FrameHdr.NrOfBitsPerCh );
  D->P_one = allocate_array (2, sizeof(**D->P_one)    , D->FrameHdr.MaxNrOfPtables, AC_HISMAX );
  D->AData =  memory_allocate ( D->FrameHdr.BitStreamLen,  sizeof(*D->AData) );
}

