/** \file   parser.c
 * \brief   Parsing functions for hotkeys
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
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "archdep_defs.h"
#include "archdep_get_vice_hotkeysdir.h"
#include "archdep_path_is_relative.h"
#include "boolexpr.h"
#include "ifstack.h"
#include "lib.h"
#include "log.h"
#include "symtab.h"
#include "sysfile.h"
#include "textfilereader.h"
#include "uiactions.h"
#include "uiapi.h"
#include "uihotkeys.h"
#include "util.h"
#include "vhkkeysyms.h"

#include "parser.h"


/* #define DEBUG_VHK */
#include "vhkdebug.h"

/* Define this to dump the symbol table on stdout when parsing a hotkeys file */
/* #define DUMP_SYMTAB */


/** \brief  Array length helper
 *
 * \param[in]   arr array name
 */
#define ARRAY_LEN(arr)  (sizeof (arr) / sizeof (arr[0]) )

/** \brief  Object for mapping of !DEBUG arguments to boolean
 */
typedef struct debug_args_s {
    const char *symbol; /**< string literal */
    bool        value;  /**< boolean value */
} debug_args_t;

/** \brief  Predefined symbol declaration */
typedef struct predef_symbol_s {
    const char *name;   /**< symbol name */
    bool        value;  /**< symbol value */
} predef_symbol_t;


/** \brief  Hotkeys parser keyword list
 *
 * List of parser keywords with ID and argument count.
 *
 * \note    The array needs to stay in alphabetical order.
 */
static const vhk_keyword_t vhk_keyword_list[] = {
    { "clear",      VHK_KW_ID_CLEAR,    0, 0,
      "!CLEAR",
      "Clear all hotkeys" },

    { "debug",      VHK_KW_ID_DEBUG,    1, 1,
      "!DEBUG <enable|disable|on|off>",
      "Turn debugging output on or off" },

    { "else",       VHK_KW_ID_ELSE,     0, 0,
      "!ELSE",
      "Execute following statement(s) if the preceding !IF <condition> is not met" },

    { "endif",      VHK_KW_ID_ENDIF,    0, 0,
      "!ENDIF",
      "End of !if [!else] block" },

    { "if",         VHK_KW_ID_IF,       1, 1,
      "!IF <condition>",
      "Execute following statement(s) if <condition> is met" },

    { "include",    VHK_KW_ID_INCLUDE,  1, 1,
      "!INCLUDE <path>",
      "Start parsing hotkeys file <path>" },

    { "undef",      VHK_KW_ID_UNDEF,    1, 1,
      "!UNDEF <modifiers+key>",
      "Undefine a hotkey by <modifiers+key>" },

    { "warning",    VHK_KW_ID_WARNING,  1, 1,
      "!WARNING <message>",
      "Show warning in hotkeys log" }
};

/** \brief  Mapping of !DEBUG arguments to boolean
 */
static const debug_args_t debug_arglist[] = {
    { "enable",     true },
    { "disable",    false },
    { "on",         true },
    { "off",        false }
};

/** \brief  Debug messages enable flag
 */
static bool vhk_debug = false;

/** \brief  Global text file reader instance
 *
 * Stack based file reader, initialized and cleaned up in \c vhk_parser_parse().
 */
textfile_reader_t reader;

/** \brief  Buffer for identifiers found in conditionals */
static char identbuf[256];


/** \brief  Log error message
 *
 * Log error message on vhk log using printf-style format string, including the
 * current filename and line number.
 *
 * \param[in]   fmt     format string
 * \param[in]   ...     arguments for \a fmt (optional)
 */
static void vhk_parser_error(const char *fmt, ...)
{
    char        buffer[1024];
    va_list     args;
    const char *filename;
    long        linenum;

    filename = textfile_reader_filename(&reader);
    linenum  = textfile_reader_linenum(&reader);

    va_start(args, fmt);
    vsnprintf(buffer, sizeof buffer, fmt, args);
    if (filename != NULL && linenum > 0) {
        log_error(vhk_log, "%s:%ld: %s", filename, linenum, buffer);
    } else {
        log_error(vhk_log, "%s", buffer);
    }
    va_end(args);
}

/** \brief  Log warning
 *
 * Log warning on vhk log using printf-style format string, including the
 * current filename and line number.
 *
 * \param[in]   fmt     format string
 * \param[in]   ...     arguments for \a fmt (optional)
 */
static void vhk_parser_warning(const char *fmt, ...)
{
    char    buffer[1024];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof buffer, fmt, args);
    log_warning(vhk_log,
                "%s:%ld: %s",
                textfile_reader_filename(&reader),
                textfile_reader_linenum(&reader),
                buffer);
    va_end(args);
}

