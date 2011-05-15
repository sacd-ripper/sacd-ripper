/**
 * SACD Ripper - http://code.google.com/write_ptr/sacd-ripper/
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#if defined(__lv2ppu__)
#include <lv2/sysfs.h>	
#elif defined(WIN32)
#include <io.h>
#endif	

#include "sacd_reader.h"
#include "scarletbook_id3.h"
#include "endianess.h"
#include "dsdiff_writer.h"
#include "dsdiff.h"
#include "version.h"

#define DSDFIFF_BUFFER_SIZE 8192

static char *get_mtoc_title_text(scarletbook_handle_t *handle) {
	int i;
	master_toc_t *master_toc = handle->master_toc;

	for (i = 0; i < master_toc->text_channel_count; i++) {
		master_text_t *master_text = handle->master_text[i];

		if (master_text->album_title_position)
			return (char*) master_text + master_text->album_title_position;
		if (master_text->disc_title_position)
			return (char*) master_text + master_text->disc_title_position;
	}
	return 0;
}

dsdiff_handle_t	*dsdiff_create(scarletbook_handle_t *sb_handle, char *filename, int channel, int track, int dst_encoded) {
	dsdiff_handle_t *handle;
	form_dsd_chunk_t *form_dsd_chunk;
	property_chunk_t *property_chunk;
	uint8_t *write_ptr, *prop_ptr;
	ssize_t track_size;

	handle = (dsdiff_handle_t *) malloc(sizeof(dsdiff_handle_t));
	memset(handle, 0, sizeof(dsdiff_handle_t));

#if defined(__lv2ppu__)
	if (sysFsOpen(filename, SYS_O_RDWR | SYS_O_CREAT | SYS_O_TRUNC, &handle->fd, 0, 0) != 0) {
		fprintf(stderr, "error creating %s", filename);
		return 0;
	}
#else
	handle->fd = open(filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0666);
	if (handle->fd == -1) {
		fprintf(stderr, "error creating %s", filename);
		return 0;
	}
#endif
	handle->header = (uint8_t *) malloc(DSDFIFF_BUFFER_SIZE);
	memset(handle->header, 0, DSDFIFF_BUFFER_SIZE);

	handle->footer = (uint8_t *) malloc(DSDFIFF_BUFFER_SIZE);
	memset(handle->footer, 0, DSDFIFF_BUFFER_SIZE);

	write_ptr = handle->header;

	// The Form DSD Chunk is required. It may appear only once in the file.
	form_dsd_chunk = (form_dsd_chunk_t *) handle->header;
	form_dsd_chunk->chunk_id = FRM8_MARKER;
	form_dsd_chunk->form_type = DSD_MARKER;
	write_ptr = (uint8_t *) handle->header + FORM_DSD_CHUNK_SIZE;

	// The Format Version Chunk is required and must be the first chunk in the Form DSD
	// Chunk. It may appear only once in the Form DSD Chunk.
	{
		format_version_chunk_t *format_version_chunk = (format_version_chunk_t *) write_ptr;
		format_version_chunk->chunk_id = FVER_MARKER;
		format_version_chunk->chunk_data_size = CALC_CHUNK_SIZE(FORMAT_VERSION_CHUNK_SIZE - CHUNK_HEADER_SIZE);
		format_version_chunk->version = hton32(DSDIFF_VERSION);
		write_ptr += FORMAT_VERSION_CHUNK_SIZE;
	}

	// The Property Chunk is required and must precede the Sound Data Chunk. It may appear
	// only once in the Form DSD Chunk.
	{
		prop_ptr = write_ptr;
		property_chunk = (property_chunk_t *) write_ptr;
		property_chunk->chunk_id = PROP_MARKER;
		property_chunk->property_type = SND_MARKER;
		write_ptr += PROPERTY_CHUNK_SIZE;
	}

	// The Sample Rate Chunk is required and may appear only once in the Property Chunk.
	{
		sample_rate_chunk_t *sample_rate_chunk = (sample_rate_chunk_t *) write_ptr;
		sample_rate_chunk->chunk_id = FS_MARKER;
		sample_rate_chunk->chunk_data_size = CALC_CHUNK_SIZE(SAMPLE_RATE_CHUNK_SIZE - CHUNK_HEADER_SIZE);
		sample_rate_chunk->sample_rate = hton32(SACD_SAMPLING_FREQUENCY);
		write_ptr += SAMPLE_RATE_CHUNK_SIZE;
	}

	// The Channels Chunk is required and may appear only once in the Property Chunk.
	{
		int i;
		uint8_t channel_count = sb_handle->channel_toc[channel]->channel_count;
		channels_chunk_t * channels_chunk = (channels_chunk_t *) write_ptr;
		channels_chunk->chunk_id = CHNL_MARKER;
		channels_chunk->chunk_data_size = CALC_CHUNK_SIZE(CHANNELS_CHUNK_SIZE - CHUNK_HEADER_SIZE + channel_count * sizeof(uint32_t));
		channels_chunk->channel_count = hton16(channel_count);

		switch(channel_count) {
		case 2:
			channels_chunk->channel_ids[0] = SLFT_MARKER;
			channels_chunk->channel_ids[1] = SRGT_MARKER;
			break;
		case 5:
			channels_chunk->channel_ids[0] = MLFT_MARKER;
			channels_chunk->channel_ids[1] = MRGT_MARKER;
			channels_chunk->channel_ids[2] = C_MARKER;
			channels_chunk->channel_ids[3] = LS_MARKER;
			channels_chunk->channel_ids[4] = RS_MARKER;
			break;
		case 6:
			channels_chunk->channel_ids[0] = MLFT_MARKER;
			channels_chunk->channel_ids[1] = MRGT_MARKER;
			channels_chunk->channel_ids[2] = C_MARKER;
			channels_chunk->channel_ids[3] = LFE_MARKER;
			channels_chunk->channel_ids[4] = LS_MARKER;
			channels_chunk->channel_ids[5] = RS_MARKER;
			break;
		default:
			for (i = 0; i < channel_count; i++) {
				sprintf((char*) &channels_chunk->channel_ids[i], "C%03i", i);
			}
			break;
		}

		write_ptr += CHANNELS_CHUNK_SIZE + sizeof(uint32_t) * channel_count;
	}

	// The Compression Type Chunk is required and may appear only once in the Property
	// Chunk.
	{
		compression_type_chunk_t *compression_type_chunk = (compression_type_chunk_t *) write_ptr;
		compression_type_chunk->chunk_id = CMPR_MARKER;
		compression_type_chunk->compression_type = DSD_MARKER;
		if (dst_encoded) {
			compression_type_chunk->count = 11;
			memcpy(compression_type_chunk->compression_name, "DST Encoded", 11);
		} else {
			compression_type_chunk->count = 14;
			memcpy(compression_type_chunk->compression_name, "not compressed", 14);
		}

		compression_type_chunk->chunk_data_size = CALC_CHUNK_SIZE(COMPRESSION_TYPE_CHUNK_SIZE - CHUNK_HEADER_SIZE + compression_type_chunk->count);
		write_ptr += CEIL_ODD_NUMBER(COMPRESSION_TYPE_CHUNK_SIZE + compression_type_chunk->count);
	}

	// The Absolute Start Time Chunk is optional but if used it may appear only once in the
	// Property Chunk.
	{
		absolute_start_time_chunk_t *absolute_start_time_chunk = (absolute_start_time_chunk_t *) write_ptr;
		absolute_start_time_chunk->chunk_id = ABSS_MARKER;
		absolute_start_time_chunk->chunk_data_size = CALC_CHUNK_SIZE(ABSOLUTE_START_TIME_CHUNK_SIZE - CHUNK_HEADER_SIZE);

		write_ptr += ABSOLUTE_START_TIME_CHUNK_SIZE;
	}

	// The Loudspeaker Configuration Chunk is optional but if used it may appear only once in
	// the Property Chunk.
	{
		uint8_t channel_count = sb_handle->channel_toc[channel]->channel_count;
		loudspeaker_config_chunk_t *loudspeaker_config_chunk = (loudspeaker_config_chunk_t *) write_ptr;
		loudspeaker_config_chunk->chunk_id = LSCO_MARKER;
		loudspeaker_config_chunk->chunk_data_size = CALC_CHUNK_SIZE(LOADSPEAKER_CONFIG_CHUNK_SIZE - CHUNK_HEADER_SIZE);

		switch (channel_count) {
			case 2:
				loudspeaker_config_chunk->loudspeaker_config = hton16(LS_CONFIG_2_CHNL);
				break;
			case 5:
				loudspeaker_config_chunk->loudspeaker_config = hton16(LS_CONFIG_5_CHNL);
				break;
			case 6:
				loudspeaker_config_chunk->loudspeaker_config = hton16(LS_CONFIG_6_CHNL);
				break;
			default:
				loudspeaker_config_chunk->loudspeaker_config = hton16(LS_CONFIG_UNDEFINED);
				break;
		}

		write_ptr += LOADSPEAKER_CONFIG_CHUNK_SIZE;
	}

	// we add a custom (unsupported) ID3 chunk to the PROP chunk to maintain all track information
	// within one file
	{
		chunk_header_t *id3_chunk;
		int id3_chunk_size;
		id3_chunk = (chunk_header_t *) write_ptr;
		id3_chunk->chunk_id = MAKE_MARKER('I','D','3',' ');
		id3_chunk_size = scarletbook_id3_tag_render(sb_handle, write_ptr + CHUNK_HEADER_SIZE, channel, track);
		id3_chunk->chunk_data_size = CALC_CHUNK_SIZE(id3_chunk_size);

		write_ptr += CEIL_ODD_NUMBER(CHUNK_HEADER_SIZE + id3_chunk_size);
	}

	// all properties have been written, now set the property chunk size
	property_chunk->chunk_data_size = CALC_CHUNK_SIZE(write_ptr - prop_ptr - CHUNK_HEADER_SIZE);

	track_size = sb_handle->channel_tracklist_offset[channel]->track_length_lsn[track] - 1;
	switch (sb_handle->channel_toc[0]->encoding) {
		case ENCODING_DSD_3_IN_14:
			track_size *= (SACD_LSN_SIZE - 32);
			break;
		case ENCODING_DSD_3_IN_16:
			track_size *= (SACD_LSN_SIZE - 284);
			break;
		case ENCODING_DST:
			break;
	}

	// Either the DSD or DST Sound Data (described below) chunk is required and may appear
	// only once in the Form DSD Chunk. The chunk must be placed after the Property Chunk.
	{
		dsd_sound_data_chunk_t * dsd_sound_data_chunk;
		dsd_sound_data_chunk = (dsd_sound_data_chunk_t *) write_ptr;
		dsd_sound_data_chunk->chunk_id = DSD_MARKER;
		dsd_sound_data_chunk->chunk_data_size = CALC_CHUNK_SIZE(track_size);
	
		write_ptr += CHUNK_HEADER_SIZE;
	}

	// Now we write the COMT comment chunk to the footer buffer
	{
		time_t rawtime;
		struct tm * timeinfo;
		comment_t * comment;
		char data[512];
		uint8_t * comment_ptr = handle->footer;
		comments_chunk_t *comment_chunk = (comments_chunk_t *) comment_ptr;
		comment_chunk->chunk_id = COMT_MARKER;
		comment_chunk->numcomments = hton16(2);

		comment_ptr += COMMENTS_CHUNK_SIZE;

		time(&rawtime);
		timeinfo = localtime(&rawtime);
		
		comment = (comment_t *) comment_ptr;
		comment->timestamp_year = hton16(timeinfo->tm_year + 1900);
		comment->timestamp_month = timeinfo->tm_mon;
		comment->timestamp_day = timeinfo->tm_mday;
		comment->timestamp_hour = timeinfo->tm_hour;
		comment->timestamp_minutes = timeinfo->tm_min;
		comment->comment_type = hton16(COMT_TYPE_FILE_HISTORY);
		comment->comment_reference = hton16(COMT_TYPE_CHANNEL_FILE_HISTORY_CREATING_MACHINE);
		sprintf(data, SACD_RIPPER_VERSION);
		comment->count = hton32(strlen(data));
		memcpy(comment->comment_text, data, strlen(data));

		comment_ptr += COMMENT_SIZE + strlen(data);

		comment = (comment_t *) comment_ptr;
		comment->timestamp_year = hton16(timeinfo->tm_year + 1900);
		comment->timestamp_month = timeinfo->tm_mon;
		comment->timestamp_day = timeinfo->tm_mday;
		comment->timestamp_hour = timeinfo->tm_hour;
		comment->timestamp_minutes = timeinfo->tm_min;
		comment->comment_type = hton16(COMT_TYPE_FILE_HISTORY);
		comment->comment_reference = hton16(COMT_TYPE_CHANNEL_FILE_HISTORY_GENERAL);
		sprintf(data, "Material ripped from SACD: %s", (char*) get_mtoc_title_text(sb_handle));
		comment->count = hton32(strlen(data));
		memcpy(comment->comment_text, data, strlen(data));

		comment_ptr += COMMENT_SIZE + strlen(data);

		handle->footer_size = CEIL_ODD_NUMBER(comment_ptr - handle->footer);
		comment_chunk->chunk_data_size = CALC_CHUNK_SIZE(handle->footer_size - COMMENTS_CHUNK_SIZE);
	}

	handle->header_size = CEIL_ODD_NUMBER(write_ptr - handle->header);
	form_dsd_chunk->chunk_data_size = CALC_CHUNK_SIZE(handle->header_size + handle->footer_size - COMMENTS_CHUNK_SIZE + CEIL_ODD_NUMBER(track_size));

#if defined(__lv2ppu__)
	{
	uint64_t nrw;
	sysFsWrite(handle->fd, handle->header, handle->header_size, &nrw); 
	}
#else
	write(handle->fd, handle->header, handle->header_size);
#endif

	return handle;
}

void dsdiff_close(dsdiff_handle_t *handle) {

	if (!handle)
		return;

#if defined(__lv2ppu__)
	{
	uint64_t nrw;
	sysFsWrite(handle->fd, handle->footer, handle->footer_size, &nrw); 
	}
#else
	write(handle->fd, handle->footer, handle->footer_size);
#endif

	if (handle->fd)
#if defined(__lv2ppu__)
		sysFsClose(handle->fd);
#else
		close(handle->fd);
#endif
	if (handle->header)
		free(handle->header);
	if (handle->footer)
		free(handle->footer);
	free(handle);
}
