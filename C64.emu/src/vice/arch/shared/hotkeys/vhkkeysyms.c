/** \file   vhkkeysyms.h
 * \brief   UI-agnostic key symbols and names
 *
 * List of keysym identifiers used by the hotkeys in VICE.
 * Each UI toolkit has its own key symbols and names, so we need a way to refer
 * to keys that isn't toolkit-specific.
 *
 * The keysyms provided here are taken from `/usr/include/X11/keysymdef.h`.
 *
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lib.h"
#include "hotkeystypes.h"

#include "vhkkeysyms.h"


#define ARR_ELEMENTS(arr)   (sizeof arr / sizeof arr[0])


/** \brief  Mapping of key name to value */
typedef struct vhk_key_s {
    const char *name;   /**< name, X11 macro name without the lead 'XK_' */
    uint32_t    sym;    /**< value, X11 keysym value */
} vhk_key_t;


/** \brief  List of UI-agnostic modifier keys */
static const vhk_modifier_t vhk_modifier_list[] = {
    { VHK_MOD_ALT,      "Alt",      "Alt" },
    { VHK_MOD_COMMAND,  "Command",  "Command \u2318" },
    { VHK_MOD_CONTROL,  "Control",  "Control \u2303" },
    { VHK_MOD_HYPER,    "Hyper",    "Hyper" },
    { VHK_MOD_META,     "Meta",     "Meta" },
    { VHK_MOD_OPTION,   "Option",   "Option \u2325" },
    { VHK_MOD_SHIFT,    "Shift",    "Shift \u21e7" },
    { VHK_MOD_SUPER,    "Super",    "Super" }
};



/* include generated keysyms table: static vhk_key_t keys[] */
#include "keysymtable.h"


/** \brief  Comparison function for bsearch(3)
 *
 * Compare the \c name members of key table elements \a p1 and \a p1 with
 * strcmp(3).
 *
 * \param   p1  pointer to first element
 * \param   p2  pointer to second element
 *
 * \return  \< 0 if \a p1 \< \a p2, 0 if \a p1 == \a p2, \> 0 if \a p1 \> \a p2
 */
static int compar(const void *p1, const void *p2)
{
    const vhk_key_t *k1 = p1;
    const vhk_key_t *k2 = p2;

    return strcmp(k1->name, k2->name);
}


/** \brief  Get keysym value from name
 *
 * \param[in]   name    keysym name
 *
 * \return  keysym value as defined in `vhkkeysyms.h`, or 0 when not found
 */
uint32_t vhk_keysym_from_name(const char *name)
{
    void            *result;
    const vhk_key_t  key = { .name = name };

    result = bsearch((const void *)&key,
                     (const void *)keys,
                      sizeof keys / sizeof keys[0],
                      sizeof keys[0],
                      compar);
    if (result == NULL) {
        return 0;
    }
    return ((const vhk_key_t *)result)->sym;
}


/** \brief  Get keysym name from value
 *
 * \param   keysym  keysym value
 *
 * \return  name or `NULL` when \a keysym isn't valid
 */
const char *vhk_keysym_name(uint32_t keysym)
{
    size_t i;

    for (i = 0; i < sizeof keys / sizeof keys[0]; i++) {
        if (keys[i].sym == keysym) {
            return keys[i].name;
        }
    }
    return NULL;
}


/** \brief  Parse string for modifier name and return modifier bit
 *
 * Parse \a name for a valid modifier name and return its value, storing the
 * position in \a name after either end-of-string or '\>' in \a endptr.
 * The \a name is allowed to start with a '\>' opening tag.
 *
 * \param[in]   name    string to parse
 * \param[out]  endptr  position in \a name of the closing '>' tag or the nul
 *                      character (optional)
 *
 * \return  VICE modifier value
 */
uint32_t vhk_modifier_from_name(const char *name, const char **endptr)
{
    size_t i;

    if (name == NULL || *name == '\0') {
        if (endptr != NULL) {
            *endptr = name;
        }
        return VHK_MOD_NONE;
    }

    /* skip opening '<' if present */
    if (*name == VHK_MODIFIER_OPEN) {
        name++;
    }

    for (i = 0; i < sizeof vhk_modifier_list / sizeof vhk_modifier_list[0]; i++) {
        const vhk_modifier_t *m;
        const char           *s;
        int                   k;

        m = &(vhk_modifier_list[i]);
        s = m->name;
        k = 0;

        while (s[k] != '\0'
                && name[k] != '\0' && name[k] != VHK_MODIFIER_CLOSE
                && tolower((unsigned char)name[k]) == tolower((unsigned char)s[k])) {
            k++;
        }
        if (s[k] == '\0' && (name[k] == '\0' || name[k] == VHK_MODIFIER_CLOSE)) {
            /* match */
            if (endptr != NULL) {
                *endptr = name + k;
            }
            return m->mask;
        }
    }
    return VHK_MOD_NONE;
}