/** \brief  Strip leading and trailing whitespace
 *
 * Remove leading and trailing whitespace from string \a s.
 *
 * \param[in]   s   string
 *
 * \return  heap-allocated string
 *
 * \note    free with lib_free()
 * \todo    move into src/lib.c?
 */
static char *vhk_parser_strtrim(const char *s)
{
    char *t;

    s = util_skip_whitespace(s);

    if (*s == '\0') {
        /* empty string */
        t = lib_calloc(1, 1);
    } else {
        /* trim trailing whitespace */
        const char *p = util_skip_whitespace_trailing(s);
        p++;
        /* add 1 for the terminating nul char */
        t = lib_malloc(p - s + 1);
        memcpy(t, s, p - s);
        t[p - s] = '\0';
    }
    return t;
}

/** \brief  Check if string only contains whitespace or an inline comment
 *
 * \param[in]   s   string to check
 *
 * \return  \c true if only whitespace or comment
 */
static bool no_more_tokens(const char *s)
{
    while (*s != '\0' && isspace((unsigned char)*s)) {
        s++;
    }
    return (bool)(*s == '\0' || *s == VHK_COMMENT || *s == VHK_COMMENT_ALT);
}

/** \brief  Open file, optionally using the VICE hotkeys dir as search path
 *
 * Try opening \a path as-is first, if that fails prepend the VICE hotkeys
 * directory, if \a path is relative, and try that.
 *
 * \param[in]   path    path to file to open
 *
 * \return  \c true on success
 */
static bool vhk_parser_open_file(const char *path)
{
    bool result = false;

    /* try opening as-is */
    result = textfile_reader_open(&reader, path);
    if (!result) {
        if (archdep_path_is_relative(path)) {
            char *fullpath = NULL;

            /* try to locate the file in the vice data dir (which is the root
             * directory of a VICE distribution on Windows) */
            if (sysfile_locate(path, NULL, &fullpath) == 0) {
                result = textfile_reader_open(&reader, fullpath);
                lib_free(fullpath);
            } else if (sysfile_locate(path, "hotkeys", &fullpath) == 0) {
                result = textfile_reader_open(&reader, fullpath);
                lib_free(fullpath);
            }
        }
    }
    if (!result) {
        vhk_parser_error("failed to open '%s': errno %d (%s)",
                         path, errno, strerror(errno));
    }
    return result;
}

/* currently unused, but might be required again later, so do not delete */
#if 0
/** \brief  Substitute a substring with another string
 *
 * Replace first occurences of \a search in \a original with \a replace.
 *
 * \param[in]   original    original string
 * \param[in]   search      (sub)string of \a original to replace
 * \param[in]   replace     replacement for \a search
 *
 * \return  heap-allocated string
 *
 * \note    free result with lib_free()
 */
static char *vhk_parser_strsubst(const char *original,
                                 const char *search,
                                 const char *replace)
{
    char *result = NULL;

    if (original == NULL || *original == '\0') {
        /* empty string, return empty string */
        result = lib_calloc(1, 1);
    } else if (search == NULL || *search == '\0') {
        /* no search string, return copy of original */
        result = lib_strdup(original);
    } else {
        /* start scanning */
        size_t slen = strlen(search);
        size_t olen = strlen(original);

        if (slen > olen) {
            /* search string longer than original, will never find a match */
            result = lib_strdup(original);
        } else {
            /* search and destroy! */
            const char *pos = strstr(original, search);
            if (pos == NULL) {
                /* no match, return copy of original */
                result = lib_strdup(original);
            } else {
                const char *opos;    /* position in original */
                char       *rpos;    /* position in result */
                size_t      reslen;
                size_t      rlen = strlen(replace);

                /* avoid juggling with conversion/promotion */
                if (slen >= rlen) {
                    /* result string will be shorter or the same size */
                    reslen = olen - (slen - rlen);
                } else {
                    /* result string will be larger */
                    reslen = olen + (rlen - slen);
                }

                rpos   = lib_malloc(reslen + 1);
                opos   = original;
                result = rpos;

                /* copy slice before the substitution, if any */
                if (pos > original) {
                    memcpy(rpos, original, pos - original);
                    rpos += pos - original;
                    opos += pos - original;
                }
                /* copy substitution */
                memcpy(rpos, replace, rlen);
                rpos += rlen;
                opos += slen;
                /* copy slice after the substitution, if any */
                if (*opos != '\0') {
                    strcpy(rpos, opos);
                }
                /* terminate string */
                result[reslen] = '\0';
            }

        }
    }
    return result;
}
#endif

/** \brief  Scan string for keyword
 *
 * Scan string \a name for a keyword and return keyword ID on match.
 *
 * The input is allowed to have trailing characters starting with whitespace,
 * which could be the keyword's argument list. The input is expected to not
 * contain the keyword indicator token. Matching is done case-insensitive.
 *
 * \param[in]   name    string with possible keyword
 *
 * \return  keyword ID when found, -1 otherwise
 */
