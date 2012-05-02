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

	flash.c

//=========================================================================
//---------------------------------------------------------------------------

  History of changes:
  ===================

20 JUL 2002 - neopop_uk
=======================================
- Cleaned and tidied up for the source release

26 JUL 2002 - neopop_uk
=======================================
- Added flash_read(char* filename) - this is called after loading the rom
	by the common emulator code.
- Added defintions of flash data header and block.
- Added flash header reading to determine buffer size.
- Added flash header writing.
- Added flash block database, saving and loading of blocks - COMPLETE!

- Removed the EEPROM emulation as it is unused, and will likely never
	be completed.

- Renamed this file "flash.c"

10 AUG 2002 - neopop_uk
=======================================
- Added flash_commit function to do the writing.

11 AUG 2002 - neopop_uk
=======================================
- Optimised flash_write

15 AUG 2002 - neopop_uk
=======================================
- Removed the legacy 'eeprom' variables.

08 SEP 2002 - neopop_uk
=======================================
- Made flash writes more robust be eliminating a possible logic error.

//---------------------------------------------------------------------------
*/

#include "neopop.h"
#include "flash.h"
#include "mem.h"

//-----------------------------------------------------------------------------
// Local Definitions
//-----------------------------------------------------------------------------
//This value is used to verify flash data - it is set to the
//version number that the flash description was modified for.

#define FLASH_VALID_ID		0x0053

//Number of different flash blocks, this should be enough.

#define FLASH_MAX_BLOCKS	256

typedef struct
{
	//Flash Id
	uint16 valid_flash_id;		// = FLASH_VALID_ID
	
	uint16 block_count;			//Number of flash data blocks

	uint32 total_file_length;		// header + block[0 - block_count]

} FlashFileHeader;

typedef struct
{
	uint32 start_address;		// 24 bit address
	uint16 data_length;		// length of following data

	//Followed by data_length bytes of the actual data.

} FlashFileBlockHeader;

//-----------------------------------------------------------------------------
// Local Data
//-----------------------------------------------------------------------------
static FlashFileBlockHeader	blocks[256];
static uint16 block_count;

//=============================================================================

//-----------------------------------------------------------------------------
// optimise_blocks()
//-----------------------------------------------------------------------------
static void optimise_blocks(void)
{
	int i, j;

	// Bubble Sort by address
	for (i = 0; i < block_count - 1; i++)
	{
		for (j = i+1; j < block_count; j++)
		{
			//Swap?
			if (blocks[i].start_address > blocks[j].start_address)
			{
				uint32 temp32;
				uint16 temp16;

				temp32 = blocks[i].start_address;
				blocks[i].start_address = blocks[j].start_address;
				blocks[j].start_address = temp32;

				temp16 = blocks[i].data_length;
				blocks[i].data_length = blocks[j].data_length;
				blocks[j].data_length = temp16;
			}
		}
	}

	//Join contiguous blocks
	//Only advance 'i' if required, this will allow subsequent
	//blocks to be compared to the newly expanded block.
	for (i = 0; i < block_count - 1; /**/)
	{
		//Next block lies within (or borders) this one?
		if (blocks[i+1].start_address <=
			(blocks[i].start_address + blocks[i].data_length))
		{
			//Extend the first block
			blocks[i].data_length = 
				(uint16)((blocks[i+1].start_address + blocks[i+1].data_length) -
				blocks[i].start_address);

			//Remove the next one.
			for (j = i+2; j < block_count; j++)
			{
				blocks[j-1].start_address = blocks[j].start_address;
				blocks[j-1].data_length = blocks[j].data_length;
			}
			block_count --;
		}
		else
		{
			i++;	// Try the next block
		}
	}
}

//=============================================================================

