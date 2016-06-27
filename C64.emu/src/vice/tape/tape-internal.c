/*
 * tape-internal.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#include <stdlib.h>

#include "lib.h"
#include "log.h"
#include "tape-internal.h"
#include "tape.h"


int tape_internal_close_tape_image(tape_image_t *tape_image)
{
    if (tape_image_close(tape_image) < 0) {
        return -1;
    }

    lib_free(tape_image);

    return 0;
}

tape_image_t *tape_internal_open_tape_image(const char *name,
                                            unsigned int read_only)
{
    tape_image_t *image;

    image = lib_malloc(sizeof(tape_image_t));
    image->name = lib_stralloc(name);
    image->read_only = read_only;

    if (tape_image_open(image) < 0) {
        lib_free(image->name);
        lib_free(image);
        log_error(LOG_DEFAULT, "Cannot open file `%s'", name);
        return NULL;
    }

    return image;
}

void tape_internal_init(void)
{
}