static vhk_keyword_id_t vhk_parser_get_keyword_id(const char  *name,
                                                  const char **endptr)
{
    size_t i = 0;

#if 0
    debug_vhk("Looking up '%s':", name);
#endif
    for (i = 0; i < ARRAY_LEN(vhk_keyword_list); i++) {
        int k = 0;
        const vhk_keyword_t *kw = &(vhk_keyword_list[i]);
        const char          *kwname = kw->name;
#if 0
        debug_vhk("Checking against '%s'.", kw->name);
#endif
        /* do lower case compare */
        while (kwname[k] != '\0'
                && name[k] != '\0'
                && tolower((int)name[k]) == tolower((int)kwname[k])) {
            k++;
        }
        if (kwname[k] == '\0' && name[k] == '\0') {
            /* full match, return id */
            if (endptr != NULL) {
                *endptr = name + k;
            }
            return kw->id;
        } else if (kwname[k] == '\0') {
            /* input matched keyword so far, but input contains more
             * characters */
            if (isspace((unsigned char)name[k])) {
                /* remaining input cannot be part of a keyword: match */
                if (endptr != NULL) {
                    *endptr = name + k;
                }
                return kw->id;
            }
        } else if (name[k] == '\0') {
            /* input matched keyword so far, but keyword contains more
             * characters: keyword is higher in alphabetical sort
             * order, so we cannot get a match */
            return VHK_KW_ID_ILLEGAL;
        }
        /* keep looking, next keyword is higher in alphabetical sort
         * order than the current keyword and could match */
    }

    /* list exhausted, no match */
    return VHK_KW_ID_ILLEGAL;
}

/** \brief  Parse string for modifier
 *
 * Parse string \a s for a known modifier name closed by '\>', store pointer
 * to closing angled bracket in \a endptr (if not `NULL`).
 * Matching is done case-insensitive.
 *
 * \param[in]   s       string to parse
 * \param[out]  endptr  pointer to closing bracket (can be `NULL`)
 *
 * \return  modifier mask or `VHK_MOD_NONE` (0) when no valid modifier found
 */
static uint32_t parser_get_modifier(const char *s, const char **endptr)
{
    return vhk_modifier_from_name(s, endptr);
}

/** \brief  Parse string for keysym and optional modifiers
 *
 * \param[in]   line            string to parse
 * \param[out]  endptr          first character in \a line past the key name
 * \param[out]  vice_keysym     VICE modifier mask
 * \param[out]  vice_modmask    VICE keysym
 * \param[out]  arch_keysym     arch keysym
 * \param[out]  arch_modmask    arch modifier mask
 *
 * \return  `true` on success
 */
static bool vhk_parser_get_keysym_and_modmask(const char  *line,
                                              const char **endptr,
                                              uint32_t    *vice_keysym,
                                              uint32_t    *vice_modmask,
                                              uint32_t    *arch_keysym,
                                              uint32_t    *arch_modmask)
{
    const char *curpos;
    const char *oldpos;
    char        keyname[256];
    ptrdiff_t   keylen;
    uint32_t    v_key;
    uint32_t    a_key;
    uint32_t    v_mask = 0;
    uint32_t    a_mask = 0;


    curpos = line;

    if (arch_keysym != NULL) {
        *arch_keysym = 0;
    }
    if (arch_modmask != NULL) {
        *arch_modmask = 0;
    }
    if (vice_keysym != NULL) {
        *vice_keysym = 0;
    }
    if (vice_modmask != NULL) {
        *vice_modmask = 0;
    }

    /* collect modifiers */
    while (*curpos != '\0') {
        uint32_t    v_mod;
        const char *end = NULL;

        if (*curpos != VHK_MODIFIER_OPEN) {
            /* no opening '<' found, must be key name */
            break;
        }

        curpos++;   /* skip '<' */
        v_mod = parser_get_modifier(curpos, &end);
        if (v_mod == VHK_MOD_NONE) {
            vhk_parser_error("parse error: unknown modifier '%s'.", curpos);
            return false;
        }
        /* add found modifier to final mask */
        v_mask |= v_mod;
        a_mask |= ui_hotkeys_arch_modifier_to_arch(v_mod);
        curpos = end + 1;   /* move past closing '>' */
    }

    /* end of modifiers, get key name */
    oldpos = curpos;
    while (*curpos != '\0' && (isalpha((unsigned char)*curpos) ||
                               isdigit((unsigned char)*curpos) ||
                               *curpos == '_')) {
        curpos++;
    }
    keylen = curpos - oldpos;
    if (keylen == 0) {
        /* error, no key name found */
        vhk_parser_error("parse error: no key name found.");
        return false;
    }
    if (keylen >= (ptrdiff_t)sizeof keyname) {
        /* key name is way too long, will never match */
        vhk_parser_error("parse error: key name exceeds allow size (%"PRI_SIZE_T")",
                         sizeof keyname - 1u);
        return false;
    }
    memcpy(keyname, oldpos, keylen);
    keyname[keylen] = '\0';

    /* set results */
    v_key = vhk_keysym_from_name(keyname);
    if (v_key == 0) {
        vhk_parser_error("parse error: unknown key name '%s'.", keyname);
        return false;
    }
    a_key = ui_hotkeys_arch_keysym_to_arch(v_key);

    debug_vhk("VICE vhk key name  = \"%s\"",keyname);
    debug_vhk("VICE keysym + mods = %04x, %08x", v_key, v_mask);
    debug_vhk("arch keysym + mods = %04x, %08x", a_key, a_mask);

    if (vice_keysym != NULL) {
        *vice_keysym = v_key;
    }
    if (vice_modmask != NULL) {
        *vice_modmask = v_mask;
    }
    if (arch_keysym != NULL) {
        *arch_keysym = a_key;
    }
    if (arch_modmask != NULL) {
        *arch_modmask = a_mask;
    }
    if (endptr != NULL) {
        *endptr = curpos;
    }

    return true;
}


