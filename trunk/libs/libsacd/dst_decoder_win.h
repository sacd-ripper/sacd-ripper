/**
 * SACD Ripper - http://code.google.com/p/sacd-ripper/
 *
 * Copyright (c) 2010-2011 by respective authors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _WIN32
#error you need a compiler with a windows target to use this
#endif

#ifndef __DST_DECODER_H__
#define __DST_DECODER_H__

#include <initguid.h>
#include <stdint.h>

// Our DecoderComp object's GUID
// {ca408ff7-b6f1-11d5-88c8-005004e8beb0}
DEFINE_GUID(CLSID_DecoderComp, 0xca408ff7, 0xb6f1, 0x11d5, 0x88, 0xc8, 0x0, 0x50, 0x04, 0xe8, 0xbe, 0xb0);

// Our dst_decoder_t VTable's GUID
// {ca408ff6-b6f1-11d5-88c8-005004e8beb0}
DEFINE_GUID(IID_dst_decoder_t, 0xca408ff6, 0xb6f1, 0x11d5, 0x88, 0xc8, 0x0, 0x50, 0x04, 0xe8, 0xbe, 0xb0);

typedef struct dst_frame_strategy_t 
{
    int FilterMapping[6];
    int FilterPredOrder[6];
    int PredictionMapping[6];
    int PredictionTableLength[6];
} 
dst_frame_strategy_t;

// Defines dst_decoder_t's functions (in a way that works both for our COM DLL
// and any app that #include's this .H file
#undef  INTERFACE
#define INTERFACE   dst_decoder_t
DECLARE_INTERFACE_ (INTERFACE, IUnknown)
{
    STDMETHOD  (QueryInterface)		(THIS_ REFIID, void **) PURE;
    STDMETHOD_ (ULONG, AddRef)		(THIS) PURE;
    STDMETHOD_ (ULONG, Release)		(THIS) PURE;

    HRESULT ( __stdcall *Init )( 
        dst_decoder_t * This,
        /* [in] */ int channel_count); 

    HRESULT ( __stdcall *Close )( 
        dst_decoder_t * This); 

    HRESULT ( __stdcall *Decode )( 
        dst_decoder_t * This,
        /* [in] */ uint8_t* dst_frame,
        /* [out] */ uint8_t* dsd_frame,
        /* [in] */ uint64_t frame_count,
        /* [out] */ uint32_t* frame_size); 

    HRESULT ( __stdcall *GetBufferSize )( 
        dst_decoder_t * This,
        /* [out] */ int32_t* buffer_size); 

    HRESULT ( __stdcall *GetStrategy )( 
        dst_decoder_t * This,
        /* [out] */ dst_frame_strategy_t* frame_strategy); 

};

int dst_decoder_create(dst_decoder_t **dst_decoder);
int dst_decoder_destroy(dst_decoder_t *dst_decoder);
int dst_decoder_init(dst_decoder_t *dst_decoder, int channel_count);
int dst_decoder_decode(dst_decoder_t *dst_decoder, uint8_t *source_data, size_t source_size, uint8_t *dest_data, size_t *dsd_size);

#endif // __DST_DECODER_H__
