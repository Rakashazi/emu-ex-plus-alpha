/** \file   symtab.c
 * \brief   Symbol table for hotkeys
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

#include "vice.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "lib.h"

#include "symtab.h"


/** \brief  Symbol table root node
 */
static symbol_t *symbols = NULL;

/*
 * Private functions
 */

/** \brief  Allocate memory for symbol table node
 *
 * Allocate new symbol table node with \a name and \a value set.
 *
 * \param[in]   name    symbol name
 * \param[in]   value   symbol value
 *
 * \return  new node, free with symbol_free()
 */
static symbol_t *node_new(const char *name, bool value)
{
    symbol_t *node = lib_malloc(sizeof *node);

    node->name  = lib_strdup(name);
    node->value = value;
    node->left  = NULL;
    node->right = NULL;
    return node;
}

/** \brief  Free memory used by \a node and its children
 *
 * Free \a node and recursively its child nodes.
 *
 * \param[in]   node    node to free
 */
static void node_free(symbol_t *node)
{
    if (node->left != NULL) {
        node_free(node->left);
    }
    if (node->right != NULL) {
        node_free(node->right);
    }
    lib_free(node->name);
    lib_free(node);
}

/** \brief  Add node to symbol table
 *
 * \param[in]   node    node to start looking for empty leaf node
 * \param[in]   name    name for new node
 * \param[in]   value   value for new node
 *
 * \return  new node or \c NULL on name collision
 */
static symbol_t *node_add(symbol_t *node, const char *name, bool value)
{
    if (node == NULL) {
        node = node_new(name, value);
    } else {
        int d = strcmp(name, node->name);
        if (d == 0) {
            node = NULL;    /* collision */
        } else if (d < 0) {
            node->left = node_add(node->left, name, value);
        } else {
            node->right = node_add(node->right, name, value);
        }
    }
    return node;
}

/** \brief  Find node in symbol table
 *
 * Look up node by \a name in symbol table, starting at \a node.
 *
 * \param[in]   node    starting node in symbol table
 * \param[in]   name    symbol name
 *
 * \return  node or \c NULL when not found
 */
static symbol_t *node_find(symbol_t *node, const char *name)
{
    if (node != NULL) {
        int d = strcmp(name, node->name);
#if 0
        printf("%s(): strcmp(\"%s\", \"%s\") == %d\n",
               __func__, node->name, name, d);
#endif
        if (d < 0) {
            node = node_find(node->left, name);
        } else if (d > 0) {
            node = node_find(node->right, name);
        }
    }
    return node;
}

/** \brief  Debug hook: dump info on (part of) symbol table on stdout
 *
 * Print symbol table in ascending alphabetical order on stdout.
 *
 * \param[in]   node    starting node
 */
static void node_dump(symbol_t *node)
{
    if (node != NULL) {
        node_dump(node->left);
        printf("symbol: %-20s => %s\n", node->name, node->value ? "true" : "false");
        node_dump(node->right);
    }
}


/*
 * Public functions
 */

/** \brief  Initialize symbol table */
void symbol_table_init(void)
{
    symbols = NULL;
}

/** \brief  Free memory used by symbol table */
void symbol_table_free(void)
{
    if (symbols != NULL) {
        node_free(symbols);
        symbols = NULL;
    }
}


/** \brief  Debug hook: dump symbol table on stdout
 *
 * Print symbol table in alphabetical order on stdout.
 */
void symbol_table_dump(void)
{
    node_dump(symbols);
}


/** \brief  Add symbol to symbol table
 *
 * Add symbol called \a name with value \a value to symbol table.
 *
 * \param[in]   name    symbol name
 * \param[in]   value   symbol value
 *
 * \return  \c false on name collision
 */
bool symbol_add(const char *name, bool value)
{
    if (symbols == NULL) {
        symbols = node_new(name, value);
    } else {
        symbol_t *node = node_add(symbols, name, value);
        if (node == NULL) {
            /* collision */
            return false;
        }
    }
    return true;
}


/** \brief  Look up symbol by name
 *
 * \param[in]   name    symbol name
 *
 * \return  node or \c NULL when not found
 */
symbol_t *symbol_find(const char *name)
{
    return node_find(symbols, name);
}
