/*
 * Copyright (C) 2000-2001 the xine project
 *
 * This file is part of xine, a unix video player.
 *
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * WIN32 PORT,
 * by Matthew Grooms <elon@altavista.com>
 *
 * unistd.h - This is mostly a catch all header that maps standard unix
 *            libc calls to the equivelent win32 functions.
 *
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif 
#include <windows.h>
#include <malloc.h>
#include <errno.h>
#include <direct.h>

#ifndef _SYS_UNISTD_H_
#define _SYS_UNISTD_H_

#ifndef inline
#define inline __inline
#endif

#ifndef ssize_t
#define ssize_t __int64
#endif

#define mkdir( A, B )   _mkdir( A )
#define lstat           stat

#ifndef S_ISDIR
#define S_ISDIR(A)      ( S_IFDIR & A )
#endif
#ifndef S_ISREG
#define S_ISREG(m) ((m) & _S_IFREG)
#endif
#ifndef S_ISBLK
#define S_ISBLK(m) 0
#endif
#ifndef S_ISCHR
#define S_ISCHR(m) 0 
#endif

#define S_IXUSR         S_IEXEC
#define S_IXGRP         S_IEXEC
#define S_IXOTH         S_IEXEC

#define  M_PI           3.14159265358979323846  /* pi */

#define bzero( A, B ) memset( A, 0, B )

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif

#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif

#define snprintf _snprintf
#define vsnprintf _vsnprintf

// FIXME : I dont remember why this is here
#define readlink

#endif
