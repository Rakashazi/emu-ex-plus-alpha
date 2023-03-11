/** \file   archdep_get_runtime_info.h
 * \brief   Get runtime information - header
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
 *
 */

#ifndef ARCHDEP_GET_RUNTIME_INFO_H
#define ARCHDEP_GET_RUNTIME_INFO_H

#include <stdbool.h>

/** \brief  Maximum size of a string in the archdep_runtime_info_t struct
 */
#define ARCHDEP_RUNTIME_STRMAX  1024


/** \brief  Object to store runtime info
 */
typedef struct archdep_runtime_info_s {
    char os_name[ARCHDEP_RUNTIME_STRMAX];       /**< OS name */
    char os_version[ARCHDEP_RUNTIME_STRMAX];    /**< OS version */
    char os_release[ARCHDEP_RUNTIME_STRMAX];    /**< OS release */
    char machine[ARCHDEP_RUNTIME_STRMAX];       /**< machine type */
} archdep_runtime_info_t;

bool archdep_get_runtime_info(archdep_runtime_info_t *info);

#endif