/*
 * .vhk file directive handlers
 */

/** \brief  Handler for the '!CLEAR' keyword
 *
 * Clears all hotkeys registered.
 *
 * \param[in]   line    text to parse (unused)
 *
 * \return  bool
 */
static bool vhk_parser_do_clear(const char *line)
{
    if (!no_more_tokens(line)) {
        vhk_parser_error("syntax error: trailing tokens after !clear.");
        return false;
    }
    if (ifstack_true()) {
        if (vhk_debug) {
            log_message(vhk_log,
                        "Hotkeys: %s:%ld: !clear -> clearing all hotkeys.",
                        textfile_reader_filename(&reader),
                        textfile_reader_linenum(&reader));
        }
        ui_hotkeys_remove_all();
    }
    return true;
}

/** \brief  Handler for the '!DEBUG' keyword
 *
 * Enable/disable debugging messages.
 *
 * \param[in]   line    text to parse (unused)
 *
 * \return  bool
 */
static bool vhk_parser_do_debug(const char *line)
{
    const char *arg;
    size_t i;

    arg = util_skip_whitespace(line);
    for (i = 0; i < ARRAY_LEN(debug_arglist); i++) {
        if (util_strncasecmp(debug_arglist[i].symbol,
                             arg,
                             strlen(debug_arglist[i].symbol)) == 0) {
            /* TODO: report debug on/off as part of debugging, but only
             *       mention 'off' if it was previously 'on': this way using
             *       '!debug off' at the start of a vhk file won't trigger a
             *       debugging message.
             */
            if (ifstack_true()) {
                vhk_debug = debug_arglist[i].value;
            }
            return true;
        }
    }
    /* no match */
    vhk_parser_error("syntax error: invalid argument to !debug: '%s'.", arg);
    return false;
}

/** \brief  Parse quoted argument, removing any (escaped) quotes
 *
 * \param[in]   text    text to parse
 *
 * \return  path stripped from quotes or \c NULL on error
 * \note    free result with \c lib_free() after use
 */
static char *parse_quoted_argument(const char *text)
{
    const char *s = text;
    char       *arg = NULL;
    char       *a;

    /* quotes? */
    if (*s == VHK_QUOTE) {
        /* allocate memory for resulting arg:
         * -2 to skip enclosing quotes,
         * +1 for terminating nul */
        arg = a = lib_malloc(strlen(text) - 2 + 1);

        /* copy string, turning any '\"' sequence into '"' */
        s++;    /* move after opening quote */
        while (*s != '\0') {
            if (*s == VHK_ESCAPE) {
                s++;
                if (*s == '\0') {
                    /* end of string, but escape token active */
                    vhk_parser_error("parse error: unexpected end of line.");
                    lib_free(arg);
                    return NULL;
                }
                *a++ = *s++;
            } else if (*s == VHK_QUOTE) {
                /* found closing quote */
                break;
            } else {
                *a++ = *s++;
            }
        }
        if (*s != VHK_QUOTE) {
            /* error, no closing quote */
            vhk_parser_error("syntax error: missing closing quote.");
            lib_free(arg);
            return NULL;
        }
        /* terminate result */
        *a = '\0';
    } else {
        /* no quotes, copy until first whitespace character */
        arg = a = lib_malloc(strlen(text) + 1);
        while (*s != '\0' && !isspace((unsigned char)*s)) {
            *a++ = *s++;
        }
        *a = '\0';
    }
    return arg;
}

