/*
 * datasette.h - CBM cassette implementation.
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

#ifndef VICE_DATASETTE_H
#define VICE_DATASETTE_H

#include "types.h"

#define DATASETTE_CONTROL_STOP    0
#define DATASETTE_CONTROL_START   1
#define DATASETTE_CONTROL_FORWARD 2
#define DATASETTE_CONTROL_REWIND  3
#define DATASETTE_CONTROL_RECORD  4
#define DATASETTE_CONTROL_RESET   5
#define DATASETTE_CONTROL_RESET_COUNTER   6

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

struct snapshot_s;
struct tap_s;

extern void datasette_init(void);
extern void datasette_set_tape_image(struct tap_s *image);
extern void datasette_control(int command);
extern void datasette_set_motor(int flag);
extern void datasette_toggle_write_bit(int write_bit);
extern void datasette_reset(void);
extern void datasette_reset_counter(void);
extern void datasette_event_playback(CLOCK offset, void *data);

/* Emulator specific functions.  */
extern void datasette_trigger_flux_change(unsigned int on);
extern void datasette_set_tape_sense(int sense);

/* For registering the resources.  */
extern int datasette_resources_init(void);
extern int datasette_cmdline_options_init(void);

extern int datasette_write_snapshot(struct snapshot_s *s);
extern int datasette_read_snapshot(struct snapshot_s *s);
#endif
