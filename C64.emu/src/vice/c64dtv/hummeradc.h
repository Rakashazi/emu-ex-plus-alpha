/*
 * hummeradc.h - Hummer ADC emulation
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
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

#ifndef VICE_HUMMERADC_H
#define VICE_HUMMERADC_H

#include "types.h"

extern void hummeradc_init(void);
extern void hummeradc_reset(void);
extern void hummeradc_shutdown(void);

extern BYTE hummeradc_read(void);
extern void hummeradc_store(BYTE value);

extern BYTE hummeradc_value;
extern BYTE hummeradc_channel;
extern BYTE hummeradc_control;
extern BYTE hummeradc_chanattr;
extern BYTE hummeradc_chanwakeup;
extern BYTE hummeradc_prev;

#endif
