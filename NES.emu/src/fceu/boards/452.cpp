/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
 *  Copyright (C) 2002 Xodnizel
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
 */
 
/* DS-9-27. Absolutely insane PCB that overlays 8 KiB of WRAM into a selectable position between $8000 and $E000. */
 
#include "mapinc.h"

static uint8 *WRAM;
static uint32 WRAMSIZE;
static uint8 latch[2];

static void Mapper452_Sync(void) {
	uint8 wramBank = latch[1] >>3 &6 |8;
	if (latch[1] &2) {
		setprg8(0x8000, latch[0] >>1);
		setprg8(0xA000, latch[0] >>1);
		setprg8(0xC000, latch[0] >>1);
		setprg8(0xE000, latch[0] >>1);
		setprg8r(0x10, (wramBank ^4) <<12, 0);
	} else
	if (latch[1] &8) {
		setprg8(0x8000, latch[0] >>1 |0);
		setprg8(0xA000, latch[0] >>1 |1);
		setprg8(0xC000, latch[0] >>1 |2);
		setprg8(0xE000, latch[0] >>1 |3 | latch[1] &4);
	} else {
		setprg16(0x8000, latch[0] >>2);
		setprg16(0xC000, 0);
	}
	setprg8r(0x10, wramBank <<12, 0);
	setchr8(0);
	setmirror(latch [1] &1 ^1);
	
}

static DECLFW(Mapper452_WriteLatch) {
	latch[0] =A &0xFF;
	latch[1] =V;
	Mapper452_Sync();
	/* Do not relay to CartBW, as RAM mapped to locations other than $8000-$DFFF are not write-enabled. */
}

static void Mapper452_Reset(void) {
	latch[0] =latch[1] =0;
	Mapper452_Sync();
}

static void Mapper452_Power(void) {
	
	latch[0] =latch[1] =0;
	Mapper452_Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xDFFF, Mapper452_WriteLatch);
	SetWriteHandler(0xE000, 0xFFFF, CartBW);
    FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void Mapper452_Close(void) {
   if (WRAM)
      FCEU_gfree(WRAM);
   WRAM = NULL;
}

static void StateRestore(int version) {
   Mapper452_Sync();
}

void Mapper452_Init(CartInfo *info) {
	info->Reset = Mapper452_Reset;
	info->Power = Mapper452_Power;
	info->Close = Mapper452_Close;
	GameStateRestore = StateRestore;

	WRAMSIZE = 8192;
	WRAM = (uint8*) FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");

	AddExState(&latch, 2, 0, "LATC");
}
