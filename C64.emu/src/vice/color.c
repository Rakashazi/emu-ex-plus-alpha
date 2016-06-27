/*
 * color.c - Color management for displays using a palette.
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

#include <stdio.h>
#include <stdlib.h>

#ifdef USE_COLOR_MANAGEMENT
#include "lib.h"
#include "log.h"
#include "palette.h"
#include "types.h"
#include "uicolor.h"


struct color_rgb_s {
    unsigned int red;
    unsigned int green;
    unsigned int blue;
};
typedef struct color_rgb_s color_rgb_t;

struct owner_list_s {
    void *color_owner;
    struct owner_list_s *next;
};
typedef struct owner_list_s owner_list_t;

struct color_list_s {
    color_rgb_t color_rgb_req;
    unsigned long color_pixel;
    BYTE pixel_data;
    owner_list_t *owner;
    struct color_list_s *next;
};
typedef struct color_list_s color_list_t;

static color_list_t *color_alloced = NULL;

static log_t color_log = LOG_ERR;

/*-----------------------------------------------------------------------*/

static void color_owner_create_empty(owner_list_t **owner)
{
    *owner = lib_malloc(sizeof(owner_list_t));

    (*owner)->color_owner = NULL;
    (*owner)->next = NULL;
}

static void color_owner_free(owner_list_t *owner)
{
    owner_list_t *owner_next;

    do {
        owner_next = owner->next;
        lib_free(owner);
        owner = owner_next;
    } while (owner != NULL);
}

static int color_has_owner(owner_list_t *owner, void *c)
{
    while (owner->next != NULL)
    {
        if (owner->color_owner == c) {
            return 1;
        }
        owner = owner->next;
    }
    return 0;
}

static void color_owner_add(owner_list_t *owner, void *c)
{
    if (color_has_owner(owner, c)) {
        return;
    }

    while (owner->next != NULL) {
        owner = owner->next;
    }

    owner->color_owner = c;
    color_owner_create_empty(&owner->next);
}

static void color_owner_copy(owner_list_t *dest, owner_list_t *src)
{
    while (dest->next != NULL) {
        dest = dest->next;
    }

    while (src->next != NULL) {
        dest->color_owner = src->color_owner;
        color_owner_create_empty(&dest->next);
        dest = dest->next;
        src = src->next;
    }
}

static void color_owner_remove(owner_list_t **owner, void *c)
{
    owner_list_t *owner_list;

    if ((*owner)->next == NULL) {
        return;
    }

    if ((*owner)->color_owner == c) {
        owner_list_t *next_owner;
        next_owner = (*owner)->next;
        lib_free(*owner);
        *owner = next_owner;
        return;
    }

    owner_list = (*owner);

    while (owner_list->next != NULL && owner_list->next->next != NULL) {
        if (owner_list->next->color_owner == c) {
            owner_list_t *next_owner;
            next_owner = owner_list->next->next;
            lib_free(owner_list->next);
            owner_list->next = next_owner;
            break;
        }
        owner_list = owner_list->next;
    }
}

/*-----------------------------------------------------------------------*/

static void color_create_empty_entry(color_list_t **color_entry)
{
    *color_entry = lib_malloc(sizeof(color_list_t));

    color_owner_create_empty(&((*color_entry)->owner));
    (*color_entry)->next = NULL;
}

static void color_free(color_list_t *list)
{
    color_list_t *list_next;

    if (list == NULL) {
        return;
    }

    do {
        list_next = list->next;
        color_owner_free(list->owner);
        lib_free(list);
        list = list_next;
    } while (list != NULL);
}

static void color_palette_to_list(color_list_t *color_list, void *c,
                                  const palette_t *palette)
{
    unsigned int i;
    color_list_t *current = color_list;

    while (current->next != NULL) {
        current = current->next;
    }

    for (i = 0; i < palette->num_entries; i++) {
        current->color_rgb_req.red = palette->entries[i].red;
        current->color_rgb_req.green = palette->entries[i].green;
        current->color_rgb_req.blue = palette->entries[i].blue;
        current->color_pixel = 0;
        current->pixel_data = 0;
        color_owner_add(current->owner, c);
        color_create_empty_entry(&current->next);
        current = current->next;
    }
}

