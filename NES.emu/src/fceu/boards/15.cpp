/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2006 CaH4e3
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "mapinc.h"

static uint16 latchea;
static uint8 latched;
static uint8 *WRAM = NULL;
static uint32 WRAMSIZE;
static SFORMAT StateRegs[] =
{
	{ &latchea, 2 | FCEUSTATE_RLSB, "AREG" },
	{ &latched, 1, "DREG" },
	{ 0 }
};

static void Sync(void) {
	uint32 preg[4];
	uint32 bank = (latched & 0x3F) << 1;
	switch (latchea & 0x03) {
	case 0:
		preg[0] = bank + 0;
		preg[1] = bank + 1;
		preg[2] = bank + 2;
		preg[3] = bank + 3;
		break;
	case 2:
		bank = bank | (latched >> 7);
		preg[0] = bank;
		preg[1] = bank;
		preg[2] = bank;
		preg[3] = bank;
		break;
	case 1:
	case 3:
		preg[0] = bank + 0;
		preg[1] = bank + 1;
		preg[2] = (((latchea & 0x02) == 0) ? (bank | 0xE) : bank) + 0;
		preg[3] = (((latchea & 0x02) == 0) ? (bank | 0xE) : bank) + 1;
		break;
	}

	setprg8(0x8000, preg[0]);
	setprg8(0xA000, preg[1]);
	setprg8(0xC000, preg[2]);
	setprg8(0xE000, preg[3]);
	setmirror(((latched >> 6) & 1) ^ 1);
	setchr8(0);
}

static DECLFW(M15Write) {
	latchea = A;
	latched = V;
	/* cah4e3 02.10.19 once again, there may be either two similar mapper 15 exist. the one for 110in1 or 168in1 carts with complex multi game features.
	   and another implified version for subor/waixing chinese originals and hacks with no different modes, working only in mode 0 and which does not
	   expect there is any CHR write protection. protecting CHR writes only for mode 3 fixes the problem, all roms may be run on the same source again. */
	if((latchea & 3) == 3)
		SetupCartCHRMapping(0, CHRptr[0], 0x2000, 0);
	else
		SetupCartCHRMapping(0, CHRptr[0], 0x2000, 1);
	Sync();
}

static void StateRestore(int version) {
	Sync();
}

static void M15Power(void) {
	latchea = 0x8000;
	latched = 0;
	setchr8(0);
	setprg8r(0x10, 0x6000, 0);
	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetWriteHandler(0x8000, 0xFFFF, M15Write);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
    FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
	Sync();
}

static void M15Reset(void) {
	latchea = 0x8000;
	latched = 0;
	Sync();
}

static void M15Close(void) {
	if (WRAM)
		FCEU_gfree(WRAM);
	WRAM = NULL;
}

void Mapper15_Init(CartInfo *info) {
	info->Power = M15Power;
	info->Reset = M15Reset;
	info->Close = M15Close;
	GameStateRestore = StateRestore;
	WRAMSIZE = 8192;
	WRAM = (uint8*)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	if (info->battery) {
		info->SaveGame[0] = WRAM;
		info->SaveGameLen[0] = WRAMSIZE;
	}
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
	AddExState(&StateRegs, ~0, 0, 0);
}

