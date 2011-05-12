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

Copyright  2004.

Source file: DSTData.c (DSTData object)

Required libraries: <none>

Authors:
RT:  Richard Theelen, PDSL-labs Eindhoven <r.h.m.theelen@philips.com>

Changes:
08-Mar-2004 RT  Initial version

************************************************************************/


/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <stdlib.h>
#include <malloc.h>
#include "types.h"
#include "dst_data.h"

/***********************************************************************
 * Static Members
 ***********************************************************************/

static  uint8_t*   m_pDSTdata    = NULL;
static  int32_t    m_TotalBytes  = 0;
static  int32_t    m_ByteCounter = 0;
static  int32_t    m_BitCounter  = 0;
static  int        m_BitPosition = 0;
static  long       m_mask[32];
static  uint8_t    m_DataByte    = 0;

/***********************************************************************
 * Forward declaration function prototype
 ***********************************************************************/

int getbits(long *outword, int out_bitptr);

/***********************************************************************
 * reset_reading_index
 ***********************************************************************/

int reset_reading_index(void)
{
  int hr = 0;

  m_BitCounter  = 0;
  m_BitPosition = -1;
  m_ByteCounter = 0;
  m_DataByte    = 0;

  return (hr);
}

/***********************************************************************
 * create_buffer
 ***********************************************************************/

int create_buffer(int32_t Size)
{
  int hr = 0;

  m_TotalBytes = Size;

  /* delete buffer if exist */
  if (m_pDSTdata != NULL)
  {
    free( m_pDSTdata );
    m_pDSTdata = NULL;
  }

  /* create new buffer for data */
  m_pDSTdata = (uint8_t*) malloc (Size);

  if (m_pDSTdata == NULL)
  {
    m_TotalBytes = 0;
    hr = -1;
  }

  reset_reading_index();

  return (hr);
}

/***********************************************************************
 * delete_buffer
 ***********************************************************************/

int delete_buffer(void)
{
  int hr = 0;

  m_TotalBytes = 0;

  if (m_pDSTdata != NULL)
  {
    hr = -1;
  }

  reset_reading_index();

  return (hr);
}

/***********************************************************************
 * fill_buffer
 ***********************************************************************/

int fill_buffer(uint8_t* pBuf, int32_t Size)
{
  int hr = 0;
  int32_t    cnt;

  m_pDSTdata = NULL;

  /* fill mask */
  for (cnt = 0; cnt < 32; cnt++)
  {
    m_mask[cnt] = 1 << cnt;
  }

  create_buffer(Size);

  for (cnt = 0; cnt < Size; cnt++)
  {
    m_pDSTdata[cnt] = pBuf[cnt];
  }

  reset_reading_index();

  return (hr);
}

/***************************************************************************/
/*                                                                         */
/* name     : fio_bit_get_uint8                                            */
/*                                                                         */
/* function : Read a character as an unsigned number from file with a      */
/*            given number of bits.                                        */
/*                                                                         */
/* pre      : Len, x, output file must be open by having used getbits_init */
/*                                                                         */
/* post     : The second variable in function call is filled with the      */
/*            unsigned character read                                      */
/*                                                                         */
/* uses     : stdio.h, stdlib.h                                            */
/*                                                                         */
/***************************************************************************/

int fio_bit_get_uint8(int Len, unsigned char *x)
{
  int   return_value;
  long  tmp;

  return_value = -1;
  if (Len > 0)
  {
    return_value = getbits(&tmp, Len);
    *x = (unsigned char)tmp;
  }
  else if (Len == 0)
  {
    *x = 0;
    return_value = 0;
  }
  else
  {
    printf("\nERROR: a negative number of bits allocated\n");
    exit(1);
  }
  return return_value;
}


/***************************************************************************/
/*                                                                         */
/* name     : fio_bit_get_uint                                             */
/*                                                                         */
/* function : Read an integer as an unsigned number from file with a       */
/*            given number of bits.                                        */
/*                                                                         */
/* pre      : Len, x, output file must be open by having used getbits_init */
/*                                                                         */
/* post     : The second variable in function call is filled with the      */
/*            unsigned integer read                                        */
/*                                                                         */
/* uses     : stdio.h, stdlib.h                                            */
/*                                                                         */
/***************************************************************************/

int fio_bit_get_uint(int Len, int *x)
{
  int   return_value;
  long  tmp;

  return_value = -1;
  if (Len > 0)
  {
    return_value = getbits(&tmp, Len);
    *x = (int)tmp;
  }
  else if (Len == 0)
  {
    *x = 0;
    return_value = 0;
  }
  else
  {
    fprintf(stderr, "\nERROR: a negative number of bits allocated\n");
    exit(1);
  }
  return return_value;
}

/***************************************************************************/
/*                                                                         */
/* name     : fio_bit_get_int                                              */
/*                                                                         */
/* function : Read an integer as a signed number from file with a          */
/*            given number of bits.                                        */
/*                                                                         */
/* pre      : Len, x, output file must be open by having used getbits_init */
/*                                                                         */
/* post     : The second variable in function call is filled with the      */
/*            signed integer read                                          */
/*                                                                         */
/* uses     : stdio.h, stdlib.h                                            */
/*                                                                         */
/***************************************************************************/

int fio_bit_get_int(int Len, int *x)
{
  int   return_value;
  long  tmp;

  return_value = -1;
  if (Len > 0)
  {
    return_value = getbits(&tmp, Len);
    *x = (int)tmp;
    
    if (*x >= (1 << (Len - 1)))
    {
      *x -= (1 << Len);
    }
  }
  else if (Len == 0)
  {
    *x = 0;
    return_value = 0;
  }
  else
  {
    fprintf(stderr, "\nERROR: a negative number of bits allocated\n");
    exit(1);
  }
  return return_value;
}


/***************************************************************************/
/*                                                                         */
/* name     : getbits                                                      */
/*                                                                         */
/* function : Read bits from the bitstream and decrement the counter.      */
/*                                                                         */
/* pre      : out_bitptr                                                   */
/*                                                                         */
/* post     : m_BitCounter, outword, returns EOF on EOF or 0 otherwise.    */
/*                                                                         */
/* uses     : stdio.h                                                      */
/*                                                                         */
/***************************************************************************/

int getbits(long *outword, int out_bitptr)
{
  m_BitCounter += out_bitptr;
  *outword = 0;
  while(--out_bitptr >= 0)
  {
    if (m_BitPosition < 0)
    {
      m_DataByte = m_pDSTdata[m_ByteCounter++];
      if (m_ByteCounter > m_TotalBytes)
      {
        return (-1); /* EOF */
      }
      m_BitPosition = 7;
    }
    if ((m_DataByte & m_mask[m_BitPosition]) != 0)
    {
      *outword |= m_mask[out_bitptr];
    }
    m_BitPosition--;
  }

  return 0;
}

/***************************************************************************/
/*                                                                         */
/* name     : get_bitcount                                                 */
/*                                                                         */
/* function : Reset the bits-written counter.                              */
/*                                                                         */
/* pre      : None                                                         */
/*                                                                         */
/* post     : Returns the number of bits written after an init_bitcount.   */
/*                                                                         */
/* uses     : -                                                            */
/*                                                                         */
/***************************************************************************/

int get_in_bitcount(void)
{
  return m_BitCounter;
}