//-----------------------------------------------------------------------------
// flash_read()
//-----------------------------------------------------------------------------
void flash_read(void)
{
	FlashFileHeader header;
	uint8* flashdata, *fileptr;
	uint16 i;
	uint32 j;

	//Initialise the internal flash configuration
	block_count = 0;

	//Read flash buffer header
	if (system_io_flash_read((uint8*)&header, sizeof(FlashFileHeader)) == FALSE)
		return;	//Silent failure - no flash data yet.

	//Verify correct flash id
	if (header.valid_flash_id != FLASH_VALID_ID)
	{
		system_message(system_get_string(IDS_BADFLASH));
		return;
	}

	//Read the flash data
	flashdata = (uint8*)malloc(header.total_file_length * sizeof(uint8));
	system_io_flash_read(flashdata, header.total_file_length);

	//Read header
	block_count = header.block_count;
	fileptr = flashdata + sizeof(FlashFileHeader);

	//Copy blocks
	memory_unlock_flash_write = TRUE;
	for (i = 0; i < block_count; i++)
	{
		FlashFileBlockHeader* current = (FlashFileBlockHeader*)fileptr;
		fileptr += sizeof(FlashFileBlockHeader);
		
		blocks[i].start_address = current->start_address;
		blocks[i].data_length = current->data_length;

		//Copy data
		for (j = 0; j < blocks[i].data_length; j++)
		{
			storeB(blocks[i].start_address + j, *fileptr);
			fileptr++;
		}
	}
	memory_unlock_flash_write = FALSE;

	//Tidy up.
	free(flashdata);

	optimise_blocks();		//Optimise


	//Output block list...
/*	for (i = 0; i < block_count; i++)
		system_debug_message("flash block: %06X, %d bytes", 
			blocks[i].start_address, blocks[i].data_length);*/
}

//-----------------------------------------------------------------------------
// flash_write()
//-----------------------------------------------------------------------------
void flash_write(uint32 start_address, uint16 length)
{
	uint16 i;

	//Now we need a new flash command before the next flash write will work!
	memory_flash_command = FALSE;

//	system_debug_message("flash write: %06X, %d bytes", start_address, length);

	for (i = 0; i < block_count; i++)
	{
		//Got this block with enough bytes to cover it
		if (blocks[i].start_address == start_address &&
			blocks[i].data_length >= length)
		{
			return; //Nothing to do, block already registered.
		}

		//Got this block with but its length is too short
		if (blocks[i].start_address == start_address &&
			blocks[i].data_length < length)
		{
			blocks[i].data_length = length;	//Enlarge block updating.
			return;
		}
	}

	// New block needs to be added
	blocks[block_count].start_address = start_address;
	blocks[block_count].data_length = length;
	block_count++;
}

//-----------------------------------------------------------------------------
// flash_commit()
//-----------------------------------------------------------------------------
void flash_commit(void)
{
	uint8 *flashdata;
	int length;

	flashdata = flash_prepare(&length);
	
	//Try to Write flash buffer
	system_io_flash_write(flashdata, length);

	free(flashdata);
}


uint8 *flash_prepare(int *lenp)
{
	int i;
	FlashFileHeader header;
	uint8 *flashdata, *fileptr;

	//No flash data?
	if (block_count == 0) {
		*lenp = 0;
		return NULL;
	}

	//Optimise before writing
	optimise_blocks();

	//Build a header;
	header.valid_flash_id = FLASH_VALID_ID;
	header.block_count = block_count;
	header.total_file_length = sizeof(FlashFileHeader);
	for (i = 0; i < block_count; i++)
	{
		header.total_file_length += sizeof(FlashFileBlockHeader);
		header.total_file_length += blocks[i].data_length;
	}

	//Write the flash data
	flashdata = (uint8*)malloc(header.total_file_length * sizeof(uint8));

	//Copy header
	memcpy(flashdata, &header, sizeof(FlashFileHeader));
	fileptr = flashdata + sizeof(FlashFileHeader);

	//Copy blocks
	for (i = 0; i < block_count; i++)
	{
		uint32 j;

		memcpy(fileptr, &blocks[i], sizeof(FlashFileBlockHeader));
		fileptr += sizeof(FlashFileBlockHeader);

		//Copy data
		for (j = 0; j < blocks[i].data_length; j++)
		{
			*fileptr = loadB(blocks[i].start_address + j);
			fileptr++;
		}
	}

	*lenp = header.total_file_length;
	return flashdata;
}
//=============================================================================
