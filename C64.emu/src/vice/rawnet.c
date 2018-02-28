/*
 * rawnet.c - raw ethernet interface
 *
 * Written by
 *  Spiro Trikaliotis <Spiro.Trikaliotis@gmx.de>
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

#ifdef HAVE_PCAP

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "rawnet.h"
#include "rawnetarch.h"

static int (*should_accept)(unsigned char *, int, int *, int *, int *, int *, int *) = NULL;

int rawnet_should_accept(unsigned char *buffer, int length, int *phashed, int *phash_index, int *pcorrect_mac, int *pbroadcast, int *pmulticast)
{
    assert(should_accept);
    return should_accept(buffer, length, phashed, phash_index, pcorrect_mac, pbroadcast, pmulticast);
}

void rawnet_set_should_accept_func(int (*func)(unsigned char *, int, int *, int *, int *, int *, int *))
{
    should_accept = func;
}

/* ------------------------------------------------------------------------- */
/*    functions for selecting and querying available NICs                    */

int rawnet_enumadapter_open(void)
{
    if (!rawnet_arch_enumadapter_open()) {
        /* tfe_cannot_use = 1; */
        return 0;
    }
    return 1;
}

int rawnet_enumadapter(char **ppname, char **ppdescription)
{
    return rawnet_arch_enumadapter(ppname, ppdescription);
}

int rawnet_enumadapter_close(void)
{
    return rawnet_arch_enumadapter_close();
}

char *rawnet_get_standard_interface(void)
{
    return rawnet_arch_get_standard_interface();
}
#endif /* #ifdef HAVE_PCAP */
