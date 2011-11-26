/*
 * Copyright (C) 2004  Haavard Kvaalen <havardk@xmms.org>
 *
 * Licensed under GNU GPL version 2.
 *
 */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "charset.h"

static size_t utf16_strlen(const unsigned char *string)
{
	size_t len = 0;

	while (*(string + len) != 0 || *(string + len + 1) != 0)
		len += 2;

	return len;
}

#ifdef HAVE_ICONV

char *convert_from_utf16(const unsigned char *utf16)
{
	return charset_convert(utf16, utf16_strlen(utf16), "UTF-16", NULL);
}

char *convert_from_utf16be(const unsigned char *utf16)
{
	return charset_convert(utf16, utf16_strlen(utf16), "UTF-16BE", NULL);
}


#else


static char* to_ascii(const unsigned char *utf16, int le)
{
	char *ascii;
	unsigned int i, len, c;

	len = utf16_strlen(utf16) / 2 + 1;
	
	ascii = malloc(len + 1);

	for (i = 0, c = 0; i < len; i++)
	{
		uint16_t uc;
		int o = i << 1;

		if (le)
			uc = *(utf16 + o) | *(utf16 + o + 1) << 8;
		else
			uc = *(utf16 + o) << 8 | *(utf16 + o + 1);

		/* Skip BOM and surrogate pairs */
		if (uc == 0xfeff || (uc >= 0xd800 && uc <= 0xdfff))
			continue;
		
		if (uc < 0x80)
			ascii[c] = (uint8_t) uc;
		else 
			ascii[c] = '?';
		c++;
	}
	
	ascii[c] = 0;
	return ascii;
}
	
char *convert_from_utf16(const unsigned char *utf16)
{
	int le = 0;
	uint16_t bom = *utf16 << 8 | *(utf16 + 1);

	if (bom == 0xfffe)
		le = 1;
	else if (bom != 0xfeff)
		return strdup("");

	return to_ascii(utf16, le);
}

char *convert_from_utf16be(const unsigned char *utf16)
{
	return to_ascii(utf16, 0);
}

#endif
