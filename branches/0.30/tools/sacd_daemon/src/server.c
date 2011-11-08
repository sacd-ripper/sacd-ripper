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

#include <stdio.h>
#include <string.h>
#include <ppu-types.h>

#include <net/net.h>
#include <net/netctl.h>
#include <arpa/inet.h>

#include <sys/thread.h>
#include <sys/systime.h>

#include <pb.h>
#include <pb_encode.h>
#include <pb_decode.h>
#include <sacd_pb_stream.h>
#include <utils.h>

#include <sacd_reader.h>
#include <sacd_ripper.pb.h>
#include <scarletbook_read.h>

#include "server.h"
#include "exit_handler.h"

int closesocket(int socket);

static int client_connected = 0;

int is_client_connected(void)
{
    return client_connected;
}

static void client_thread(void *userdata)
{
	p_socket socket = (p_socket) userdata;
    ServerRequest request;
    ServerResponse response;
    uint8_t zero = 0;
    pb_istream_t input = pb_istream_from_socket(socket);
    pb_ostream_t output = pb_ostream_from_socket(socket);
    sacd_reader_t   *sacd_reader = 0;
    scarletbook_handle_t *handle = 0;
    int non_encrypted_disc = 0;
    int checked_for_non_encrypted_disc = 0;

	client_connected = 1;
	
	response.data.bytes = (uint8_t *) malloc(1024 * 1024 * 2);

    for (;;)
    {
        if (!pb_decode(&input, ServerRequest_fields, &request))
        {
            break;
        }
        
        response.has_data = false;
        response.data.size = 0;
        response.result = -1;
    
        switch(request.type)
        {
        case ServerRequest_Type_DISC_OPEN:
            response.type = ServerResponse_Type_DISC_OPENED;
            sacd_reader = sacd_open("/dev_bdvd");
            if (sacd_reader) 
            {
                handle = scarletbook_open(sacd_reader, 0);
                checked_for_non_encrypted_disc = 0;
                non_encrypted_disc = 0;                
            }
            response.result = (handle && sacd_authenticate(sacd_reader));
            break;
        case ServerRequest_Type_DISC_CLOSE:
            response.type = ServerResponse_Type_DISC_CLOSED;
            if (handle && sacd_reader)
            {
                scarletbook_close(handle);
                sacd_close(sacd_reader);
                response.result = 1;
            }
            break;
        case ServerRequest_Type_DISC_READ:
            {
                response.type = ServerResponse_Type_DISC_READ;
                if (handle && sacd_reader)
                {
                    response.result = sacd_read_block_raw(sacd_reader, request.sector_offset, request.sector_count, response.data.bytes);
                    response.has_data = response.result > 0;
                    response.data.size = max(response.result * SACD_LSN_SIZE, response.result);
      
#if 0                    
                    // check what block ranges are encrypted..
                    if (request.sector_offset < encrypted_start_1)
                    {
                        block_size = min(encrypted_start_1 - request.sector_offset, MAX_PROCESSING_BLOCK_SIZE);
                        encrypted = 0;
                    }
                    else if (request.sector_offset >= encrypted_start_1 && request.sector_offset <= encrypted_end_1)
                    {
                        block_size = min(encrypted_end_1 + 1 - request.sector_offset, MAX_PROCESSING_BLOCK_SIZE);
                        encrypted = 1;
                    }
                    else if (request.sector_offset > encrypted_end_1 && request.sector_offset < encrypted_start_2)
                    {
                        block_size = min(encrypted_start_2 - request.sector_offset, MAX_PROCESSING_BLOCK_SIZE);
                        encrypted = 0;
                    }
                    else if (request.sector_offset >= encrypted_start_2 && request.sector_offset <= encrypted_end_2)
                    {
                        block_size = min(encrypted_end_2 + 1 - request.sector_offset, MAX_PROCESSING_BLOCK_SIZE);
                        encrypted = 1;
                    }
                    else
                    {
                        block_size = MAX_PROCESSING_BLOCK_SIZE;
                        encrypted = 0;
                    }
                    
                    // the ATAPI call which returns the flag if the disc is encrypted or not is unknown at this point. 
                    // user reports tell me that the only non-encrypted discs out there are DSD 3 14/16 discs. 
                    // this is a quick hack/fix for these discs.
                    if (encrypted && checked_for_non_encrypted_disc == 0)
                    {
                        switch (handle->area[0].area_toc->frame_format)
                        {
                        case FRAME_FORMAT_DSD_3_IN_14:
                        case FRAME_FORMAT_DSD_3_IN_16:
                            non_encrypted_disc = *(uint64_t *)(response.data.bytes + 16) == 0;
                            break;
                        }

                        checked_for_non_encrypted_disc = 1;
                    }

                    // encrypted blocks need to be decrypted first
                    if (encrypted && non_encrypted_disc == 0)
                    {
                        sacd_decrypt(sacd_reader, response.data.bytes, block_size);
                    }
#endif                    
                }
            }
            break;
        case ServerRequest_Type_DISC_SIZE:
            response.type = ServerResponse_Type_DISC_SIZE;
            if (sacd_reader)
            {
                response.result = sacd_get_total_sectors(sacd_reader);
            }
            break;
        }
    
        if (!pb_encode(&output, ServerResponse_fields, &response))
        {
            break;
        }
    
        /* We signal the end of a request with a 0 tag. */
        pb_write(&output, &zero, 1);
    
        if (request.type == ServerRequest_Type_DISC_CLOSE)
        {
            break;
        }
    }
    
    free(response.data.bytes);
	
	closesocket((int) *socket);
	client_connected = 0;
	
	sysThreadExit(0);
} 

void listener_thread(void *unused)
{
	sys_ppu_thread_t id; 
	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	
	sa.sin_family = AF_INET;
	sa.sin_port = htons(2002);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	
	int list_s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	// only 1 socket connection is allowed at once
	if(bind(list_s, (struct sockaddr *)&sa, sizeof(sa)) == -1 || listen(list_s, 1) == -1)
	{
	    sysThreadExit(0);
	}
	
	int conn_s;
	
	while (!user_requested_exit())
	{
		if((conn_s = accept(list_s, NULL, NULL)) > 0)
		{
			sysThreadCreate(&id, client_thread, (void *)&conn_s, 1337, 0x2000, 0, "client");
			sysThreadYield();
		}
	}
	
	closesocket(list_s);
	
	sysThreadExit(0);
}