/** \brief  Handler for the '!INCLUDE' a keyword
 *
 * \param[in]   line    text to parse, must start *after* "!INCLUDE"
 *
 * \return  bool
 *
 * \note    errors are reported with \c vhk_parser_error()
 */
static bool vhk_parser_do_include(const char *line)
{
    bool  result;
    char *path;

    line = util_skip_whitespace(line);
    if (*line == '\0') {
        /* missing argument */
        vhk_parser_error("syntax error: missing argument for !include.");
        return false;
    }

    path = parse_quoted_argument(line);
    if (path == NULL) {
        /* error already logged */
        return false;
    }
    result = vhk_parser_open_file(path);
    /* any error opening the file is already reported */
    lib_free(path);
    return result;
}

/** \brief  Handler for the '!UNDEF' keyword
 *
 * \param[in]   line    text to parse
 *
 * \return  bool
 *
 * \note    errors are reported with log_message()
 *
 * \todo    Support two windows for x128
 */
static bool vhk_parser_do_undef(const char *line)
{
    const char       *curpos;
    const char       *oldpos;
    uint32_t          vice_mask;
    uint32_t          vice_key;
    uint32_t          arch_mask;
    uint32_t          arch_key;
    ui_action_map_t  *map;

    curpos = util_skip_whitespace(line);
    if (*curpos == '\0') {
        /* error: missing argument */
        vhk_parser_error("syntax error: missing argument for !undef.");
        return true;
    }

    /* get combined modifier masks and keyval */
    oldpos = curpos;
    if (!vhk_parser_get_keysym_and_modmask(curpos, &curpos,
                                           &vice_mask, &vice_key,
                                           &arch_mask, &arch_key)) {
        /* error already logged */
        return false;
    }
    if (vhk_debug) {
        log_message(vhk_log,
                    "Hotkeys %s:%ld: VICE key: %04x, VICE mask: %08x, keyname: %s.",
                    textfile_reader_filename(&reader),
                    textfile_reader_linenum(&reader),
                    vice_key,
                    vice_mask,
                    vhk_keysym_name(vice_key));
    }

    /* lookup map for hotkey */
    if (ifstack_true()) {
        map = ui_action_map_get_by_hotkey(vice_key, vice_mask);
        if (map != NULL) {
            if (vhk_debug) {
                log_message(vhk_log,
                            "Hotkeys: %s:%ld: found hotkey defined for action %d (%s),"
                            " clearing.",
                            textfile_reader_filename(&reader),
                            textfile_reader_linenum(&reader),
                            map->action, ui_action_get_name(map->action));
            }
            ui_action_map_clear_hotkey(map);
        } else {
            /* cannot use gtk_accelerator_name(): Gtk throws a fit about not having
             * a display and thus no GdkKeymap. :( */
#if 0

            char *accel_name = gtk_accelerator_name(keyval, mask);
#endif
            /* do it the hard way: copy argument without trailing crap */
            char   accel_name[256];
            size_t accel_len;

            accel_len = (size_t)(curpos - oldpos);
            if (accel_len >= sizeof(accel_name)) {
                accel_len = sizeof(accel_name) - 1;
            }
            memcpy(accel_name, oldpos, accel_len);
            accel_name[accel_len] = '\0';

            vhk_parser_warning("hotkey '%s' not found, ignoring.");
        }
    }
    return true;
}


/** \brief  Get identifier and return its token ID
 *
 * Parse \a text for an identifier in the form [a-zA-Z_][a-zA-Z0-9_]+
 *
 * \param[in]   text    text to parse
 * \param[in]   endptr  pointer to first non-identifier character in \a text
 *
 * \return  \c BEXPR_FALSE or \c BEXPR_TRUE, or \c BEXPR_INVALID (-1) on error
 */
static int get_identifier(const char *text, const char **endptr)
{
    const char *old;
    symbol_t   *node;

    old = text++;
    while (*text != '\0' && (isalnum((unsigned char)*text) || *text == '_')) {
        text++;
    }
    *endptr = text;
    if ((size_t)(text - old + 1) > sizeof identbuf) {
        /* identifier too long */
        vhk_parser_error("syntax error: identifier exceeds maximum length"
                         " of %" PRI_SIZE_T ".",
                         sizeof identbuf - 1u);
        return BEXPR_INVALID;
    }
    memcpy(identbuf, old, (size_t)(text - old));
    identbuf[text - old] = '\0';

    /* look up identifier to determine value */
    node = symbol_find(identbuf);
    if (node == NULL) {
        vhk_parser_error("syntax error: unknown identifier '%s'.", identbuf);
        return BEXPR_INVALID;
    }
    return node->value ? BEXPR_TRUE : BEXPR_FALSE;
}

