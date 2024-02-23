/*
 * datasette.h - CBM cassette implementation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#ifndef VICE_DATASETTE_H
#define VICE_DATASETTE_H

#include "types.h"

enum {
    DATASETTE_CONTROL_STOP = 0,
    DATASETTE_CONTROL_START,
    DATASETTE_CONTROL_FORWARD,
    DATASETTE_CONTROL_REWIND,
    DATASETTE_CONTROL_RECORD,
    DATASETTE_CONTROL_RESET,
    DATASETTE_CONTROL_RESET_COUNTER
};

/* Counter is c=g*(sqrt(v*t/d*pi+r^2/d^2)-r/d)
   Some constants for the Datasette-Counter, maybe resourses in future */
#ifndef PI
#define PI          3.1415926535
#endif

#define DS_D        1.27e-5
#define DS_R        1.07e-2
#define DS_V_PLAY   4.76e-2
#define DS_G        0.525

/* at FF/REWIND, Datasette-counter makes ~4 rounds per second */
#define DS_RPS_FAST 4.00

#define TAP_ZERO_GAP_DELAY_MAX          5000
#define TAP_ZERO_GAP_DELAY_DEFAULT      2500

#define TAP_SPEED_TUNING_ONE            100000
#define TAP_SPEED_TUNING_MAX            (TAP_SPEED_TUNING_ONE / 2)  /* +/-50% */
#define TAP_SPEED_TUNING_DEFAULT        0

#define TAP_WOBBLE_FREQ_ONE             1000
#define TAP_WOBBLE_FREQ_MAX             (TAP_WOBBLE_FREQ_ONE * 50) /* 50Hz */
#define TAP_WOBBLE_FREQ_DEFAULT         (TAP_WOBBLE_FREQ_ONE * 3) /* 3Hz */

#define TAP_WOBBLE_AMPLITUDE_ONE        100000
#define TAP_WOBBLE_AMPLITUDE_MAX        (TAP_WOBBLE_AMPLITUDE_ONE / 2)  /* +/-50% */
#define TAP_WOBBLE_AMPLITUDE_DEFAULT    (TAP_WOBBLE_AMPLITUDE_ONE / 200)  /* +/-0.5% */

#define TAP_AZIMUTH_ERROR_ONE           1000
#define TAP_AZIMUTH_ERROR_MAX           (TAP_AZIMUTH_ERROR_ONE * 10) /* +/- 10 cycles */
#define TAP_AZIMUTH_ERROR_DEFAULT       0

#define TAPE_SOUND_VOLUME_ONE           2048
#define TAPE_SOUND_VOLUME_MAX           4096
#define TAPE_SOUND_VOLUME_DEFAULT       1024

struct tap_s;

extern int datasette_sound_emulation;
extern int datasette_sound_emulation_volume;

void datasette_init(void);
void datasette_set_tape_image(int port, struct tap_s *image);
void datasette_control(int port, int command);
void datasette_reset(void);
void datasette_reset_counter(int port);
void datasette_event_playback_port1(CLOCK offset, void *data);
void datasette_event_playback_port2(CLOCK offset, void *data);

/* Emulator specific functions.  */
void machine_trigger_flux_change(int port, unsigned int on);
void machine_set_tape_sense(int port, int sense);
void machine_set_tape_write_in(int port, int val);
void machine_set_tape_motor_in(int port, int val);

void datasette_set_tape_sense(int port, int sense);

/* For registering the resources.  */
int datasette_resources_init(int amount);
int datasette_cmdline_options_init(void);

#endif