/** \brief  Get string of modifier names for a modifier mask
 *
 * Generate string with modifier names enclosed by angled brackets from modifier
 * mask \a vice_modmask.
 *
 * \param[in]   vice_modmask    VICE modifier mask
 *
 * \return  heap-allocated string, free with `lib_free()`
 *
 * \note    Always returns an allocated string, being the empty string for a
 *          modifier mask of 0 (`VHK_MOD_NONE`).
 */
char *vhk_modmask_name(uint32_t vice_modmask)
{
    const vhk_modifier_t *mod;
    char                 *name;
    char                 *curpos;
    size_t                len;
    size_t                i;

    if (vice_modmask == VHK_MOD_NONE) {
        /* return empty string */
        name = lib_malloc(1u);
        *name = '\0';
        return name;
    }

    /* allocate result string */
    len = 0;
    for (i = 0; i < sizeof vhk_modifier_list / sizeof vhk_modifier_list[0]; i++) {
        mod = &vhk_modifier_list[i];
        if (vice_modmask & mod->mask) {
            len += strlen(mod->name) + 2u;  /* +2 for '<' and '>' */
        }
    }
    curpos = name = lib_malloc(len + 1u);

    /* add modifier names */
    for (i = 0; i < sizeof vhk_modifier_list / sizeof vhk_modifier_list[0]; i++) {
        mod = &vhk_modifier_list[i];
        if (vice_modmask & mod->mask) {
            /* add "<modname>" */
            len = strlen(mod->name);
            *curpos++ = VHK_MODIFIER_OPEN;
            memcpy(curpos, mod->name, len);
            curpos += len;
            *curpos++ = VHK_MODIFIER_CLOSE;
        }
    }
    *curpos = '\0';
    return name;
}


/** \brief  Get modifier name
 *
 * Get a string representing a keyboard modifier. The returned string is the
 * unadorned name, so no '\<' or '\>' around the name like in the hotkeys files.
 *
 * \param[in]   vice_modifier   VICE modifier
 *
 * \return  modifier name
 */
const char *vhk_modifier_name(uint32_t vice_modifier)
{
    size_t m;

    if (vice_modifier == 0) {
        return NULL;
    }
    for (m = 0 ; ARR_ELEMENTS(vhk_modifier_list); m++) {
        if (vice_modifier == vhk_modifier_list[m].mask) {
            return vhk_modifier_list[m].name;
        }
    }
    return NULL;
}


/** \brief  Order in which modifier names should appear in hotkey labels
 *
 */
static const uint32_t mod_order[] = {
    VHK_MOD_CONTROL,
    VHK_MOD_ALT,
    VHK_MOD_OPTION,
    VHK_MOD_SHIFT,
    VHK_MOD_COMMAND,
    VHK_MOD_SUPER,
    VHK_MOD_META,
    VHK_MOD_HYPER
};


/** \brief  Get accelerator label for a hotkey
 *
 * Create a string to present a hotkey's key name and modifier mask to the user.
 * This function is very much like \c gtk_accelerator_get_label() and meant for
 * UI toolkits that do not provide such a function, such as SDL.
 *
 * \param[in]   vice_keysym     VICE keysym
 * \param[in]   vice_modmask    VICE modifier mask
 *
 * \return  heap-allocated string, free with \c lib_free()
 *
 * \note    returns \c NULL for invalid \a vice_keysym values
 */
char *vhk_hotkey_label(uint32_t vice_keysym, uint32_t vice_modmask)
{
    const char *modname;
    size_t      modlen;
    const char *keyname;
    size_t      namelen;
    char       *buffer;
    char       *bufpos;
    size_t      bufsize;
    size_t      i;

    /* get keyname and exit early if invalid */
    keyname = vhk_keysym_name(vice_keysym);
    if (keyname == NULL) {
        return NULL;
    }
    /* check if we have any modifiers at all */
    if (vice_modmask == 0) {
        /* no modifier, just return the key name */
        return lib_strdup(keyname);
    }
    namelen = strlen(keyname);

    /* determine size of buffer to allocate for label */
    bufsize = 0;
    for (i = 0; i < ARR_ELEMENTS(mod_order); i++) {
        if (vice_modmask & mod_order[i]) {
            modname = vhk_modifier_name(mod_order[i]);
            if (modname != NULL) {
                bufsize += strlen(modname) + 1u; /* +1 for trailing '+' */
            }
        }
    }
    bufsize += namelen + 1; /* +1 for terminating '\0' */

    /* allocate buffer for label */
    bufpos = buffer = lib_malloc(bufsize);

    /* iterate modifiers again, adding them to the buffer */
    for (i = 0; i < ARR_ELEMENTS(mod_order); i++) {
        if (vice_modmask & mod_order[i]) {
            modname = vhk_modifier_name(mod_order[i]);
            if (modname != NULL) {
                modlen = strlen(modname);
                strncpy(bufpos, modname, bufsize - (size_t)(bufpos - buffer));
                bufpos += modlen;
                *bufpos++ = '+';
            }
        }
    }
    /* add key name plus terminating \0' character */
    memcpy(bufpos, keyname, namelen + 1u);

    return buffer;
}