/** \brief  Handler for the !IF keyword
 *
 * Parse \a line for condition and push condition on the if-stack. A condition
 * may consist of predefined identifiers (such as "C64", "GTK3" or "UNIX"),
 * boolean operators "!" (not), "||" (or) or "&&" (and). Parentheses are
 * supported as are the special identifiers "false" and "true".
 * Nesting of IF/ELSE is supported.
 *
 * See documentation (TODO) for a more thorough explanation.
 *
 * \param[in]   line    text after the keyword
 *
 * \return  \c true on success
 */
static bool vhk_parser_do_if(const char *line)
{
    const char *curpos;
    const char *endptr;
    bool        result;

    /* tokenize expression */
    bexpr_reset();
    curpos = line;
    while (*curpos != '\0') {
        int token;

        curpos = util_skip_whitespace(curpos);
        if (*curpos == '\0') {
            break;
        }

        if (isalpha((unsigned char)*curpos) || *curpos == '_') {
            /* might be identifier */
            token = get_identifier(curpos, &endptr);
            if (token < 0) {
                return false;
            }
        } else {
            token = bexpr_token_parse(curpos, &endptr);
            if (token < 0) {
                vhk_parser_error("parse error: %s.", bexpr_strerror(bexpr_errno));
                return false;
            }
        }
        bexpr_token_add(token);
        curpos = endptr;
    }

    /* evaluate expression */
    if (bexpr_evaluate(&result)) {
        ifstack_if(result);
        return true;
    }
    vhk_parser_error("syntax error: %s.", bexpr_strerror(bexpr_errno));
    return false;
}

/** \brief  Handler for the !ELSE keyword
 *
 * \param[in]   line    text after the keyword (ignored)
 *
 * \return  \c true on success
 */
static bool vhk_parser_do_else(const char *line)
{
    /* only whitespace or comment allowed after '!else' */
    if (!no_more_tokens(line)) {
        vhk_parser_error("syntax error: trailing tokens after !else.");
        return false;
    }

    if (ifstack_else()) {
        return true;
    } else {
        vhk_parser_error("syntax error: %s.", ifstack_strerror(ifstack_errno));
        return false;
    }
}

/** \brief  Handler for the !ENDIF keyword
 *
 * \param[in]   line    text after the keyword (ignored)
 *
 * \return  \c true on success
 */
static bool vhk_parser_do_endif(const char *line)
{
    /* only whitespace or comment allowed after '!endif' */
    if (!no_more_tokens(line)) {
        vhk_parser_error("syntax error: trailing tokens after !endif.");
        return false;
    }

    if (ifstack_endif()) {
        return true;
    } else {
        vhk_parser_error("syntax error: %s.", ifstack_strerror(ifstack_errno));
        return false;
    }
}

/** \brief  Handler for the !WARNING keyword
 *
 * Log warning on stdout using \a line as its text.
 *
 * \param[in]   line    text after the keyword
 *
 * \return  \c true on success
 */
static bool vhk_parser_do_warning(const char *line)
{
    if (ifstack_true()) {
        log_warning(vhk_log, "%s", util_skip_whitespace(line));
    }
    return true;
}

/** \brief  Handle a keyword
 *
 * \param[in]   line    text to parse, must start *after* the '!' indicator
 *
 * \return  bool
 *
 * \note    errors are reported with log_message()
 */
static bool vhk_parser_handle_keyword(const char *line)
{
    bool result = true;

    if (*line == '\0') {
        vhk_parser_error("syntax error: missing keyword after '!'.");
        result =  false;
    } else if (!isalpha((unsigned char)*line)) {
        vhk_parser_error("syntax error: illegal character after '!'.");
        result = false;
    } else {
        /* get keyword ID */
        const char *endptr = NULL;

        vhk_keyword_id_t id = vhk_parser_get_keyword_id(line, &endptr);
        switch (id) {
            case VHK_KW_ID_CLEAR:
                /* handle CLEAR */
                result = vhk_parser_do_clear(endptr);
                break;

            case VHK_KW_ID_DEBUG:
                /* handle DEBUG */
                result = vhk_parser_do_debug(endptr);
                break;

            case VHK_KW_ID_INCLUDE:
                /* handle INCLUDE */
                if (ifstack_true()) {
                    result = vhk_parser_do_include(endptr);
                }
                break;

            case VHK_KW_ID_UNDEF:
                /* handle UNDEF */
                result = vhk_parser_do_undef(endptr);
                break;

            case VHK_KW_ID_IF:
                /* always handle IF */
                result = vhk_parser_do_if(endptr);
                break;

            case VHK_KW_ID_ELSE:
                /* always handle ELSE */
                result = vhk_parser_do_else(endptr);
                break;

            case VHK_KW_ID_ENDIF:
                /* always handle ENDIF */
                result = vhk_parser_do_endif(endptr);
                break;

            case VHK_KW_ID_WARNING:
                /* show warning in log */
                result = vhk_parser_do_warning(endptr);
                break;

            default:
                /* unknown keyword */
                vhk_parser_error("syntax error: unknown keyword '!%s'.", line);
                result = false;
        }
    }
    return result;
}

