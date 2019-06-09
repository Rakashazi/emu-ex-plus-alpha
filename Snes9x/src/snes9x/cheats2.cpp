/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include "snes9x.h"
#include "memmap.h"
#include "cheats.h"

static uint8 S9xGetByteFree (uint32);
static void S9xSetByteFree (uint8, uint32);


static uint8 S9xGetByteFree (uint32 address)
{
	int32	Cycles = CPU.Cycles;
    int32   NextEvent = CPU.NextEvent;
	uint8	byte;

    CPU.NextEvent = 0x7FFFFFFF;
	byte = S9xGetByte(address);
    CPU.NextEvent = NextEvent;
	CPU.Cycles = Cycles;

	return (byte);
}

static void S9xSetByteFree (uint8 byte, uint32 address)
{
	int32	Cycles = CPU.Cycles;
    int32  NextEvent = CPU.NextEvent;

    CPU.NextEvent = 0x7FFFFFFF;
	S9xSetByte(byte, address);
    CPU.NextEvent = NextEvent;
	CPU.Cycles = Cycles;
}

void S9xInitWatchedAddress (void)
{
	for (unsigned int i = 0; i < sizeof(watches) / sizeof(watches[0]); i++)
		watches[i].on = false;

}

void S9xInitCheatData (void)
{
	Cheat.RAM = Memory.RAM;
	Cheat.SRAM = Memory.SRAM;
	Cheat.FillRAM = Memory.FillRAM;
}

void S9xAddCheat (bool8 enable, bool8 save_current_value, uint32 address, uint8 byte)
{
	if (Cheat.num_cheats < sizeof(Cheat.c) / sizeof(Cheat.c[0]))
	{
		Cheat.c[Cheat.num_cheats].address = address;
		Cheat.c[Cheat.num_cheats].byte = byte;
		Cheat.c[Cheat.num_cheats].enabled = enable;

		if (save_current_value)
		{
			Cheat.c[Cheat.num_cheats].saved_byte = S9xGetByteFree(address);
			Cheat.c[Cheat.num_cheats].saved = TRUE;
		}

		Cheat.num_cheats++;
	}
}

void S9xDeleteCheat (uint32 which1)
{
	if (which1 < Cheat.num_cheats)
	{
		if (Cheat.c[which1].enabled)
			S9xRemoveCheat(which1);

		memmove(&Cheat.c[which1], &Cheat.c[which1 + 1], sizeof(Cheat.c[0]) * (Cheat.num_cheats - which1 - 1));

		Cheat.num_cheats--;
	}
}

void S9xDeleteCheats (void)
{
	S9xRemoveCheats();
	Cheat.num_cheats = 0;
}

void S9xRemoveCheat (uint32 which1)
{
	if (Cheat.c[which1].saved)
	{
		uint32	address = Cheat.c[which1].address;

		int		block = (address & 0xffffff) >> MEMMAP_SHIFT;
		uint8	*ptr = Memory.Map[block];

		if (ptr >= (uint8 *) CMemory::MAP_LAST)
			*(ptr + (address & 0xffff)) = Cheat.c[which1].saved_byte;
		else
			S9xSetByteFree(Cheat.c[which1].saved_byte, address);
	}
}

void S9xRemoveCheats (void)
{
	for (uint32 i = 0; i < Cheat.num_cheats; i++)
		if (Cheat.c[i].enabled)
			S9xRemoveCheat(i);
}

void S9xEnableCheat (uint32 which1)
{
	if (which1 < Cheat.num_cheats && !Cheat.c[which1].enabled)
	{
		Cheat.c[which1].enabled = TRUE;
		S9xApplyCheat(which1);
	}
}

void S9xDisableCheat (uint32 which1)
{
	if (which1 < Cheat.num_cheats && Cheat.c[which1].enabled)
	{
		S9xRemoveCheat(which1);
		Cheat.c[which1].enabled = FALSE;
	}
}

void S9xApplyCheat (uint32 which1)
{
	uint32	address = Cheat.c[which1].address;

	if (!Cheat.c[which1].saved)
	{
		Cheat.c[which1].saved_byte = S9xGetByteFree(address);
		Cheat.c[which1].saved = TRUE;
	}

	int		block = (address & 0xffffff) >> MEMMAP_SHIFT;
	uint8	*ptr = Memory.Map[block];

	if (ptr >= (uint8 *) CMemory::MAP_LAST)
		*(ptr + (address & 0xffff)) = Cheat.c[which1].byte;
	else
		S9xSetByteFree(Cheat.c[which1].byte, address);
}

void S9xApplyCheats (void)
{
	if (Settings.ApplyCheats)
	{
		for (uint32 i = 0; i < Cheat.num_cheats; i++)
			if (Cheat.c[i].enabled)
				S9xApplyCheat(i);
	}
}

bool8 S9xLoadCheatFile (const char *filename)
{
	FILE	*fs;
	uint8	data[28];

	Cheat.num_cheats = 0;

	fs = fopen(filename, "rb");
	if (!fs)
		return (FALSE);

	while (fread((void *) data, 1, 28, fs) == 28)
	{
		Cheat.c[Cheat.num_cheats].enabled = (data[0] & 4) == 0;
		Cheat.c[Cheat.num_cheats].byte = data[1];
		Cheat.c[Cheat.num_cheats].address = data[2] | (data[3] << 8) |  (data[4] << 16);
		Cheat.c[Cheat.num_cheats].saved_byte = data[5];
		Cheat.c[Cheat.num_cheats].saved = (data[0] & 8) != 0;
		memmove(Cheat.c[Cheat.num_cheats].name, &data[8], 20);
		Cheat.c[Cheat.num_cheats++].name[20] = 0;
	}

	fclose(fs);

	return (TRUE);
}

bool8 S9xSaveCheatFile (const char *filename)
{
	if (Cheat.num_cheats == 0)
	{
		remove(filename);
		return (TRUE);
	}

	FILE	*fs;
	uint8	data[28];

	fs = fopen(filename, "wb");
	if (!fs)
		return (FALSE);

	for (uint32 i = 0; i < Cheat.num_cheats; i++)
	{
		memset(data, 0, 28);

		if (i == 0)
		{
			data[6] = 254;
			data[7] = 252;
		}

		if (!Cheat.c[i].enabled)
			data[0] |= 4;

		if (Cheat.c[i].saved)
			data[0] |= 8;

		data[1] = Cheat.c[i].byte;
		data[2] = (uint8) (Cheat.c[i].address >> 0);
		data[3] = (uint8) (Cheat.c[i].address >> 8);
		data[4] = (uint8) (Cheat.c[i].address >> 16);
		data[5] = Cheat.c[i].saved_byte;

		memmove(&data[8], Cheat.c[i].name, 19);

		if (fwrite(data, 28, 1, fs) != 1)
		{
			fclose(fs);
			return (FALSE);
		}
	}

	return (fclose(fs) == 0);
}