static void color_copy_entry(color_list_t *dest, color_list_t *src)
{
    while (dest->next != NULL) {
        dest = dest->next;
    }

    dest->color_rgb_req.red = src->color_rgb_req.red;
    dest->color_rgb_req.green = src->color_rgb_req.green;
    dest->color_rgb_req.blue = src->color_rgb_req.blue;
    dest->color_pixel = src->color_pixel;
    dest->pixel_data = src->pixel_data;
    color_owner_copy(dest->owner, src->owner);
    color_create_empty_entry(&dest->next);
}

static void color_copy_list(color_list_t *dest, color_list_t *src)
{
    while (src->next != NULL) {
        color_copy_entry(dest, src);
        src = src->next;
    }
}

static void color_compare_list(color_list_t *base, color_list_t *comp,
                               color_list_t *differ, color_list_t *equal)
{
    color_list_t *cbase;

    while (comp->next != NULL) {
        cbase = base;
        while (cbase->next != NULL) {
            if (comp->color_rgb_req.red == cbase->color_rgb_req.red
                && comp->color_rgb_req.green == cbase->color_rgb_req.green
                && comp->color_rgb_req.blue == cbase->color_rgb_req.blue) {
                color_copy_entry(equal, comp);
                break;
            }
            cbase = cbase->next;
        }
        if (cbase->next == NULL) {
            color_copy_entry(differ, comp);
        }
        comp = comp->next;
    }
}

static void color_remove_owner_from_list(color_list_t *list, void *c)
{
    while (list->next != NULL) {
        color_owner_remove(&list->owner, c);
        list = list->next;
    }
}

static void color_add_owner_from_other_list(color_list_t *dest,
                                            color_list_t *src)
{
    color_list_t *cdest;

    while (src->next != NULL) {
        cdest = dest;
        while (cdest->next != NULL) {
            if (src->color_rgb_req.red == cdest->color_rgb_req.red
                && src->color_rgb_req.green == cdest->color_rgb_req.green
                && src->color_rgb_req.blue == cdest->color_rgb_req.blue) {
                color_owner_add(cdest->owner, src->owner->color_owner);
                break;
            }
            cdest = cdest->next;
        }
        src = src->next;
    }
}

static void color_copy_list_with_owner(color_list_t *dest, color_list_t *src)
{
    while (src->next != NULL) {
        if (src->owner->next != NULL) {
            color_copy_entry(dest, src);
        }
        src = src->next;
    }
}

static void color_copy_list_without_owner(color_list_t *dest, color_list_t *src)
{
    while (src->next != NULL) {
        if (src->owner->next == NULL) {
            color_copy_entry(dest, src);
        }
        src = src->next;
    }
}

static void color_fill_pixel_return(color_list_t *dest, color_list_t *src,
                                    unsigned long *col_return, void *c)
{
    unsigned int colnr, dest_colnr;
    color_list_t *cdest;
    owner_list_t *cowner;

    colnr = 0;

    while (src->next != NULL) {
        cdest = dest;
        dest_colnr = 0;

        while (cdest->next != NULL) {
            cowner = cdest->owner;
            while (cowner->next != NULL) {
                if (cowner->color_owner == c) {
                    if (src->color_rgb_req.red == cdest->color_rgb_req.red
                        && src->color_rgb_req.green
                        == cdest->color_rgb_req.green
                        && src->color_rgb_req.blue
                        == cdest->color_rgb_req.blue) {
                        uicolor_convert_color_table(colnr,
                                                    &(cdest->pixel_data),
                                                    cdest->color_pixel, c);
                        if (col_return != NULL) {
                            col_return[colnr] = cdest->color_pixel;
                        }
                        colnr++;
                        goto out;
                    }
                }
                cowner = cowner->next;
            }
            cdest = cdest->next;
            dest_colnr++;
        }
out:
        src = src->next;
    }
}

/*-----------------------------------------------------------------------*/

