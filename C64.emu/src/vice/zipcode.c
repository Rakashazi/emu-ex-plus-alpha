/*
 * zipcode.c - Support for zipcode files in VICE.
 *
 * Written by
 *  Daniel Sladic <sladic@eecg.toronto.edu>
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

#include <stdio.h>

#include "types.h"
#include "zipcode.h"

int zipcode_read_sector(FILE *zip_fd, int track, int *sector, char *buf)
{
    unsigned char trk, sec, len, rep, repnum, chra;
    unsigned int i, j, count;
    size_t t1, t2;

    t1 = fread((char *)&trk, 1, 1, zip_fd);
    t2 = fread((char *)&sec, 1, 1, zip_fd);

    *sector = sec;

    if ((trk & 0x3f) != track || !t1 || !t2) {
        return -1;
    }

    if (trk & 0x80) {
        t1 = fread((char *)&len, 1, 1, zip_fd);
        t2 = fread((char *)&rep, 1, 1, zip_fd);
        if (!t1 || !t2) {
            return -2;
        }

        count = 0;

        for (i = 0; i < len; i++) {
            if ((t1 = fread((char *)&chra, 1, 1, zip_fd)) == 0) {
                return -3;
            }

            if (chra != rep) {
                buf[count++] = chra;
                continue;
            }

            t1 = fread((char *)&repnum, 1, 1, zip_fd);
            t2 = fread((char *)&chra, 1, 1, zip_fd);
            if (!t1 || !t2) {
                return 1;
            }
            i += 2;
            for (j = 0; j < repnum; j++) {
                buf[count++] = chra;
            }
        }
        return 0;
    }

    if (trk & 0x40) {
        if ((t1 = fread((char *)&chra, 1, 1, zip_fd)) == 0) {
            return -4;
        }

        for (i = 0; i < 256; i++) {
            buf[i] = chra;
        }

        return 0;
    }

    if (fread(buf, 256, 1, zip_fd) < 1) {
        return -5;
    }

    return 0;
}
