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

#include <sysutil/sysutil.h>

#include "rsxutil.h"
#include "exit_handler.h"

static int receive_exit_request = 0;

static void sysutil_exit_callback(u64 status, u64 param, void *usrdata)
{
    switch (status)
    {
    case SYSUTIL_EXIT_GAME:
        receive_exit_request = 1;
        break;
    case SYSUTIL_DRAW_BEGIN:
    case SYSUTIL_DRAW_END:
        break;
    default:
        break;
    }
}

static void program_exit_callback()
{
    sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);

    gcmSetWaitFlip(context);
    rsxFinish(context, 1);
}

int initialize_exit_handlers()
{
    int ret;

    ret = atexit(program_exit_callback);
    if (ret != 0)
        return ret;

    ret = sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, sysutil_exit_callback, NULL);
    if (ret != 0)
        return ret;

    return 0;
}

int user_requested_exit()
{
    return receive_exit_request;
}
