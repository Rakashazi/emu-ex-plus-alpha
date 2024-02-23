/** \file   hotkeystypes.h
 * \brief   Types used by the hotkeys system
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

#ifndef VICE_HOTKEYS_HOTKEYSTYPES_H
#define VICE_HOTKEYS_HOTKEYSTYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "log.h"


/** \brief  Comment token */
#define VHK_COMMENT         '#'

/** \brief  Alternative comment token */
#define VHK_COMMENT_ALT     ';'

/** \brief  Keyword token */
#define VHK_KEYWORD         '!'

/** \brief  Modifier opening token */
#define VHK_MODIFIER_OPEN   '<'

/** \brief  Modifier closing token */
#define VHK_MODIFIER_CLOSE  '>'

/** \brief  Quote token */
#define VHK_QUOTE           '"'

/** \brief  Token for 'escaping' the following token */
#define VHK_ESCAPE          '\\'


/** \brief  Hotkey file keyword IDs
 */
typedef enum vhk_keyword_id_e {
    VHK_KW_ID_ILLEGAL = -1, /**< illegal directive */
    VHK_KW_ID_CLEAR,        /**< clear all hotkeys */
    VHK_KW_ID_DEBUG,        /**< enable/disable debuging messages */
    VHK_KW_ID_INCLUDE,      /**< include "file" */
    VHK_KW_ID_IF,           /**< check condition (TODO) */
    VHK_KW_ID_ELSE,         /**< inverse branch of conditional block (TODO) */
    VHK_KW_ID_ENDIF,        /**< end of conditional block (TODO) */
    VHK_KW_ID_UNDEF,        /**< clear hotkey */
    VHK_KW_ID_WARNING       /**< show warning in hotkeys log */
} vhk_keyword_id_t;

/** \brief  Parser keyword data object
 */
typedef struct vhk_keyword_s {
    const char *        name;       /**< name */
    vhk_keyword_id_t    id;         /**< ID */
    int                 args_min;   /**< minimum number of arguments */
    int                 args_max;   /**< maximum number of arguments */
    const char *        syntax;     /**< syntax */
    const char *        desc;       /**< description */
} vhk_keyword_t;

/** \brief  Modifier mask data */
typedef struct vhk_modifier_s {
    uint32_t    mask;   /**< modifier mask */
    const char *name;   /**< name in .vhk files */
    const char *desc;   /**< description */
} vhk_modifier_t;

/** \brief  Mapping of a hotkey to menu items
 *
 */
typedef struct vhk_map_s {
    int       action;       /**< action ID */
    uint32_t  vice_keysym;  /**< VICE keysym */
    uint32_t  vice_modmask; /**< VICE modifiers */
    uint32_t  arch_keysym;  /**< arch keysym */
    uint32_t  arch_modmask; /**< arch modmask */
    void     *menu_item[2]; /**< menu items: primary and secondary display */
    void     *user_data;    /**< additional arch-specific data */
} vhk_map_t;

/** \brief  File entry object for the textfile reader
 *
 * Singly linked list to implement a stack.
 */
typedef struct textfile_entry_s {
    char *                      path;       /**< full path of file */
    long                        pos;        /**< position in file */
    long                        linenum;    /**< current line number in file */
    struct textfile_entry_s *   next;       /**< next entry in stack */
} textfile_entry_t;

/** \brief  Object to read text files that can 'include' others
 */
typedef struct textfile_reader_s {
    char *              buffer;     /**< buffer holding a single line of text */
    size_t              bufsize;    /**< size of buffer */
    size_t              buflen;     /**< length of text in buffer */
    FILE *              fp;         /**< file pointer for the current file */
    textfile_entry_t *  entries;    /**< stack of files */
} textfile_reader_t;

/** \brief Source of vhk file
 *
 * Source of the vhk file parsed, used in the UI to display proper path and
 * enable/disable widgets.
 */
typedef enum vhk_source_e {
    VHK_SOURCE_NONE,    /**< no source (no vhk file loaded) */
    VHK_SOURCE_VICE,    /**< default location in VICE data dir */
    VHK_SOURCE_USER,    /**< UI and emu-specific file in user config dir */
    VHK_SOURCE_RESOURCE /**< path in resource "HotkeyFile */
} vhk_source_t;

/* The hotkeys log, perhaps turn this into an accessor instead of this ugly
 * extern declaration? */
extern log_t vhk_log;

#endif
