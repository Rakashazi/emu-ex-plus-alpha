/*
 * vicii.h - A cycle-exact event-driven MOS6569 (VIC-II) emulation.
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
#define VICII_EXTENDED 1
#define VICII_DTV 2

/* VICII border mode defines */
#define VICII_NORMAL_BORDERS 0
#define VICII_FULL_BORDERS   1
#define VICII_DEBUG_BORDERS  2
#define VICII_NO_BORDERS     3
#define VICII_BORDER_MODE(v) (v << 12)

/* VICII model defines (for vicii/) FIXME enum instead? */
#define VICII_MODEL_PALG        0
#define VICII_MODEL_PALG_OLD    1
#define VICII_MODEL_NTSCM       2
#define VICII_MODEL_NTSCM_OLD   3
#define VICII_MODEL_PALN        4

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

int vicii_resources_init(void);
int vicii_cmdline_options_init(void);
struct raster_s *vicii_init(unsigned int flag);
struct video_canvas_s *vicii_get_canvas(void);

void vicii_reset(void);
void vicii_set_light_pen(CLOCK mclk, int state);
void vicii_trigger_light_pen(CLOCK mclk);
CLOCK vicii_lightpen_timing(int x, int y);
void vicii_set_vbank(int new_vbank);
void vicii_set_ram_base(uint8_t *base);
void vicii_powerup(void);
void vicii_set_canvas_refresh(int enable);
void vicii_reset_registers(void);
void vicii_update_memory_ptrs_external(void);
void vicii_handle_pending_alarms_external(CLOCK num_write_cycles);
void vicii_handle_pending_alarms_external_write(void);

void vicii_screenshot(struct screenshot_s *screenshot);
void vicii_shutdown(void);
void vicii_change_timing(struct machine_timing_s *machine_timing, int border_mode);

int vicii_dump(void);

void vicii_snapshot_prepare(void);
int vicii_snapshot_write_module(struct snapshot_s *s);
int vicii_snapshot_read_module(struct snapshot_s *s);

void vicii_async_refresh(struct canvas_refresh_s *r);

void vicii_set_phi1_vbank(int num_vbank);
void vicii_set_phi2_vbank(int num_vbank);
void vicii_set_phi1_ram_base(uint8_t *base);
void vicii_set_phi2_ram_base(uint8_t *base);
void vicii_set_phi1_addr_options(uint16_t mask, uint16_t offset);
void vicii_set_phi2_addr_options(uint16_t mask, uint16_t offset);
void vicii_set_chargen_addr_options(uint16_t mask, uint16_t value);
void vicii_set_phi1_chargen_addr_options(uint16_t mask, uint16_t value);
void vicii_set_phi2_chargen_addr_options(uint16_t mask, uint16_t value);

/* 8502 specific functions */
CLOCK vicii_clock_add(CLOCK clock, int64_t amount);
void vicii_clock_read_stretch(void);
void vicii_clock_write_stretch(void);
int vicii_get_half_cycle(void);
void vicii_memory_refresh_alarm_handler(void);
int vicii_check_memory_refresh(CLOCK clock);

#endif
