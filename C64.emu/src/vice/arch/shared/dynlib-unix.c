/** \file   dynlib-unix.c
 * \brief   Unix support for dynamic library loading
 *
 * \author  Christian Vogelgsang <chris@vogelgsang.org>
 */

/*
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include <stdlib.h>
#include "dynlib.h"

#ifdef HAVE_DYNLIB_SUPPORT

#include <dlfcn.h>

#ifndef RTLD_LOCAL
#define RTLD_LOCAL 0
#endif

void *vice_dynlib_open(const char *name)
{
    return dlopen(name, RTLD_LAZY | RTLD_LOCAL);
}

void *vice_dynlib_symbol(void *handle,const char *name)
{
    return dlsym(handle, name);
}

char *vice_dynlib_error(void)
{
     char *error = dlerror();

     return error ? error : "no error";
}

int vice_dynlib_close(void *handle)
{
    return dlclose(handle);
}

#else

void *vice_dynlib_open(const char *name)
{
    return NULL;
}

void *vice_dynlib_symbol(void *handle,const char *name)
{
    return NULL;
}

char *vice_dynlib_error(void)
{
    return "no error";
}

int vice_dynlib_close(void *handle)
{
    return -1;
}

#endif
