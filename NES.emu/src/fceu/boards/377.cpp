/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 * Copyright (C) 2022
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* NES 2.0 Mapper 377 - NES 2.0 Mapper 377 is used for the
 * 1998 Super Game 8-in-1 (JY-111) pirate multicart. It works similarly to Mapper 267 except it has an outer 256KiB PRG-ROM bank.
 */

#include "mapinc.h"
#include "mmc3.h"

#define OUTER_BANK (((EXPREGS[0] & 0x20) >> 2) | (EXPREGS[0] & 0x06))

static void M377CW(uint32 A, uint8 V) {
	setchr1(A, (V & 0x7F) | (OUTER_BANK << 6));
}

static void M377PW(uint32 A, uint8 V) {
	setprg8(A, (V & 0x0F) | (OUTER_BANK << 3));
}

static DECLFW(M377Write) {
    if (!(EXPREGS[0] & 0x80)) {
		EXPREGS[0] = V;
		FixMMC3PRG(MMC3_cmd);
		FixMMC3CHR(MMC3_cmd);
	}
}

static void M377Reset(void) {
	EXPREGS[0] = 0;
	MMC3RegReset();
}

static void M377Power(void) {
	EXPREGS[0] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M377Write);
}

void Mapper377_Init(CartInfo *info) {
	GenMMC3_Init(info, 128, 128, 0, 0);
	cwrap = M377CW;
	pwrap = M377PW;
	info->Reset = M377Reset;
	info->Power = M377Power;
	AddExState(EXPREGS, 1, 0, "EXPR");
}
