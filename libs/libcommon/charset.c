/*
 * Copyright (C) 1999-2001  Haavard Kvaalen <havardk@xmms.org>
 *
 * Licensed under GNU LGPL version 2.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iconv.h>
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
	if (!charset)
		charset = "ISO-8859-1";

	return charset;
}

char* charset_convert(const char *string, size_t insize, char *from, char *to)
{
	size_t outleft, outsize;
	iconv_t cd;
	char *out, *outptr;
	const char *input = (const char *) string;

	if (!string)
		return NULL;

	if (!from)
		from = charset_get_current();
	if (!to)
		to = charset_get_current();

	if ((cd = iconv_open(to, from)) == (iconv_t)-1)
	{
		fprintf(stderr, "convert_string(): Conversion not supported. "
			  "Charsets: %s -> %s", from, to);
		return strdup(string);
	}

	/* Due to a GLIBC bug, round outbuf_size up to a multiple of 4 */
	/* + 1 for nul in case len == 1 */
	outsize = ((insize + 3) & ~3) + 2;
	out = malloc(outsize);
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
				outsize = (outsize - 1) * 2 + 2;
				out = realloc(out, outsize);
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
				fprintf(stderr, "convert_string(): Conversion failed. "
					  "Inputstring: %s; Error: %s",
					  string, strerror(errno));
				break;
		}
	}
	*outptr = '\0';
    outptr++;
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

char* charset_to_utf8(const char *string)
{
	if (!string)
		return NULL;
	return charset_convert(string, strlen(string), NULL, "UTF-8");
}