/** \brief  Handle a hotkey mapping
 *
 * Parse \a line for a hotkey mapping, set hotkey on success.
 *
 * \param[in]   line    string to parse, expected to be trimmed
 *
 * \return  bool
 *
 * \note    errors are reported with log_message()
 */
static bool vhk_parser_handle_mapping(const char *line)
{
    ui_action_map_t *map;
    const char      *curpos;
    const char      *oldpos;
    char             action_name[256];
    ptrdiff_t        namelen;
    int              action_id    = ACTION_INVALID;
    uint32_t         vice_keysym  = 0;
    uint32_t         vice_modmask = 0;
    uint32_t         arch_keysym  = 0;
    uint32_t         arch_modmask = 0;

    curpos = oldpos = line;

    /* get action name */
    /* TODO: support quotes? */
    while (*curpos != '\0' && IS_ACTION_NAME_CHAR((int)*curpos)) {
        curpos++;
    }
    /* check for errors */
    if (curpos == oldpos) {
        /* no valid action name tokens found */
        vhk_parser_error("syntax error: missing action name.");
        return false;
    }
    if (curpos - oldpos >= sizeof(action_name)) {
        vhk_parser_error("syntax error: action name exceeds allowed"
                         " length of %" PRI_SIZE_T ".",
                         sizeof action_name - 1u);
        return false;
    }

    /* make properly nul-terminated string of action name for lookups*/
    namelen = curpos - oldpos;
    memcpy(action_name, oldpos, (size_t)namelen);
    action_name[namelen] = '\0';

    curpos = util_skip_whitespace(curpos);

    /* get combined modifier masks and keyval */
    if (!vhk_parser_get_keysym_and_modmask(curpos, &curpos,
                                           &vice_keysym, &vice_modmask,
                                           &arch_keysym, &arch_modmask)) {
        /* error already logged */
        return false;
    }

    if (vhk_debug) {
        log_message(vhk_log,
                    "Hotkeys: VICE keysym: %04x, VICE modmask: %08x, keyname: %s, action: %s",
                    vice_keysym, vice_modmask, vhk_keysym_name(vice_keysym), action_name);
    }

    /* finally try to register the hotkey */
    action_id = ui_action_get_id(action_name);
    if (action_id <= ACTION_NONE) {
        vhk_parser_warning("unknown action '%s', continuing anyway.", action_name);
        /* allow parsing to continue */
        return true;
    }

    if (ifstack_true()) {
        /* is the action valid for the current machine? */
        if (!ui_action_is_valid(action_id)) {
            vhk_parser_warning("invalid action '%s' for current emulator, continuing anyway.",
                               action_name);
            /* allow parsing to continue >*/
            return true;
        }

        /* register mapping, first try looking up the item */
        map = ui_action_map_set_hotkey(action_id,
                                       vice_keysym, vice_modmask,
                                       arch_keysym, arch_modmask);
        if (map != NULL) {
            /* set hotkey for menu item, if it exists */
            /* call arch-specific "virtual method" */
            debug_vhk("calling ui_hotkeys_install_by_map()");
            ui_hotkeys_install_by_map(map);
        } else {
            /* shouldn't get here */
            vhk_parser_error("invalid action '%s' for current emulator", action_name);
            return false;
        }
    }
    return true;
}


/** \brief  Parse hotkeys file
 *
 * \param[in]   path    path to hotkeys file
 *
 * \return  \c true on success
 */
