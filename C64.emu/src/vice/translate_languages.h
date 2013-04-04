/*
 * translate_languages.h - Global language table.
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

#ifndef VICE_TRANSLATE_LANGUAGES_H
#define VICE_TRANSLATE_LANGUAGES_H

#ifdef HAS_TRANSLATION
static char *language_table[] = {
/* english */
    "en",

/* danish */
    "da",

/* german */
    "de",

/* spanish */
    "es",

/* french */
    "fr",

/* hungarian */
    "hu",

/* italian */
    "it",

/* korean */
    "ko",

/* dutch */
    "nl",

/* polish */
    "pl",

/* russian */
    "ru",

/* swedish */
    "sv",

/* turkish */
    "tr"
};

/* This is the codepage table, which holds the codepage
   used per language to encode the original text */
static int language_cp_table[] = {
/* english */
    28591,      /* ISO 8859-1 */

/* danish */
    28591,      /* ISO 8859-1 */

/* german */
    28591,      /* ISO 8859-1 */

/* spanish */
    28591,      /* ISO 8859-1 */

/* french */
    28591,      /* ISO 8859-1 */

/* hungarian */
    28592,      /* ISO 8859-2 */

/* italian */
    28591,      /* ISO 8859-1 */

/* italian */
    949,        /* CP 949 */

/* dutch */
    28591,      /* ISO 8859-1 */

/* polish */
    28592,      /* ISO 8859-2 */

/* russian */
    28595,      /* ISO 8859-5 */

/* swedish */
    28591,      /* ISO 8859-1 */

/* turkish */
    28599       /* ISO 8859-9 */
};
#endif

#endif
