/** \file   archapi.h
 * \brief   Common system-specific API.
 *
 * \author  Andreas Boose <viceteam@t-online.de>
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
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

/* Do not include this header file, include `archdep.h' instead.  */
#ifndef VICE_ARCHAPI_PRIVATE_API
#error "Do not include this header file, include `archdep.h' instead."
#endif

#ifndef VICE_ARCHAPI
#define VICE_ARCHAPI

#include "vice.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "archdep_defs.h"
#include "archdep_access.h"
#include "archdep_boot_path.h"
#include "archdep_cbmfont.h"
#include "archdep_chdir.h"
#include "archdep_close.h"
#include "archdep_create_user_cache_dir.h"
#include "archdep_create_user_config_dir.h"
#include "archdep_create_user_state_dir.h"
#include "archdep_current_dir.h"
#include "archdep_default_autostart_disk_image_file_name.h"
#include "archdep_default_fliplist_file_name.h"
#include "archdep_default_joymap_file_name.h"
#include "archdep_default_logfile.h"
#include "archdep_default_logger.h"
#include "archdep_default_portable_resource_file_name.h"
#include "archdep_default_resource_file_name.h"
#include "archdep_default_rtc_file_name.h"
#include "archdep_default_sysfile_pathlist.h"
#include "archdep_dir.h"
#include "archdep_ethernet_available.h"
#include "archdep_exit.h"
#include "archdep_expand_path.h"
#include "archdep_extra_title_text.h"
#include "archdep_fdopen.h"
#include "archdep_file_exists.h"
#include "archdep_file_is_blockdev.h"
#include "archdep_file_is_chardev.h"
#include "archdep_file_size.h"
#include "archdep_filename_parameter.h"
#include "archdep_fix_permissions.h"
#include "archdep_fix_streams.h"
#include "archdep_fseeko.h"
#include "archdep_ftello.h"
#include "archdep_get_current_drive.h"
#include "archdep_get_runtime_info.h"
#include "archdep_get_vice_datadir.h"
#include "archdep_get_vice_docsdir.h"
#include "archdep_get_vice_drivesdir.h"
#include "archdep_get_vice_hotkeysdir.h"
#include "archdep_get_vice_machinedir.h"
#include "archdep_getcwd.h"
#include "archdep_home_path.h"
#include "archdep_icon_path.h"
#include "archdep_is_haiku.h"
#include "archdep_is_macos_bindist.h"
#include "archdep_is_windows_nt.h"
#include "archdep_kbd_get_host_mapping.h"
#include "archdep_list_drives.h"
#include "archdep_make_backup_filename.h"
#include "archdep_mkdir.h"
#include "archdep_mkstemp_fd.h"
#include "archdep_network.h"
#include "archdep_open_default_log_file.h"
#include "archdep_path_is_relative.h"
#include "archdep_program_name.h"
#include "archdep_program_path.h"
#include "archdep_quote_parameter.h"
#include "archdep_quote_unzip.h"
#include "archdep_rawnet_capability.h"
#include "archdep_real_path.h"
#include "archdep_remove.h"
#include "archdep_rename.h"
#include "archdep_require_vkbd.h"
#include "archdep_rmdir.h"
#include "archdep_rtc_get_centisecond.h"
#include "archdep_sanitize_filename.h"
#include "archdep_set_current_drive.h"
#include "archdep_set_openmp_wait_policy.h"
#include "archdep_signals.h"
#include "archdep_socketpeek.h"
#include "archdep_sound.h"
#include "archdep_spawn.h"
#include "archdep_startup_log_error.h"
#include "archdep_stat.h"
#include "archdep_tick.h"
#include "archdep_tmpnam.h"
#include "archdep_user_cache_path.h"
#include "archdep_user_config_path.h"
#include "archdep_user_state_path.h"
#include "archdep_usleep.h"
#include "archdep_xdg.h"

/* Program start.  */
int archdep_init(int *argc, char **argv);

/* Free everything on exit. (in arch/../archdep.c)  */
void archdep_shutdown(void);

#endif
