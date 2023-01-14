/**
 * SACD Ripper - https://github.com/sacd-ripper/
 *
 * Copyright (c) 2010-2015 by respective authors.
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

#ifndef VERSION_H_INCLUDED
#define VERSION_H_INCLUDED
#define _STRINGIZER(arg)     #arg
#define STRINGIZER(arg)      _STRINGIZER(arg)

//#define SACD_RIPPER_VERSION 00308
#define SACD_RIPPER_VERSION_STRING STRINGIZER(GIT_COMMIT_HASH)
#define SACD_RIPPER_REPO STRINGIZER(GIT_REPO_URL)

//#define SACD_RIPPER_VERSION_REVISION (SACD_RIPPER_VERSION / 100000)
//#define SACD_RIPPER_VERSION_MAJOR (SACD_RIPPER_VERSION / 100 % 1000)
//#define SACD_RIPPER_VERSION_MINOR (SACD_RIPPER_VERSION % 100)

#define SACD_RIPPER_VERSION_INFO                         \
    "SACD extract 0.3.9.3 \n" \
    SACD_RIPPER_VERSION_STRING "\n" \
    "Copyright (c) 2010-2020 by respective authors.\n" \

#endif /* VERSION_H_INCLUDED */
