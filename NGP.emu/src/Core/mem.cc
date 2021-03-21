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

	mem.c

//=========================================================================
//---------------------------------------------------------------------------

  History of changes:
  ===================

20 JUL 2002 - neopop_uk
=======================================
- Cleaned and tidied up for the source release

22 JUL 2002 - neopop_uk
=======================================
- Added default state of interrupt control register, both enabled.
- Added auto detection of rom's colour mode.

25 JUL 2002 - neopop_uk
=======================================
- Added default hardware window values.

27 JUL 2002 - neopop_uk
=======================================
- Bios calls are only decoded in non-hle mode now. The HLE code provides
a more efficient way of decoding calls. 

01 AUG 2002 - neopop_uk
=======================================
- Added a default background colour. Improves "Mezase! Kanji Ou"

01 AUG 2002 - neopop_uk
=======================================
- Added default settings for memory error and mask memory error.
  
10 AUG 2002 - neopop_uk
=======================================
- Moved all of the bios memory setup from bios.c
- Added more detailed setup of memory, to fool clever games.
- Added raster horizontal position emulation, fixes "NeoGeo Cup '98 Plus Color"

15 AUG 2002 - neopop_uk
=======================================
- Made EEPROM status more secure if something goes wrong.

16 AUG 2002 - neopop_uk
=======================================
- Ignores direct EEPROM access to a rom that's too short.

05 SEP 2002 - neopop_uk
=======================================
- In debug mode, NULL memory read/writes only create an error if the
	error message mask is disabled.

07 SEP 2002 - neopop_uk
=======================================
- Made the direct EEPROM access more secure by requiring an EEPROM
command to be issued before every write. This isn't perfect, but it
should stop "Gals Fighters" from writing all over itself.

//---------------------------------------------------------------------------
*/

#include "neopop.h"
#include "TLCS900h_registers.h"
#include "Z80_interface.h"
#include "bios.h"
#include "gfx.h"
#include "mem.h"
#include "interrupt.h"
#include "sound.h"
#include "flash.h"
#include <assert.h>
#include <imagine/logger/logger.h>
#include <imagine/util/bitset.hh>

using uint16u [[gnu::aligned(1)]] = uint16;
using uint32u [[gnu::aligned(1)]] = uint32;

//=============================================================================

//Hack way of returning good EEPROM status.
bool eepromStatusEnable = FALSE;
static uint32 eepromStatus;

uint8 ram[1 + RAM_END - RAM_START];

bool debug_abort_memory = FALSE;
bool debug_mask_memory_error_messages = FALSE;

bool memory_unlock_flash_write = FALSE;
bool memory_flash_error = FALSE;
bool memory_flash_command = FALSE;

//=============================================================================

#ifdef NEOPOP_DEBUG
static void memory_error(uint32 address, bool read)
{
	debug_abort_memory = TRUE;

	if (filter_mem)
	{
		if (debug_mask_memory_error_messages)
			return;

		if (read)
			system_debug_message("Memory Exception: Read from %06X", address);
		else
			system_debug_message("Memory Exception: Write to %06X", address);
	}
}
#endif

//=============================================================================

static uint32 memNullVal = 0;

void* translate_address_read(uint32 address)
{
	address &= 0xFFFFFF;

#ifdef NEOPOP_DEBUG

	if (address == 0 && debug_mask_memory_error_messages == FALSE)
	{ memory_error(address, TRUE); return &memNullVal; }

#endif

	// ===================================

	if (address <= RAM_END)
	{
		//RAS.H read (Simulated horizontal raster position)
		if (address == 0x8008)
		{
			ram[0x8008] = (uint8)((abs(TIMER_HINT_RATE - (int)timer_hint)) >> 2);
			//logMsg("reading h-ras %d", (int)ram[0x8008]);
		}
		return ram + address;
	}

	// ===================================

	//Get EEPROM status?
	if (eepromStatusEnable)
	{
		eepromStatusEnable = FALSE;
		if (address == 0x220000 || address == 0x230000)
		{
			eepromStatus = 0xFFFFFFFF;
			return &eepromStatus;
		}
	}

	//ROM (LOW)
	//if (/*rom.data &&*/ address >= ROM_START && address <= ROM_END)
	if (address >= ROM_START && address <= rom.realEnd)
	{
		assert(address <= ROM_START + rom.length);
		return rom.data + (address & 0x1FFFFF);
		/*if (address <= ROM_START + rom.length)
			return rom.data + (address - ROM_START);
		else
			return &nullVal;*/
	}

	//ROM (HIGH)
	//if (/*rom.data &&*/ address >= HIROM_START && address <= HIROM_END)
	if (address >= ROM_START && address <= rom.realHEnd)
	{
		assert(address <= HIROM_START + (rom.length - 0x200000));
		return rom.data + 0x200000 + (address - HIROM_START);
		/*if (address <= HIROM_START + (rom.length - 0x200000))
			return rom.data + 0x200000 + (address - HIROM_START);
		else
			return &nullVal;*/
	}

	// ===================================

	//BIOS Access?
	if ((address & 0xFF0000) == 0xFF0000)
		return bios + (address & 0xFFFF); // BIOS ROM

	// ===================================

	//Signal a flash memory error
	if (memory_unlock_flash_write)
	{
		logMsg("flash memory error");
		memory_flash_error = TRUE;
	}

#ifdef NEOPOP_DEBUG
	memory_error(address, TRUE);
#endif
	logMsg("read 0x%X invalid", address);
	memNullVal = 0;
	return &memNullVal;
}

