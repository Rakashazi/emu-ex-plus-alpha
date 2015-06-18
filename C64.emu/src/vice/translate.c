/*
 * translate.c - Global internationalization routines.
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
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"

#ifdef HAS_TRANSLATION
#include "intl.h"
#endif

#include "lib.h"
#include "log.h"
#include "resources.h"
#include "translate_languages.h"
#include "translate.h"
#include "util.h"

#define countof(array) (sizeof(array) / sizeof((array)[0]))

typedef struct translate_s {
    int resource_id;
    char *text;
} translate_t;

#ifdef HAS_TRANSLATION
char *current_language = NULL;
int current_language_index = 0;
#endif

/* GLOBAL STRING ID TEXT TABLE */

#include "translate_text.c"


/* --------------------------------------------------------------------- */

static char *get_string_by_id(int id)
{
    unsigned int k;

    for (k = 0; k < countof(string_table); k++) {
        if (string_table[k].resource_id == id) {
            return string_table[k].text;
        }
    }
    return NULL;
}

static char *sid_return = NULL;

/* special case translation, this command-line option normally
   produces alot of lines (which only differ slightly) for the
   translators to translate, this function builds up the total
   command-line options from smaller translation pieces.
 */
static char *translate_and_build_sid_cmdline_option(int en_resource)
{
    char *old, *new;

    /* check if sid_return is already built */
    if (sid_return != NULL) {
        lib_free(sid_return);
    }

    /* start building up the command-line */
    old = lib_stralloc("Specify SID engine and model (");

    /* add fast sid options */
    new = util_concat(old, translate_text(IDCLS_FASTSID_ENGINE_MODEL), NULL);
    lib_free(old);
    old = new;


#ifdef HAVE_RESID
    /* add resid options if available */
    if (en_resource != IDCLS_SPECIFY_SIDCART_ENGINE_MODEL) {
        new = util_concat(old, ", ", translate_text(IDCLS_RESID_ENGINE_MODEL), NULL);
        lib_free(old);
        old = new;
    }

    /* add residdtv options if available */
    if (en_resource == IDCLS_SPECIFY_SIDDTV_ENGINE_MODEL) {
        new = util_concat(old, ", ", translate_text(IDCLS_RESIDDTV_ENGINE_MODEL), NULL);
        lib_free(old);
        old = new;
    }
#endif

#ifdef HAVE_CATWEASELMKIII
    /* add catweasel options if available */
    new = util_concat(old, ", ", translate_text(IDCLS_CATWEASELMKIII_ENGINE_MODEL), NULL);
    lib_free(old);
    old = new;
#endif

#ifdef HAVE_HARDSID
    /* add hardsid options if available */
    new = util_concat(old, ", ", translate_text(IDCLS_HARDSID_ENGINE_MODEL), NULL);
    lib_free(old);
    old = new;
#endif

#ifdef HAVE_PARSID
    /* add parsid options if available */
    new = util_concat(old, ", ", translate_text(IDCLS_PARSID_ENGINE_MODEL), NULL);
    lib_free(old);
    old = new;
#endif

    /* add ending bracket */
    new = util_concat(old, ")", NULL);
    lib_free(old);

    sid_return = new;

    return sid_return;
}

#ifdef HAS_TRANSLATION
#include "translate_table.h"

static char *text_table[countof(translate_text_table)][countof(language_table)];

static void translate_text_init(void)
{
    unsigned int i, j;
    char *temp;

    for (i = 0; i < countof(language_table); i++) {
        for (j = 0; j < countof(translate_text_table); j++) {
            if (translate_text_table[j][i] == 0) {
                text_table[j][i] = NULL;
            } else {
                temp = get_string_by_id(translate_text_table[j][i]);
                text_table[j][i] = intl_convert_cp(temp, language_cp_table[i]);
            }
        }
    }
}

char translate_id_error_text[30];