bool vhk_parser_parse(const char *path)
{
    bool status = true;

    /* disable debugging by default */
    vhk_debug = false;
#if 0
    hotkeys_log_timestamp();
#endif

    /* initialize if stack */
    ifstack_reset();

    /* initialize file stack and open the file */
    textfile_reader_init(&reader);

    if (vhk_parser_open_file(path)) {
        /* main hotkeys file */

        while (true) {
            const char *line = textfile_reader_read(&reader);

            if (line != NULL) {
                bool  parse_ok = true;
                char *trimmed  = vhk_parser_strtrim(line);

                if (vhk_debug) {
                    log_debug("(%s) LINE %3ld: '%s'",
                              ifstack_true() ? "TRUE " : "FALSE",
                              textfile_reader_linenum(&reader),
                              line);
                }

                if (*trimmed == '\0'
                        || *trimmed == VHK_COMMENT
                        || *trimmed == VHK_COMMENT_ALT) {
                    /* empty line or comment, skip */
                    parse_ok = true;
                } else if (*trimmed == VHK_KEYWORD) {
                    /* found keyword, need to handle at least if/else/endif for
                     * correct conditional handling */
                    parse_ok = vhk_parser_handle_keyword(trimmed + 1);
                } else {
                    /* assume hotkey definition */
                    parse_ok = vhk_parser_handle_mapping(trimmed);
                }

                /* free trimmed line and check for parse result of line */
                lib_free(trimmed);
                if (!parse_ok) {
                    /* assume error already logged */
                    status = false;
                    goto cleanup;
                }
            } else {
                /* TODO: check if EOF or error */
                break;
            }
        }

        textfile_reader_close(&reader);

        /* check for unterminated IF */
        if (!ifstack_is_empty()) {
            /* cannot use vhk_parser_error() here, there's no open file anymore */
            log_error(vhk_log, "syntax error: unterminated !if at end of input.");
            status = false;
            goto cleanup;
        }

    } else {
        /* error already reported by vhk_parser_open_file() */
        status = false;
    }
cleanup:
    textfile_reader_free(&reader);
    return status;
}



/** \brief  List of predefined symbols known at compile time
 *
 * This doesn't include the machine names, since those need to be determined
 * at runtime :(
 */
static const predef_symbol_t predef_symbols[] = {
    /* useful for `#if 0` like constructs */
    {   "false",    false },
    {   "true",     true  },

    {   "GTK3",
#ifdef USE_GTK3UI
        true
#else
        false
#endif
    },
    {   "SDL1",
#ifdef USE_SDLUI
        true
#else
        false
#endif
    },
    {   "SDL2",
#ifdef USE_SDL2UI
        true
#else
        false
#endif
    },
    {   "HAVE_DEBUG",
#ifdef DEBUG
        true
#else
        false
#endif
    },
    {   "HAVE_FFMPEG",
#ifdef HAVE_FFMPEG
        true
#else
        false
#endif
    },
    {   "UNIX",
#if defined(UNIX_COMPILE) && !defined(MACOS_COMPILE)
        true
#else
        false
#endif
    },
    {   "MACOS",
#ifdef MACOS_COMPILE
        true
#else
        false
#endif
    },
    {   "WINDOWS",
#ifdef WINDOWS_COMPILE
        true
#else
        false
#endif
    }
};

/** \brief  Add compile time predefined symbols */
static void add_predef_symbols(void)
{
    size_t i = 0;

    for (i = 0; i < ARRAY_LEN(predef_symbols); i++) {
        if (!symbol_add(predef_symbols[i].name, predef_symbols[i].value)) {
            log_error(LOG_ERR,
                      "Predefined symbol '%s' already in table.",
                      predef_symbols[i].name);
            return;
        }
    }
}

/** \brief  Add runtime predefined symbols */
static void add_machine_symbols(void)
{
    symbol_add("C64",    (bool)(machine_class == VICE_MACHINE_C64));
    symbol_add("C64SC",  (bool)(machine_class == VICE_MACHINE_C64SC));
    symbol_add("SCPU64", (bool)(machine_class == VICE_MACHINE_SCPU64));
    symbol_add("C64DTV", (bool)(machine_class == VICE_MACHINE_C64DTV));
    symbol_add("C128",   (bool)(machine_class == VICE_MACHINE_C128));
    symbol_add("CBM5X0", (bool)(machine_class == VICE_MACHINE_CBM5x0));
    symbol_add("CBM6X0", (bool)(machine_class == VICE_MACHINE_CBM6x0));
    symbol_add("PET",    (bool)(machine_class == VICE_MACHINE_PET));
    symbol_add("PLUS4",  (bool)(machine_class == VICE_MACHINE_PLUS4));
    symbol_add("VIC20",  (bool)(machine_class == VICE_MACHINE_VIC20));
    symbol_add("VSID",   (bool)(machine_class == VICE_MACHINE_VSID));
}


/** \brief  Initialize parser for use
 *
 * Set up and fill the symbol table. This function should only be called once,
 * during general init, and *not* before each call to \c vkh_parser_parse().
 */
void vhk_parser_init(void)
{
    /* initialize symbol table and add symbols */
    symbol_table_init();
    add_predef_symbols();
    add_machine_symbols();
#ifdef DUMP_SYMTAB
    symbol_table_dump();
#endif

    /* initialize expression evaluator */
    bexpr_init();
    /* initialize if-stack */
    ifstack_init();
}


/** \brief  Clean up resources used by the parser
 *
 * Free symbol table. This function should only be called once, during general
 * shutdown, and *not* before each call to \c vkh_parser_parse().
 */
void vhk_parser_shutdown(void)
{
    symbol_table_free();
    bexpr_free();
    ifstack_free();
}
