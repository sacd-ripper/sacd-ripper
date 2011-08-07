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

#include <windows.h>
#include <objbase.h>
#include <stdio.h>

#include <logging.h>

#include "dst_decoder_win.h"

int dst_decoder_create(dst_decoder_t **dst_decoder)
{
    IClassFactory	*classFactory;
    HRESULT			hr;

    if (!CoInitialize(0))
    {
        if ((hr = CoGetClassObject(&CLSID_DecoderComp, CLSCTX_INPROC_SERVER, 0, &IID_IClassFactory, &classFactory)))
        {
            LOG(lm_main, LOG_ERROR, ("Can't get IClassFactory"));
            return -1;
        }
        else
        {
            if ((hr = classFactory->lpVtbl->CreateInstance(classFactory, 0, &IID_dst_decoder_t, dst_decoder)))
            {
                classFactory->lpVtbl->Release(classFactory);
                
                LOG(lm_main, LOG_ERROR, ("Can't create dst_decoder_t object"));
                return -1;
            }
            else
            {
                classFactory->lpVtbl->Release(classFactory);
            }
        }

        return 0;
    }

    return -1;
}

int dst_decoder_destroy(dst_decoder_t *dst_decoder)
{
    HRESULT ret;

    if (dst_decoder)
    {
        ret = dst_decoder->lpVtbl->Close(dst_decoder);
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("could not close the dst_decoder %x", ret));

            return -1;
        }

        // Release the dst_decoder_t now that we're done with it
        ret = dst_decoder->lpVtbl->Release(dst_decoder);
        if (ret != 0)
        {
            LOG(lm_main, LOG_ERROR, ("could not release the dst_decoder %x", ret));

            return -1;
        }
    }

    // we are done with OLE, free it
    CoUninitialize();

    return 0;
}   

int dst_decoder_init(dst_decoder_t *dst_decoder, int channel_count)
{
    if (dst_decoder)
    {
        HRESULT hr = dst_decoder->lpVtbl->Init(dst_decoder, channel_count);
        if (hr != 0)
        {
            return -1;
        }
        return 0;
    }

    return -1;
}

int dst_decoder_decode(dst_decoder_t *dst_decoder, uint8_t *source_data, size_t source_size, uint8_t *dest_data, size_t *dsd_size)
{
    HRESULT hr;
    uint32_t frame_size = source_size;
    int32_t dest_size;

    if (dst_decoder)
    {
        if (source_size > 0)
        {
            hr = dst_decoder->lpVtbl->Decode(dst_decoder, source_data, dest_data, 0, &frame_size);
            if (hr != 0)
            {
                return -1;
            }

            hr = dst_decoder->lpVtbl->GetBufferSize(dst_decoder, &dest_size);
            if (hr != 0)
            {
                return -1;
            }

            *dsd_size = dest_size;
        }
        else
        {
            *dsd_size = 0;
        }
        return 0;
    }

    return -1;
}
