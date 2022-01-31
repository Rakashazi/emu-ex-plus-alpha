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

typedef enum {
    UI_JAM_INVALID = -1, UI_JAM_RESET, UI_JAM_HARD_RESET, UI_JAM_MONITOR, UI_JAM_NONE
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
extern int ui_resources_init(void);
extern void ui_resources_shutdown(void);
extern int ui_cmdline_options_init(void);
extern void ui_init_with_args(int *argc, char **argv);
extern int ui_init_finish(void);
extern int ui_init_finalize(void);
extern void ui_shutdown(void);
extern int ui_init(void);

/* Print a message.  */
extern void ui_message(const char *format, ...);

/* Print an error message.  */
extern void ui_error(const char *format, ...);

/* Display a mesage without interrupting emulation */
extern void ui_display_statustext(const char *text, int fade_out);

/* Let the user browse for a filename; display format as a titel */
extern char* ui_get_file(const char *format, ...);

/* Drive related UI.  */
extern void ui_enable_drive_status(ui_drive_enable_t state,
                                   int *drive_led_color);
extern void ui_display_drive_track(unsigned int drive_number,
                                   unsigned int drive_base,
                                   unsigned int half_track_number,
                                   unsigned int disk_side);
/* The pwm value will vary between 0 and 1000.  */
extern void ui_display_drive_led(unsigned int drive_number, unsigned int drive_base, unsigned int led_pwm1, unsigned int led_pwm2);
extern void ui_display_drive_current_image(unsigned int unit_number, unsigned int drive_number, const char *image);
extern int ui_extend_image_dialog(void);

/* Tape related UI 
 *
 * The port argument is the index in the internal array of tape ports, so 0 or 1.
 */
extern void ui_set_tape_status(int port, int tape_status);
extern void ui_display_tape_motor_status(int port, int motor);
extern void ui_display_tape_control_status(int port, int control);
extern void ui_display_tape_counter(int port, int counter);
extern void ui_display_tape_current_image(int port, const char *image);

/* Show a CPU JAM dialog.  */
extern ui_jam_action_t ui_jam_dialog(const char *format, ...);

/* Reset */
extern void ui_display_reset(int device, int mode);

/* Recording UI */
extern void ui_display_playback(int playback_status, char *version);
extern void ui_display_recording(int recording_status);
extern void ui_display_event_time(unsigned int current, unsigned int total);

/* Joystick UI */
extern void ui_display_joyport(uint16_t *joyport);

/* Volume UI */
void ui_display_volume(int vol);

/* Hotkeys */
void ui_hotkeys_init(void);


#endif
