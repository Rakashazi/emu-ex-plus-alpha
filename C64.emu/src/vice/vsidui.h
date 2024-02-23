/*
 * vsidui.h - Implementation of the C64-specific part of the UI.
 *
 * Written by
 *  Dag Lem <resid@nimrod.no>
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

#ifndef VICE_VSIDUI_H
#define VICE_VSIDUI_H

#include <stdint.h>

int vsid_ui_init_early(void);
int vsid_ui_init(void);
void vsid_ui_close(void);

void vsid_ui_display_name(const char *name);
void vsid_ui_display_author(const char *author);
void vsid_ui_display_copyright(const char *copyright);
void vsid_ui_display_sync(int sync);
void vsid_ui_display_sid_model(int model);
void vsid_ui_display_tune_nr(int nr);
void vsid_ui_display_nr_of_tunes(int count);
void vsid_ui_set_default_tune(int nr);
void vsid_ui_display_time(unsigned int sec);
void vsid_ui_display_irqtype(const char *irq);

void vsid_ui_setdrv(char* driver_info_text);
void vsid_ui_set_driver_addr(uint16_t addr);
void vsid_ui_set_load_addr(uint16_t addr);
void vsid_ui_set_init_addr(uint16_t addr);
void vsid_ui_set_play_addr(uint16_t addr);
void vsid_ui_set_data_size(uint16_t size);

#endif
