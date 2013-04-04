/*
 * memcmp.c - 8 bit clean memory compare.
 *
 * Written by
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

#include <stdio.h>
#include <string.h>

int memcmp(const void *s1, const void *s2, size_t n)
{
    const char *p1 = (const char *)s1;
    const char *p2 = (const char *)s2;

    while (n-- > 0) {
        if (*p1 < *p2) {
            return -1;
        } else if (*p1 > *p2) {
            return +1;
        }
        p1++, p2++;
    }

    return 0;
}