//=============================================================================

void* translate_address_write(uint32 address)
{	
	address &= 0xFFFFFF;

#ifdef NEOPOP_DEBUG

	if (address == 0 && debug_mask_memory_error_messages == FALSE)
	{ memory_error(address, FALSE); return NULL; }

#endif

	// ===================================


	if (address <= RAM_END)
		return ram + address;

	// ===================================

	if (memory_unlock_flash_write)
	{
		//ROM (LOW)
		if (/*rom.data &&*/ address >= ROM_START && address <= ROM_END)
		{
			if (address <= ROM_START + rom.length)
				return rom.data + (address - ROM_START);
			else
			{
				logMsg("write 0x%X past size of low rom", address);
				return &memNullVal;
			}
		}

		//ROM (HIGH)
		if (/*rom.data &&*/ address >= HIROM_START && address <= HIROM_END)
		{
			if (address <= HIROM_START + (rom.length - 0x200000))
				return rom.data + 0x200000 + (address - HIROM_START);
			else
			{
				logMsg("write 0x%X past size of high rom", address);
				return &memNullVal;
			}
		}

		//Signal a flash memory error
		memory_flash_error = TRUE;
	}
	else
	{
		//ROM (LOW)
		if (/*rom.data &&*/ address >= ROM_START && address <= ROM_END)
		{
			//Ignore EEPROM commands
			if (address == 0x202AAA || address == 0x205555)
			{
	//			system_debug_message("%06X: Enable EEPROM command from %06X", pc, address);
				memory_flash_command = TRUE;
				return &memNullVal;
			}

			//Set EEPROM status reading?
			if (address == 0x220000 || address == 0x230000)
			{
	//			system_debug_message("%06X: EEPROM status read from %06X", pc, address);
				eepromStatusEnable = TRUE;
				return &memNullVal;
			}

			if (memory_flash_command)
			{
				//Write the 256byte block around the flash data
				flash_write(address & 0xFFFF00, 256);
				
				//Need to issue a new command before writing will work again.
				memory_flash_command = FALSE;
		
	//			system_debug_message("%06X: Direct EEPROM write to %06X", pc, address & 0xFFFF00);
	//			system_debug_stop();

				//Write to the rom itself.
				if (address <= ROM_START + rom.length)
				{
					logMsg("write 0x%X to rom", address);
					return rom.data + (address - ROM_START);
				}
			}
		}
	}

	// ===================================

#ifdef NEOPOP_DEBUG
	memory_error(address, FALSE);
#endif
	logMsg("write 0x%X invalid", address);
	return &memNullVal;
}

//=============================================================================

void post_write(uint32 address)
{
	address &= 0xFFFFFF;

	//Direct Access to Sound Chips
	if ((*(uint16*)(ram + 0xb8)) == htole16(0xAA55))
	{
		if (address == 0xA1)	Write_SoundChipTone(ram[0xA1]);
		if (address == 0xA0)	Write_SoundChipNoise(ram[0xA0]);
	}

	//DAC Write
	if (address == 0xA2)	dac_write();

	//Clear counters?
	if (address == 0x20)
	{
		uint8 TRUN = ram[0x20];

		if ((TRUN & 0x01) == 0)		timer[0] = 0;
		if ((TRUN & 0x02) == 0)		timer[1] = 0;
		if ((TRUN & 0x04) == 0)		timer[2] = 0;
		if ((TRUN & 0x08) == 0)		timer[3] = 0;
	}

	//z80 - NMI
	if (address == 0xBA)
		Z80_nmi();
}

