/** \file   uihotkeys.h
 * \brief   UI-agnostic hotkeys - header
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

#ifndef VICE_HOTKEYS_UIHOTKEYS_H
#define VICE_HOTKEYS_UIHOTKEYS_H

#ifndef USE_HEADLESSUI

#include <stdbool.h>
#include <stdint.h>

#include "hotkeystypes.h"
#include "vhkkeysyms.h"
#include "uiactions.h"


void          ui_hotkeys_init    (const char *prefix);
void          ui_hotkeys_shutdown(void);
void          ui_hotkeys_remove_all(void);

void          ui_hotkeys_install_by_map  (ui_action_map_t *map);
void          ui_hotkeys_update_by_map   (ui_action_map_t *map,
                                          uint32_t         vice_keysym,
                                          uint32_t         vice_modmask);
void          ui_hotkeys_update_by_action(int      action,
                                          uint32_t vice_keysym,
                                          uint32_t vice_modmask);
void          ui_hotkeys_remove_by_map   (ui_action_map_t *map);
void          ui_hotkeys_remove_by_action(int action);

int           ui_hotkeys_resources_init(void);
int           ui_hotkeys_cmdline_options_init(void);
void          ui_hotkeys_set_default_requested(bool requested);

bool          ui_hotkeys_load(const char *path);
void          ui_hotkeys_load_vice_default(void);
bool          ui_hotkeys_load_user_default(void);
void          ui_hotkeys_reload(void);
bool          ui_hotkeys_save(void);
bool          ui_hotkeys_save_as(const char *path);

const char   *ui_hotkeys_vhk_filename_vice(void);
char         *ui_hotkeys_vhk_filename_user(void);
char         *ui_hotkeys_vhk_full_path_user(void);
char         *ui_hotkeys_vhk_full_path_vice(void);
vhk_source_t  ui_hotkeys_vhk_source_type(void);
char         *ui_hotkeys_vhk_source_path(void);

#endif  /* ifndef USE_HEADLESS_UI */
#endif
