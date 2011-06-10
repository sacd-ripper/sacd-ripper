/*
 * Copyright (C) 1999-2001  Haavard Kvaalen <havardk@xmms.org>
 *
 * Licensed under GNU LGPL version 2.
 *
 */

#include <stdlib.h>
#include <string.h>

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#include <errno.h>

#ifdef HAVE_CODESET
#include <langinfo.h>
#endif

#include "charset.h"


char* charset_get_current(void)
{
	char *charset = getenv("CHARSET");

#ifdef HAVE_CODESET
	if (!charset)
		charset = nl_langinfo(CODESET);
#endif
	/* Maybe we should default to ISO-8859-1 instead? */
	if (!charset)
		charset = "US-ASCII";

	return charset;
}


#ifdef HAVE_ICONV
char* charset_convert(const char *string, size_t insize, char *from, char *to)
{
	size_t outleft, outsize;
	iconv_t cd;
	char *out, *outptr;
	ICONV_CONST char *input = (ICONV_CONST char *) string;

	if (!string)
		return NULL;

	if (!from)
		from = charset_get_current();
	if (!to)
		to = charset_get_current();

/* 	g_message("converting %s from %s to %s (%u)", string, from, to, insize); */
	if ((cd = iconv_open(to, from)) == (iconv_t)-1)
	{
		g_warning("convert_string(): Conversion not supported. "
			  "Charsets: %s -> %s", from, to);
		return strdup(string);
	}

	/* Due to a GLIBC bug, round outbuf_size up to a multiple of 4 */
	/* + 1 for nul in case len == 1 */
	outsize = ((insize + 3) & ~3) + 1;
	out = g_malloc(outsize);
	outleft = outsize - 1;
	outptr = out;

 retry:
	if (iconv(cd, &input, &insize, &outptr, &outleft) == -1)
	{
		int used;
		switch (errno)
		{
			case E2BIG:
				used = outptr - out;
				outsize = (outsize - 1) * 2 + 1;
				out = g_realloc(out, outsize);
				outptr = out + used;
				outleft = outsize - 1 - used;
				goto retry;
			case EINVAL:
				break;
			case EILSEQ:
				/* Invalid sequence, try to get the
                                   rest of the string */
				input++;
				insize--;
				goto retry;
			default:
				g_warning("convert_string(): Conversion failed. "
					  "Inputstring: %s; Error: %s",
					  string, strerror(errno));
				break;
		}
	}
	*outptr = '\0';

	iconv_close(cd);
	return out;
}

char* charset_from_utf8(const char *string)
{
	if (!string)
		return NULL;
	return charset_convert(string, strlen(string), "UTF-8", NULL);
}

#else

char* charset_convert(const char *string, size_t insize, char *from, char *to)
{
	if (!strcmp(from, "UTF-8") && !to)
		return charset_from_utf8(string);
	return strdup(string);
}

char* charset_from_utf8(const char *string)
{
 	char *ascii, *c;
	const unsigned char *utf = (const unsigned char *) string;
 	int len;

	if (!string)
		return NULL;

	len = strlen(string) + 1;
	ascii = calloc(len, 1);
	c = ascii;
	
	while (*utf != 0)
	{
		if (*utf < 0x80)
			*c++ = *utf++;
		else
		{
			char u = *utf++ << 1;
			*c++ = '?';
			
			/*
			 * Skip the entire utf8 character.
			 */
			while (u & 0x80 && *utf & 0x80 && !(*utf & 0x40))
			{
			       utf++;
			       u <<= 1;
			}
		}
	}
	return ascii;
}

#endif

char* charset_to_utf8(const char *string)
{
	if (!string)
		return NULL;
	return charset_convert(string, strlen(string), NULL, "UTF-8");
}
