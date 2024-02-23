/** \file   symtab.h
 * \brief   Symbol table for hotkeys - header
 * \author  Bas Wassink <b.wassink@ziggo.nl>
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
 */

#ifndef VICE_HOTKEYS_SYMTAB_H
#define VICE_HOTKEYS_SYMTAB_H

#include <stdbool.h>


/** \brief  Symbol table node
 *
 * Binary search tree node making up the symbol table.
 */
typedef struct symbol_s {
    char *           name;  /**< node name, allocated with lib_strdup() */
    bool             value; /**< node value */
    struct symbol_s *left;  /**< left leaf node */
    struct symbol_s *right; /**< right leaf node */
} symbol_t;


void      symbol_table_init(void);
void      symbol_table_free(void);
void      symbol_table_dump(void);

bool      symbol_add (const char *name, bool value);
symbol_t *symbol_find(const char *name);

#endif
