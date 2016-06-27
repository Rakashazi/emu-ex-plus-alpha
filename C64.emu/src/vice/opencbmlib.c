/*
 * opencbmlib.c - Interface to access the opencbm library.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Christian Vogelgsang <chris@vogelgsang.org>
 *
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

#include "vice.h"

#ifdef HAVE_OPENCBM

#include <stdio.h>

#include "archdep.h"
#include "log.h"
#include "opencbmlib.h"
#include "dynlib.h"

static void *opencbm_so = NULL;

/* Macro for getting function pointers from opencbm dll.  */
#define GET_SYMBOL_AND_TEST(_name_)                                               \
    opencbmlib->p_##_name_ = (_name_##_t)vice_dynlib_symbol(opencbm_so, #_name_); \
    if (opencbmlib->p_##_name_ == NULL) {                                         \
        log_debug("symbol " #_name_ " failed!");                                  \
    }

static void opencbmlib_free_library(void)
{
    if (opencbm_so != NULL) {
        if (vice_dynlib_close(opencbm_so) != 0) {
            log_debug("closing dynamic library " ARCHDEP_OPENCBM_SO_NAME " failed!");
        }
    }

    opencbm_so = NULL;
}

static int opencbmlib_load_library(opencbmlib_t *opencbmlib)
{
    if (opencbm_so == NULL) {
        opencbm_so = vice_dynlib_open(ARCHDEP_OPENCBM_SO_NAME);

        if (opencbm_so == NULL) {
            log_verbose("opening dynamic library " ARCHDEP_OPENCBM_SO_NAME " failed!");
            return -1;
        }

        GET_SYMBOL_AND_TEST(cbm_driver_open);
        GET_SYMBOL_AND_TEST(cbm_driver_close);
        GET_SYMBOL_AND_TEST(cbm_get_driver_name);
        GET_SYMBOL_AND_TEST(cbm_listen);
        GET_SYMBOL_AND_TEST(cbm_talk);
        GET_SYMBOL_AND_TEST(cbm_open);
        GET_SYMBOL_AND_TEST(cbm_close);
        GET_SYMBOL_AND_TEST(cbm_raw_read);
        GET_SYMBOL_AND_TEST(cbm_raw_write);
        GET_SYMBOL_AND_TEST(cbm_unlisten);
        GET_SYMBOL_AND_TEST(cbm_untalk);
        GET_SYMBOL_AND_TEST(cbm_get_eoi);
        GET_SYMBOL_AND_TEST(cbm_reset);

        log_verbose("sucessfully loaded " ARCHDEP_OPENCBM_SO_NAME);
    }

    return 0;
}

int opencbmlib_open(opencbmlib_t *opencbmlib)
{
    return opencbmlib_load_library(opencbmlib);
}

void opencbmlib_close(void)
{
    opencbmlib_free_library();
}

unsigned int opencbmlib_is_available(void)
{
    if (opencbm_so != NULL) {
        return 1;
    }

    return 0;
}

#endif
