/*
 * clipboard.c - Common clipboard related functions.
 *
 * Written by
 *  Spiro Trikaliotis <Spiro.Trikaliotis@gmx.de>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include <assert.h>
#include <string.h>

#include "charset.h"
#include "lib.h"
#include "mem.h"
#include "types.h"

char *clipboard_read_screen_output(char *line_ending)
{
    char * outputbuffer = NULL;

    do {
        WORD base;
        BYTE allrows, allcols;
        unsigned int row, col;
        unsigned int size;
        unsigned int line_ending_length = (unsigned int)strlen(line_ending);
        unsigned int i;
        int bank;
        char * p;

        mem_get_screen_parameter(&base, &allrows, &allcols, &bank);

        size = allrows * (allcols + line_ending_length) + 1;

        outputbuffer = lib_malloc(size);
        if (outputbuffer == NULL) {
            break;
        }

        p = outputbuffer;

        for (row = 0; row < allrows; row++) {
            char * last_non_whitespace = p - 1;

            for (col = 0; col < allcols; col++) {
                BYTE data;

                data = mem_bank_peek(bank, base++, NULL);
                data = charset_p_toascii(charset_screencode_to_petcii(data), 1);

                if (data != ' ') {
                    last_non_whitespace = p;
                }
                *p++ = data;
            }

            /* trim the line if there are only whitespace at the end */

            if (last_non_whitespace < p) {
                p = last_non_whitespace + 1;
            }

            /* add a line-ending */

            for (i = 0; i < line_ending_length; i++) {
                *p++ = line_ending[i];
            }
        }

        *p = 0;

        assert(p < outputbuffer + size);
    } while (0);

    return outputbuffer;
}