//=============================================================================

static const bool ALIGN_ACCESS = 0;

uint8 loadB(uint32 address)
{
	uint8* ptr = (uint8*)translate_address_read(address);
	/*if (ptr == NULL)
		return 0;
	else*/
		return *ptr;
}

uint16 loadW(uint32 address)
{
	uint16u *ptr = (uint16u*)translate_address_read(address);
	if((uintptr_t)ptr % 2 != 0)
	{
		//bug_exit("address %X", address);
	}
	/*if (ptr == NULL)
		return 0;
	else*/
	if(ALIGN_ACCESS && ((uintptr_t)ptr & IG::bit(0)))
	{
		uint16 v;
		memcpy(&v, ptr, 2); // LE
		return v;
	}

	return le16toh(*ptr);
}

uint32 loadL(uint32 address)
{
	uint32u *ptr = (uint32u*)translate_address_read(address);
	if((uintptr_t)ptr % 4 != 0)
	{
		//bug_exit("address %X", address);
	}
	/*if (ptr == NULL)
		return 0;
	else*/

	if(ALIGN_ACCESS && ((uintptr_t)ptr & (IG::bit(0) | IG::bit(1))))
	{
		uint32 v;
		memcpy(&v, ptr, 4); // LE
		return v;
	}

	return le32toh(*ptr);
}

//=============================================================================

void storeB(uint32 address, uint8 data)
{
	uint8* ptr = (uint8*)translate_address_write(address);

	//Write
	//if (ptr)
	{
		*ptr = data;
		post_write(address);
	}
}

void storeW(uint32 address, uint16 data)
{
	uint16u *ptr = (uint16u*)translate_address_write(address);
	if((uintptr_t)ptr % 2 != 0)
	{
		//bug_exit("address %X", address);
	}

	//Write
	//if (ptr)

	if(ALIGN_ACCESS && ((uintptr_t)ptr & IG::bit(0)))
		memcpy(ptr, &data, 2); // LE
	else
		*ptr = htole16(data);
	{

		post_write(address);
	}
}

void storeL(uint32 address, uint32 data)
{
	uint32u *ptr = (uint32u*)translate_address_write(address);
	if((uintptr_t)ptr % 4 != 0)
	{
		//bug_exit("address %X", address);
	}

	//Write
	//if (ptr)
	if(ALIGN_ACCESS && ((uintptr_t)ptr & (IG::bit(0) | IG::bit(1))))
		memcpy(ptr, &data, 4); // LE
	else
		*ptr = htole32(data);
	{
		post_write(address);
	}
}

//=============================================================================

