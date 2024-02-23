/*
 * uiapi.h - Common user interface API.
 *
 * Written by
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

/* Do not include this header file, include `ui.h' instead.  */

#ifndef VICE_UIAPI
#define VICE_UIAPI

#include "types.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    UI_JAM_INVALID = -1,    /**< error */
    UI_JAM_RESET_CPU,       /**< reset machine/drivee CPU */
    UI_JAM_POWER_CYCLE,     /**< power cycle macine/drive */
    UI_JAM_MONITOR,         /**< break into the monitor */
    UI_JAM_NONE             /**< don't do anything */
} ui_jam_action_t;

typedef enum {
    UI_EXTEND_IMAGE_INVALID = -1, UI_EXTEND_IMAGE_NEVER, UI_EXTEND_IMAGE_ALWAYS
} ui_extendimage_action_t;

typedef enum {
    UI_DRIVE_ENABLE_NONE = 0,
    UI_DRIVE_ENABLE_0 = 1 << 0,
    UI_DRIVE_ENABLE_1 = 1 << 1,
    UI_DRIVE_ENABLE_2 = 1 << 2,
    UI_DRIVE_ENABLE_3 = 1 << 3
} ui_drive_enable_t;

/* Initialization  */
int ui_resources_init(void);
void ui_resources_shutdown(void);
int ui_cmdline_options_init(void);
void ui_init_with_args(int *argc, char **argv);
int ui_init_finalize(void);
void ui_shutdown(void);
int ui_init(void);

/* Print a message.  */
void ui_message(const char *format, ...) VICE_ATTR_PRINTF;

/* Print an error message.  */
void ui_error(const char *format, ...) VICE_ATTR_PRINTF;

/* Display a mesage without interrupting emulation */
void ui_display_statustext(const char *text, int fade_out);

/* Let the user browse for a filename; display format as a titel */
char* ui_get_file(const char *format, ...) VICE_ATTR_PRINTF;

/* Drive related UI.  */
void ui_enable_drive_status(ui_drive_enable_t state, int *drive_led_color);
void ui_display_drive_track(unsigned int drive_number, unsigned int drive_base, unsigned int half_track_number, unsigned int disk_side);

/* The pwm value will vary between 0 and 1000.  */
void ui_display_drive_led(unsigned int drive_number, unsigned int drive_base, unsigned int led_pwm1, unsigned int led_pwm2);
void ui_display_drive_current_image(unsigned int unit_number, unsigned int drive_number, const char *image);
int ui_extend_image_dialog(void);

/* Tape related UI
 *
 * The port argument is the index in the internal array of tape ports, so 0 or 1.
 */
void ui_set_tape_status(int port, int tape_status);
void ui_display_tape_motor_status(int port, int motor);
void ui_display_tape_control_status(int port, int control);
void ui_display_tape_counter(int port, int counter);
void ui_display_tape_current_image(int port, const char *image);

/* Show a CPU JAM dialog.  */
ui_jam_action_t ui_jam_dialog(const char *format, ...) VICE_ATTR_PRINTF;

/* Reset */
void ui_display_reset(int device, int mode);

/* Recording UI */
void ui_display_playback(int playback_status, char *version);
void ui_display_recording(int recording_status);
void ui_display_event_time(unsigned int current, unsigned int total);

/* Joystick UI */
void ui_display_joyport(uint16_t *joyport);
void arch_ui_activate(void);

/* Volume UI */
void ui_display_volume(int vol);



/* Hotkeys
 *
 * The following are functions that are required to be implemented by an arch
 * and/or UI using the hotkeys API in src/arch/shared/hotkeys/.
 */

#include "arch/shared/uiactions.h"

/** \brief  Run UI-specific hotkeys initialization code
 *
 * This function is called after the generic hotkeys code has initialized and
 * allows UIs to do their own additional initialization.
 * For example, Gtk3 will set up its `GtkAccelGroup` here.
 */
void ui_hotkeys_arch_init(void);

/** \brief  Run UI-specific hotkeys shutdown code
 *
 * This function is called in `ui_hotkeys_shutdown()` before any other cleanup
 * happens, allowing UIs to do their own additional cleanup if required.
 */
void ui_hotkeys_arch_shutdown(void);

/** \brief  Install hotkey
 *
 * Run UI-specific code to register the hotkey in \a map.
 * The \a map contains a UI action ID, a VICE keysym and a VICE modifier mask,
 * the UI needs to register the hotkey using its API and, if present, set the
 * menu item pointer(s) in \a map.
 *
 * \param[in]   map UI action map object
 */
void ui_hotkeys_arch_install_by_map(ui_action_map_t *map);

/** \brief  Update hotkey
 *
 * The \a map should contain a valid action ID and the keysym and modmask of
 * the currently registered hotkey for that action. The function is expected
 * to remove the old hotkey from its menu items, if any, set the hotkey for
 * the action to \a vice_keysym + \a vice_modmask, and if menu items are present
 * update their accelator labels to the new hotkey. The generic hotkeys code
 * takes care of updating the internals of \a map.
 *
 * \param[in]   map             UI action map object
 * \param[in]   vice_keysym     new VICE keysym
 * \param[in]   vice_modmask    new VICE modifier mask
 */
void ui_hotkeys_arch_update_by_map(ui_action_map_t *map,
                                   uint32_t         vice_keysym,
                                   uint32_t         vice_modmask);

/** \brief  Remove hotkey
 *
 * Remove hotkey from the UI. The \a map contains the keysym and modifier mask
 * of the hotkey to remove. The shared hotkeys code will take care of removing
 * the keysyms/modifier masks from \a map, the arch-specific code is expected
 * to remove any accelerator (labels) from the UI and disconnecting any
 * signal handlers connected for the hotkey.
 *
 * \param[in]   map     UI action map object
 */
void ui_hotkeys_arch_remove_by_map(ui_action_map_t *map);

/* Functions translating between VICE and arch keysysm and modifiers
 *
 * TODO: Decide if we need to have both functions for single modifiers and
 *       modifier masks (combined modifiers).
 */

/** \brief  Translate arch keysym to VICE keysysm
 *
 * \param[in]   arch_keysym arch keysym
 *
 * \return  VICE keysym
 */
uint32_t ui_hotkeys_arch_keysym_from_arch  (uint32_t arch_keysym);

/** \brief  Translate VICE keysym to arch keysysm
 *
 * \param[in]   vice_keysym VICE keysym
 *
 * \return  arch keysym
 */
uint32_t ui_hotkeys_arch_keysym_to_arch    (uint32_t vice_keysym);

/** \brief  Translate arch modifier to VICE modifier
 *
 * \param[in]   arch_mod    arch modifier
 *
 * \return  VICE modifier
 */
uint32_t ui_hotkeys_arch_modifier_from_arch(uint32_t arch_mod);

/** \brief  Translate VICE modifier to arch modifier
 *
 * \param[in]   vice_mod    VICE modifier
 *
 * \return  arch modifier
 */
uint32_t ui_hotkeys_arch_modifier_to_arch  (uint32_t vice_mod);

/** \brief  Translate arch modifier mask to VICE modifier mask
 *
 * \param[in]   arch_modmask    arch modifier mask
 *
 * \return  VICE modifier mask
 */
uint32_t ui_hotkeys_arch_modmask_from_arch (uint32_t arch_modmask);

/** \brief  Translate VICE modifier mask to arch modifier
 *
 * \param[in]   vice_modmask    VICE modifier mask
 *
 * \return  arch modifier mask
 */
uint32_t ui_hotkeys_arch_modmask_to_arch   (uint32_t vice_modmask);

#endif
