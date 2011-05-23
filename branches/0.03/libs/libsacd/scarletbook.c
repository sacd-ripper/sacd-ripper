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

#include "scarletbook.h"

const char *album_genre[] =
{
    "Not used"
    , "Not defined"
    , "Adult Contemporary"
    , "Alternative Rock"
    , "Children's Music"
    , "Classical"
    , "Contemporary Christian"
    , "Country"
    , "Dance"
    , "Easy Listening"
    , "Erotic"
    , "Folk"
    , "Gospel"
    , "Hip Hop"
    , "Jazz"
    , "Latin"
    , "Musical"
    , "New Age"
    , "Opera"
    , "Operetta"
    , "Pop Music"
    , "RAP"
    , "Reggae"
    , "Rock Music"
    , "Rhythm & Blues"
    , "Sound Effects"
    , "Sound Track"
    , "Spoken Word"
    , "World Music"
    , "Blues"
};

const char *album_category[] =
{
    "Not used"
    , "General"
    , "Japanese"
};

char *get_speaker_config_string(channel_toc_t *channel) 
{
    if (channel->channel_count == 2 && channel->loudspeaker_config == 0)
    {
        return "2ch.";
    }
    else if (channel->channel_count == 5 && channel->loudspeaker_config == 3)
    {
        return "5ch.";
    }
    else if (channel->channel_count == 6 && channel->loudspeaker_config == 4)
    {
        return "5.1ch";
    }
    else
    {
        return "Unknown";
    }
}

char *get_frame_format_string(channel_toc_t *channel) 
{
    if (channel->frame_format == FRAME_FORMAT_DSD_3_IN_14)
    {
        return "DSD 3 in 14";
    }
    else if (channel->frame_format == FRAME_FORMAT_DSD_3_IN_16)
    {
        return "DSD 3 in 16";
    }
    else if (channel->frame_format == FRAME_FORMAT_DST)
    {
        return "Lossless DST";
    }
    else
    {
        return "Unknown";
    }
}