char *translate_text(int en_resource)
{
    unsigned int i;
    char *retval = NULL;

    if (en_resource == IDCLS_UNUSED) {
        return NULL;
    }

    if (en_resource == 0) {
        log_error(LOG_DEFAULT, "TRANSLATE ERROR: ID 0 was requested.");
        return "ID 0 translate error";
    }

    /* handle sid cmdline special case translations */
    if (en_resource == IDCLS_SPECIFY_SIDCART_ENGINE_MODEL ||
        en_resource == IDCLS_SPECIFY_SID_ENGINE_MODEL ||
        en_resource == IDCLS_SPECIFY_SIDDTV_ENGINE_MODEL) {
        return translate_and_build_sid_cmdline_option(en_resource);
    }

    if (en_resource < 0x10000) {
        retval = intl_translate_text(en_resource);
    } else {
        for (i = 0; i < countof(translate_text_table); i++) {
            if (translate_text_table[i][0] == en_resource) {
                if (translate_text_table[i][current_language_index] != 0 &&
                    text_table[i][current_language_index] != NULL &&
                    strlen(text_table[i][current_language_index]) != 0) {
                    retval = text_table[i][current_language_index];
                } else {
                    retval = text_table[i][0];
                }
            }
        }
    }

    if (retval == NULL) {
        log_error(LOG_DEFAULT, "TRANSLATE ERROR: ID %d was requested, and would be returning NULL.", en_resource);
        sprintf(translate_id_error_text, "ID %d translate error", en_resource);
        retval = translate_id_error_text;
    }

    return retval;
}

int translate_res(int en_resource)
{
    return intl_translate_res(en_resource);
}

/* --------------------------------------------------------------------- */

static int set_current_language(const char *lang, void *param)
{
    unsigned int i;

    util_string_set(&current_language, "en");
    current_language_index = 0;

    if (strlen(lang) != 2) {
        return 0;
    }

    for (i = 0; i < countof(language_table); i++) {
        if (!strcasecmp(lang, language_table[i])) {
            current_language_index = i;
            util_string_set(&current_language, language_table[i]);
            intl_update_ui();
            return 0;
        }
    }

    return 0;
}

static const resource_string_t resources_string[] = {
    { "Language", "en", RES_EVENT_NO, NULL,
      &current_language, set_current_language, NULL },
    { NULL }
};

int translate_resources_init(void)
{
    intl_init();
    translate_text_init();

    return resources_register_string(resources_string);
}

static char *lang_list = NULL;

void translate_resources_shutdown(void)
{
    unsigned int i, j;

    for (i = 0; i < countof(language_table); i++) {
        for (j = 0; j < countof(translate_text_table); j++) {
            lib_free(text_table[j][i]);
        }
    }
    intl_shutdown();
    lib_free(current_language);

    /* check if sid_return is already built */
    if (sid_return != NULL) {
        lib_free(sid_return);
    }

    lib_free(lang_list);
}

static cmdline_option_t cmdline_options[] =
{
    { "-lang", SET_RESOURCE, 1,
      NULL, NULL, "Language", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_COMBO,
      IDCLS_P_ISO_LANGUAGE_CODE, IDCLS_SPECIFY_ISO_LANG_CODE,
      NULL, NULL },
    { NULL }
};

int translate_cmdline_options_init(void)
{
    char *temp_list = NULL;
    int i;

    lang_list = util_concat(". (", language_table[0], NULL);
    for (i = 1; i < countof(language_table); i++) {
        if (countof(language_table) == i + 1) {
            temp_list = util_concat(lang_list, "/", language_table[i], ")", NULL);
        } else {
            temp_list = util_concat(lang_list, "/", language_table[i], NULL);
        }
        lib_free(lang_list);
        lang_list = temp_list;
    }
    
    cmdline_options[0].description = lang_list;

    return cmdline_register_options(cmdline_options);
}

void translate_arch_language_init(void)
{
    char *lang;

    lang = intl_arch_language_init();
    set_current_language(lang, "");
}
#else

char *translate_text(int en_resource)
{
    if (en_resource == IDCLS_UNUSED) {
        return NULL;
    }

    if (en_resource == 0) {
        log_error(LOG_DEFAULT, "TRANSLATE ERROR: ID 0 was requested.");
        return "ID 0 translate error";
    }

    /* handle sid cmdline special case translations */
    if (en_resource == IDCLS_SPECIFY_SIDCART_ENGINE_MODEL ||
        en_resource == IDCLS_SPECIFY_SID_ENGINE_MODEL ||
        en_resource == IDCLS_SPECIFY_SIDDTV_ENGINE_MODEL) {
        return translate_and_build_sid_cmdline_option(en_resource);
    }

    return _(get_string_by_id(en_resource));
}
#endif
