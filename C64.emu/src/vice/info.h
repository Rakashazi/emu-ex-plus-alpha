/*
 * info.h - Info about the VICE project, including the GPL.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#ifndef VICE_INFO_H
#define VICE_INFO_H

#ifdef WINMIPS
extern const char *info_license_text[];
extern const char *info_contrib_text[];
#else
extern const char info_license_text[];
extern const char info_contrib_text[];
#endif

extern const char info_warranty_text[];

#if defined(USE_SDLUI) || defined(USE_SDLUI2)

#ifdef WINMIPS
extern const char *info_license_text40[];
#else
extern const char info_license_text40[];
#endif

extern const char info_warranty_text40[];
#endif

typedef struct vice_team_s {
    char *years;
    char *name;
    char *emailname;
} vice_team_t;

typedef struct vice_trans_s {
    char *years;
    char *name;
    char *language;
    char *emailname;
} vice_trans_t;

extern vice_team_t core_team[];
extern vice_team_t ex_team[];
extern char *doc_team[];
extern vice_trans_t trans_team[];

#endif
