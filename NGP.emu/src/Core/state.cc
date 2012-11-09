//---------------------------------------------------------------------------
// NEOPOP : Emulator as in Dreamland
//
// Copyright (c) 2001-2002 by neopop_uk
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version. See also the license.txt file for
//	additional informations.
//---------------------------------------------------------------------------

/*
//---------------------------------------------------------------------------
//=========================================================================

	state.c

//=========================================================================
//---------------------------------------------------------------------------

  History of changes:
  ===================

20 JUL 2002 - neopop_uk
=======================================
- Cleaned and tidied up for the source release

28 JUL 2002 - neopop_uk
=======================================
- Converted the load and save functions to use the new state name.
- Move the version "0050" state loader into it's own function, this makes
	it much easier to add new loaders in the future.

15 AUG 2002 - neopop_uk
=======================================
- Removed storing the 'halt' state as the variable no longer exists.
- Changed 'int reserved1' into 'bool eepromStatusEnable'

//---------------------------------------------------------------------------
*/

#include "neopop.h"
#include "state.h"
#include "TLCS900h_registers.h"
#include "interrupt.h"
#include "dma.h"
#include "mem.h"
#include "chunk.h"

//=============================================================================

static bool read_state_0050(const char* filename);
static bool read_state_0060(const char* filename);

//-----------------------------------------------------------------------------
// state_restore()
//-----------------------------------------------------------------------------
bool state_restore(const char* filename)
{
	uint16 version;

	if (system_io_state_read(filename, (uint8*)&version, sizeof(uint16)))
	{
		switch(version)
		{
		case 0x0050:
			return read_state_0050(filename);

#ifdef MSB_FIRST
		case 0x0060:
#else
		case 0x6000:
#endif
			return read_state_0060(filename);
		default:
			system_message(system_get_string(IDS_BADSTATE));
			return FALSE;
		}

#ifdef NEOPOP_DEBUG
		system_debug_message("Restoring State ...");
		system_debug_refresh();
#endif
	}

	return FALSE;
}

//=============================================================================

//-----------------------------------------------------------------------------
// state_store()
//-----------------------------------------------------------------------------
bool state_store(const char* filename)
{
	FILE *fp;
	int ret, options;

	/* XXX: user settable */
	options = OPT_ROMH;
	
	if ((fp=fopen(filename, "wb")) == NULL)
		return FALSE;
	
	ret = write_header(fp);
	ret = write_SNAP(fp, options);
	ret = write_EOD(fp);

	if (fclose(fp) < 0)
		ret = FALSE;

	return ret;
}

//=============================================================================

static bool read_state_0050(const char* filename)
{
	NEOPOPSTATE0050	state;
	int i,j;

	if (system_io_state_read(filename, (uint8*)&state, sizeof(NEOPOPSTATE0050)))
	{
		//Verify correct rom...
		if (memcmp(rom_header, &state.header, sizeof(RomHeader)) != 0)
		{
			system_message(system_get_string(IDS_WRONGROM));
			return FALSE;
		}

		//Apply state description
		reset();

		eepromStatusEnable = state.eepromStatusEnable;

		//TLCS-900h Registers
		pc = state.pc;
		sr = state.sr;				changedSP();
		f_dash = state.f_dash;

		eepromStatusEnable = state.eepromStatusEnable;

		for (i = 0; i < 4; i++)
		{
			gpr[i] = state.gpr[i];
			for (j = 0; j < 4; j++)
				gprBank[i][j] = state.gprBank[i][j];
		}

		//Timers
		timer_hint = state.timer_hint;

		for (i = 0; i < 4; i++)	//Up-counters
			timer[i] = state.timer[i];

		timer_clock0 = state.timer_clock0;
		timer_clock1 = state.timer_clock1;
		timer_clock2 = state.timer_clock2;
		timer_clock3 = state.timer_clock3;

		//Z80 Registers
		memcpy(&Z80_regs, &state.Z80_regs, sizeof(Z80));

		//Sound Chips
		memcpy(&toneChip, &state.toneChip, sizeof(SoundChip));
		memcpy(&noiseChip, &state.noiseChip, sizeof(SoundChip));

		//DMA
		for (i = 0; i < 4; i++)
		{
			dmaS[i] = state.dmaS[i];
			dmaD[i] = state.dmaD[i];
			dmaC[i] = state.dmaC[i];
			dmaM[i] = state.dmaM[i];
		}

		//Memory
		memcpy(ram, &state.ram, 0xC000);
		system_sound_chipreset(); // reset sound chip again or sample_chip_noise() can hang
		return TRUE;
	}

	return FALSE;
}

static bool read_state_0060(const char* filename)
{
	FILE *fp;
	uint32 tag, size;
	
	if ((fp=fopen(filename, "rb")) == NULL)
		return FALSE;

	if (read_header(fp) != TRUE) {
		fclose(fp);
		return FALSE;
	}

	if (read_chunk(fp, &tag, &size) != TRUE) {
	    fclose(fp);
	    return FALSE;
	}

	if (tag != TAG_SNAP) {
	    fclose(fp);
	    return FALSE;
	}

	return read_SNAP(fp, size);
}

//=============================================================================
