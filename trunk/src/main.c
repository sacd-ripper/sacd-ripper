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
 
#include "sacd_reader.h"
#include "scarletbook_types.h"
#include "scarletbook_read.h"
#include "scarletbook_print.h"

#include "scarletbook_id3v2.h"

#include "dsdiff.h"

int main(int argc, char* argv[])
{
	sacd_reader_t *sacd_reader;
	scarletbook_handle_t *handle;

	if (!argv[1])
		return -1;

	sacd_reader = sacd_open(argv[1]);
	if (sacd_reader)
	{
		handle = scarletbook_open(sacd_reader, 0);
		scarletbook_print(handle);
		scarletbook_close(handle);
	}

	sacd_close(sacd_reader);

	return 0;
}
