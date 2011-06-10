/*
 * Copyright (C) 1999-2001  Haavard Kvaalen <havardk@xmms.org>
 *
 * Licensed under GNU LGPL version 2.
 *
 */


char* charset_get_current(void);
char* charset_convert(const char *string, size_t insize, char *from, char *to);
char* charset_to_utf8(const char *string);
char* charset_from_utf8(const char *string);



