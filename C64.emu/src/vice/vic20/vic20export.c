/*
 * vic20export.c - Expansion port and devices handling for the VIC20.
 *
 * Written by
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

#include <stdio.h>
#include <string.h>

#include "assert.h"
#include "export.h"
#include "lib.h"

/* #define DEBUGEXPORT */

#ifdef DEBUGEXPORT
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

export_list_t vic20export_head = { NULL, NULL, NULL };

export_list_t *export_query_list(export_list_t *item)
{
    if (item) {
        return item->next;
    } else {
        return vic20export_head.next;
    }
}

void export_dump(void)
{
    /* TODO */
}

int export_add(const export_resource_t *export_res)
{
    export_list_t *current;
    export_list_t *newentry = lib_malloc(sizeof(export_list_t));

    assert(export_res != NULL);
    DBG(("EXP: register name:%s\n", export_res->name));

    /* find last entry */
    current = &vic20export_head;
    while (current->next != NULL) {
        current = current->next;
    }
    /* add new entry at end of list */
    current->next = newentry;
    newentry->previous = current;
    newentry->device = (export_resource_t *)export_res;
    newentry->next = NULL;

    return 0;
}

int export_remove(const export_resource_t *export_res)
{
    export_list_t *current;
    export_list_t *prev;

    assert(export_res != NULL);
    DBG(("EXP: unregister name:%s\n", export_res->name));

    /* find entry */
    current = vic20export_head.next;
    while (current != NULL) {
        if (current->device) {
            if (current->device == export_res) {
                /* if entry found, remove it from list */
                prev = current->previous;
                prev->next = current->next;
                if (current->next) {
                    current->next->previous = prev;
                }
                lib_free(current);
                return 0;
            }
        }
        current = current->next;
    }
    /* FIXME: when all structs have been updated we can place an assertion here */
    DBG(("EXP: BUG unregister name: '%s' not found\n", export_res->name));
    return -1;
}

int export_resources_init(void)
{
    return 0;
}