static uint8 systemMemory[] =
{
	// 0x00												// 0x08
	0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x08, 0xFF, 0xFF,
	// 0x10												// 0x18
	0x34, 0x3C, 0xFF, 0xFF, 0xFF, 0x3F, 0x00, 0x00,		0x3F, 0xFF, 0x2D, 0x01, 0xFF, 0xFF, 0x03, 0xB2,
	// 0x20												// 0x28
	0x80, 0x00, 0x01, 0x90, 0x03, 0xB0, 0x90, 0x62,		0x05, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x4C, 0x4C,
	// 0x30												// 0x38
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		0x30, 0x00, 0x00, 0x00, 0x20, 0xFF, 0x80, 0x7F,
	// 0x40												// 0x48
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	// 0x50												// 0x58
	0x00, 0x20, 0x69, 0x15, 0x00, 0x00, 0x00, 0x00,		0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
	// 0x60												// 0x68
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		0x17, 0x17, 0x03, 0x03, 0x02, 0x00, 0x00, 0x4E,
	// 0x70												// 0x78
	0x02, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	// 0x80												// 0x88
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	// 0x90												// 0x98
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	// 0xA0												// 0xA8
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	// 0xB0												// 0xB8
	0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00,		0xAA, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	// 0xC0												// 0xC8
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	// 0xD0												// 0xD8
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	// 0xE0												// 0xE8
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	// 0xF0												// 0xF8
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

//=============================================================================

void reset_memory(void)
{
	unsigned int i;

	eepromStatusEnable = FALSE;
	memory_flash_command = FALSE;

	memset(ram, 0, sizeof(ram));	//Clear ram

//=============================================================================
//000000 -> 000100	CPU Internal RAM (Timers/DMA/Z80)
//=====================================================

	for (i = 0; i < sizeof(systemMemory); i++)
		ram[i] = systemMemory[i];

//=============================================================================
//006C00 -> 006FFF	BIOS Workspace
//==================================

	if (rom.data)
	{
		*(uint32*)(ram + 0x6C00) = rom_header->startPC;		//Start
		*(uint16*)(ram + 0x6E82) =
			*(uint16*)(ram + 0x6C04) = rom_header->catalog;	//Catalog
		*( uint8*)(ram + 0x6E84) =
			*( uint8*)(ram + 0x6C06) = rom_header->subCatalog;	//Sub-Cat
		memcpy(ram + 0x6C08, rom.data + 0x24, 12);			//name

		*(uint8*)(ram + 0x6C58) = 0x01;			//LO-EEPROM present

		//32MBit cart?
		if (rom.length > 0x200000)
			*(uint8*)(ram + 0x6C59) = 0x01;			//HI-EEPROM present
		else
			*(uint8*)(ram + 0x6C59) = 0x00;			//HI-EEPROM not present

		ram[0x6C55] = 1;	//Commercial game

		// pre-calc rom offsets
		rom.realEnd = ROM_START + rom.length;
		rom.realHEnd = HIROM_START + (rom.length - 0x200000);
	}
	else
	{
		*(uint32*)(ram + 0x6C00) = htole32(0x00FF970A);	//Start
		*(uint16*)(ram + 0x6C04) = htole16(0xFFFF);	//Catalog
		*( uint8*)(ram + 0x6C06) = 0x00;			//Sub-Cat
		sprintf((char*)(ram + 0x6C08), "NEOGEOPocket");	//bios rom 'Name'

		*(uint8*)(ram + 0x6C58) = 0x00;			//LO-EEPROM not present
		*(uint8*)(ram + 0x6C59) = 0x00;			//HI-EEPROM not present

		ram[0x6C55] = 0;	//Bios menu
	}
	

	ram[0x6F80] = 0xFF;	//Lots of battery power!
	ram[0x6F81] = 0x03;

	ram[0x6F84] = 0x40;	// "Power On" startup
	ram[0x6F85] = 0x00;	// No shutdown request
	ram[0x6F86] = 0x00;	// No user answer (?)

	//Language: 0 = Japanese, 1 = English
	ram[0x6F87] = (uint8)language_english;

	//Color Mode Selection: 0x00 = B&W, 0x10 = Colour
	if (system_colour == COLOURMODE_GREYSCALE)
		ram[0x6F91] = ram[0x6F95] = 0x00; //Force Greyscale
	if (system_colour == COLOURMODE_COLOUR)	
		ram[0x6F91] = ram[0x6F95] = 0x10; //Force Colour
	if (system_colour == COLOURMODE_AUTO) 
	{
		if (rom.data) 
			ram[0x6F91] = ram[0x6F95] = rom_header->mode;	//Auto-detect
		else 
			ram[0x6F91] = ram[0x6F95] = 0x10; // Default = Colour
	}

	//Interrupt table
	for (i = 0; i < 0x12; i++)
		*(uint32*)(ram + 0x6FB8 + (i * 4)) = htole32(0x00FF23DF);


//=============================================================================
//008000 -> 00BFFF	Video RAM
//=============================

	ram[0x8000] = 0xC0;	// Both interrupts allowed

	//Hardware window
	ram[0x8002] = 0x00;
	ram[0x8003] = 0x00;
	ram[0x8004] = 0xFF;
	ram[0x8005] = 0xFF;

	ram[0x8006] = 0xc6;	// Frame Rate Register

	ram[0x8012] = 0x00;	// NEG / OOWC setting.

	ram[0x8118] = 0x80;	// BGC on!

	ram[0x83E0] = 0xFF;	// Default background colour
	ram[0x83E1] = 0x0F;

	ram[0x83F0] = 0xFF;	// Default window colour
	ram[0x83F1] = 0x0F;

	ram[0x8400] = 0xFF;	// LED on
	ram[0x8402] = 0x80;	// Flash cycle = 1.3s
}

//=============================================================================


