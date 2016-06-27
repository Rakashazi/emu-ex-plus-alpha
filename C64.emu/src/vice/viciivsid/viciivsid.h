/*
 * viciivsid.h - A cycle-exact event-driven MOS6569 (VIC-II) emulation.
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

#ifndef VICE_VICII_H
#define VICE_VICII_H

#include "types.h"

struct canvas_refresh_s;
struct machine_timing_s;
struct raster_s;
struct screenshot_s;
struct snapshot_s;

#define VICII_STANDARD 0

/* VICII border mode defines */
#define VICII_NORMAL_BORDERS 0
#define VICII_BORDER_MODE(v) (v << 12)

/* VICII model defines (for viciisc/) FIXME enum instead? */
/* PAL, 63 cycle, 9 luma, "old" */
#define VICII_MODEL_6569    0
/* PAL, 63 cycle, 9 luma, "new" */
#define VICII_MODEL_8565    1
/* PAL, 63 cycle, 5 luma, "old" */
#define VICII_MODEL_6569R1  2
/* NTSC, 65 cycle, 9 luma, "old" */
#define VICII_MODEL_6567    3
/* NTSC, 65 cycle, 9 luma, "new" */
#define VICII_MODEL_8562    4
/* NTSC, 64 cycle, ? luma, "old" */
#define VICII_MODEL_6567R56A 5
/* PAL-N, 65 cycle, ? luma, "?" */
#define VICII_MODEL_6572    6
#define VICII_MODEL_NUM 7

extern int vicii_resources_init(void);
extern int vicii_cmdline_options_init(void);
extern struct raster_s *vicii_init(unsigned int flag);
extern struct video_canvas_s *vicii_get_canvas(void);

extern void vicii_reset(void);
extern void vicii_set_light_pen(CLOCK mclk, int state);
extern void vicii_trigger_light_pen(CLOCK mclk);
extern CLOCK vicii_lightpen_timing(int x, int y);
extern void vicii_set_vbank(int new_vbank);
extern void vicii_set_ram_base(BYTE *base);
extern void vicii_powerup(void);
extern void vicii_set_canvas_refresh(int enable);
extern void vicii_reset_registers(void);
extern void vicii_update_memory_ptrs_external(void);
extern void vicii_handle_pending_alarms_external(int num_write_cycles);
extern void vicii_handle_pending_alarms_external_write(void);

extern void vicii_screenshot(struct screenshot_s *screenshot);
extern void vicii_shutdown(void);
extern void vicii_change_timing(struct machine_timing_s *machine_timing);

extern int vicii_dump(void);

extern void vicii_snapshot_prepare(void);
extern int vicii_snapshot_write_module(struct snapshot_s *s);
extern int vicii_snapshot_read_module(struct snapshot_s *s);

extern void vicii_async_refresh(struct canvas_refresh_s *r);

extern void vicii_set_phi1_vbank(int num_vbank);
extern void vicii_set_phi2_vbank(int num_vbank);
extern void vicii_set_phi1_ram_base(BYTE *base);
extern void vicii_set_phi2_ram_base(BYTE *base);
extern void vicii_set_phi1_addr_options(WORD mask, WORD offset);
extern void vicii_set_phi2_addr_options(WORD mask, WORD offset);
extern void vicii_set_chargen_addr_options(WORD mask, WORD value);
extern void vicii_set_phi1_chargen_addr_options(WORD mask, WORD value);
extern void vicii_set_phi2_chargen_addr_options(WORD mask, WORD value);

#endif
