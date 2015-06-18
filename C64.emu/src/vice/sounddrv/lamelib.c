/*
 * lamelib.c - Interface to access the lame lib.
 *
 * Written by
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

#if defined(USE_LAMEMP3) && !defined(HAVE_STATIC_LAME)

#include "archdep.h"
#include "lamelib.h"
#include "dynlib.h"
#include "log.h"

lamelib_t lamelib;

static void *lib_so = NULL;

/* macro for getting functionpointers from avcodec */
#define GET_SYMBOL_AND_TEST( _name_ ) \
    lamelib.p_##_name_ = (_name_##_t) vice_dynlib_symbol(lib_so, #_name_ ); \
    if (!lamelib.p_##_name_ ) { \
        log_debug("getting symbol " #_name_ " failed!"); \
        return -1; \
    }

static int load_lib(void)
{
    if (!lib_so) {
        lib_so = vice_dynlib_open(ARCHDEP_LAME_SO_NAME);

        if (!lib_so) {
            log_debug("opening dynamic library " ARCHDEP_LAME_SO_NAME " failed!");
            return -1;
        }

        GET_SYMBOL_AND_TEST(lame_init);
        GET_SYMBOL_AND_TEST(lame_close);
        GET_SYMBOL_AND_TEST(lame_set_num_channels);
        GET_SYMBOL_AND_TEST(lame_set_in_samplerate);
        GET_SYMBOL_AND_TEST(lame_set_quality);
        GET_SYMBOL_AND_TEST(lame_set_brate);
        GET_SYMBOL_AND_TEST(lame_init_params);
        GET_SYMBOL_AND_TEST(lame_encode_buffer_interleaved);
        GET_SYMBOL_AND_TEST(lame_encode_flush);
    }

    return 0;
}

static void free_lib(void)
{
    if (lib_so) {
        if (vice_dynlib_close(lib_so) != 0) {
            log_debug("closing dynamic library " ARCHDEP_LAME_SO_NAME " failed!");
        }
    }
    lib_so = NULL;

    lamelib.p_lame_init = NULL;
    lamelib.p_lame_close = NULL;
    lamelib.p_lame_set_num_channels = NULL;
    lamelib.p_lame_set_in_samplerate = NULL;
    lamelib.p_lame_set_quality = NULL;
    lamelib.p_lame_set_brate = NULL;
    lamelib.p_lame_init_params = NULL;
    lamelib.p_lame_encode_buffer_interleaved = NULL;
    lamelib.p_lame_encode_flush = NULL;
}

int lamelib_open(void)
{
    int result;

    result = load_lib();
    if (result != 0) {
        free_lib();
    }
    return result;
}

void lamelib_close(void)
{
    free_lib();
}

#endif
