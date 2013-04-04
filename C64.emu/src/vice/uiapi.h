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
    UI_JAM_RESET, UI_JAM_HARD_RESET, UI_JAM_MONITOR, UI_JAM_NONE
} ui_jam_action_t;

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
extern int ui_init(int *argc, char **argv);
extern int ui_init_finish(void);
extern int ui_init_finalize(void);
extern void ui_shutdown(void);

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
                                   unsigned int half_track_number);
/* The pwm value will vary between 0 and 1000.  */
extern void ui_display_drive_led(int drive_number, unsigned int pwm1, unsigned int led_pwm2);
extern void ui_display_drive_current_image(unsigned int drive_number, const char *image);
extern int ui_extend_image_dialog(void);

/* Tape related UI */
extern void ui_set_tape_status(int tape_status);
extern void ui_display_tape_motor_status(int motor);
extern void ui_display_tape_control_status(int control);
extern void ui_display_tape_counter(int counter);
extern void ui_display_tape_current_image(const char *image);

/* Show a CPU JAM dialog.  */
extern ui_jam_action_t ui_jam_dialog(const char *format, ...);

/* Update all menu entries.  */
extern void ui_update_menus(void);

/* Recording UI */
extern void ui_display_playback(int playback_status, char *version);
extern void ui_display_recording(int recording_status);
extern void ui_display_event_time(unsigned int current, unsigned int total);

/* Joystick UI */
extern void ui_display_joyport(BYTE *joyport);

/* Volume UI */
void ui_display_volume(int vol);

/* Event related UI. */
extern void ui_dispatch_next_event(void);

#endif