#if 0
static void color_print_list(const char *name, color_list_t *list)
{
    log_message(color_log, "List %s start:", name);
    while (list->next != NULL) {
        owner_list_t *owner_list = list->owner;
        log_message(color_log,
                    "R %02x G %02x B %02x XCOL %08lx PIXEL %02x.",
                    list->color_rgb_req.red,
                    list->color_rgb_req.green,
                    list->color_rgb_req.blue,
                    list->color_pixel,
                    list->pixel_data);

        while (owner_list->next != NULL) {
            log_message(color_log, "Owner: %p.", owner_list->color_owner);
            owner_list = owner_list->next;
        }

        list = list->next;
    }
    log_message(color_log, "List ends.");
}
#endif

/*-----------------------------------------------------------------------*/

static int color_alloc_new_colors(color_list_t *list)
{
    color_list_t *list_start;
    BYTE data;
    unsigned long color_pixel;

    list_start = list;

    while (list->next != NULL) {
        if (uicolor_alloc_color(list->color_rgb_req.red,
                                list->color_rgb_req.green,
                                list->color_rgb_req.blue,
                                &color_pixel,
                                &data) < 0) {
            while (list_start != list) {
                uicolor_free_color(list_start->color_rgb_req.red,
                                   list_start->color_rgb_req.green,
                                   list_start->color_rgb_req.blue,
                                   list_start->color_pixel);
                list_start = list_start->next;
            }
            return -1;
        }
        list->color_pixel = color_pixel;
        list->pixel_data = data;
        list = list->next;
    }
    return 0;
}

static void color_free_old_colors(color_list_t *list)
{
    while (list->next != NULL) {
        uicolor_free_color(list->color_rgb_req.red,
                           list->color_rgb_req.green,
                           list->color_rgb_req.blue,
                           list->color_pixel);
        list = list->next;
    }
}

/*-----------------------------------------------------------------------*/

void color_init(void)
{
    color_log = log_open("Color");

    color_create_empty_entry(&color_alloced);
}

void color_shutdown(void)
{
    color_free(color_alloced);
}

int color_alloc_colors(void *c, const palette_t *palette,
                       unsigned long *col_return)
{
    color_list_t *color_new, *color_to_alloc, *color_no_alloc,
    *color_alloced_owner, *color_without_owner;

    if (palette == NULL) {
        return 0; /* no palette yet, nothing to alloc. */
    }

    /* Convert the palette to a color list.  */
    color_create_empty_entry(&color_new);
    color_palette_to_list(color_new, c, palette);

    /* This splits `color_new' into two separate lists.  */
    color_create_empty_entry(&color_to_alloc);
    color_create_empty_entry(&color_no_alloc);
    color_compare_list(color_alloced, color_new, color_to_alloc,
                       color_no_alloc);

    /* Allocate only colors we do not have.  */
    if (color_alloc_new_colors(color_to_alloc) < 0) {
        color_free(color_new);
        color_free(color_to_alloc);
        color_free(color_no_alloc);
        return -1;
    }

    /* Remove the current owner from allocated colors list.  */
    color_remove_owner_from_list(color_alloced, c);

    /* Add the owner to remaining colors if necessary.  */
    color_add_owner_from_other_list(color_alloced, color_no_alloc);

    /* Add the newly allocated colors to the allocated colors list.  */
    color_copy_list(color_alloced, color_to_alloc);

    /* Copy valid colors (with owner) to new list.  */
    color_create_empty_entry(&color_alloced_owner);
    color_copy_list_with_owner(color_alloced_owner, color_alloced);

    color_fill_pixel_return(color_alloced_owner, color_new, col_return, c);

    /* Copy invalid colors (without owner) to new list.  */
    color_create_empty_entry(&color_without_owner);
    color_copy_list_without_owner(color_without_owner, color_alloced);
    color_free_old_colors(color_without_owner);

    /* Throw away old list and temp lists.  */
    color_free(color_alloced);
    color_free(color_new);
    color_free(color_to_alloc);
    color_free(color_no_alloc);
    color_free(color_without_owner);

    /* The new list.  */
    color_alloced = color_alloced_owner;

    return 0;
}
#endif
