#pragma once

#include "m68k.h"
#include "cell_map.c"
#include "LC89510.h"
#include "cd_sys.h"
#include "gfx_cd.h"
#include "InstructionCycleTableSCD.hh"

namespace Mednafen
{
class CDAccess;
}

struct SegaCD
{
	constexpr SegaCD(): cpu(m68kCyclesSCD, 1) {}
	M68KCPU cpu;
	bool isActive = 0;
	uint8_t busreq = 0;
	unsigned stopwatchTimer = 0;
	unsigned counter75hz = 0;
	int timer_int3 = 0;
	unsigned volume = 1024;

	uint8_t gate[0x200]{};

	_scd_toc TOC;
	int32_t cddaLBA = 0;
	uint16_t cddaDataLeftover = 0;
	bool CDD_Complete = 0;
	unsigned Status_CDD = 0;
	unsigned Status_CDC = 0;
	int Cur_LBA = 0;
	unsigned Cur_Track = 0;
	int File_Add_Delay = 0;
	CDC cdc;
	CDD cdd;

	union PrgRam
	{
		constexpr PrgRam() {}
		uint8_t b[512 * 1024]{};
		uint8_t bank[4][128 * 1024];
	};
	PrgRam prg;

	union WordRam
	{
		constexpr WordRam() { }
		uint8_t ram2M[0x40000]{};
		uint8_t ram1M[2][0x20000];
	};
	WordRam word;

	Rot_Comp rot_comp;

	union PCMRam
	{
		constexpr PCMRam() {}
		uint8_t b[0x10000]{};
		uint8_t bank[0x10][0x1000];
	};
	PCMRam pcmMem;

	struct PCM
	{
		constexpr PCM() {}
		uint8_t control = 0; // reg7
		uint8_t enabled = 0; // reg8
		uint8_t cur_ch = 0;
		uint8_t bank = 0;

		struct Channel // 08, size 0x10
		{
			constexpr Channel() {}
			uint8_t regs[8]{};
			unsigned  addr = 0;	// .08: played sample address
		} ch[8];
	};
	PCM pcm;

	uint8_t bcramReg = 0;
	uint8_t audioTrack = 0;
	bool subResetPending = 0;
	bool delayedDMNA = 0;
};

extern SegaCD sCD;

// Pre-formatted internal BRAM
static const uint8_t fmtBram[4*0x10] =
{
	0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x00, 0x00, 0x00, 0x00, 0x40,
	0x00, 0x7d, 0x00, 0x7d, 0x00, 0x7d, 0x00, 0x7d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x53, 0x45, 0x47, 0x41, 0x5f, 0x43, 0x44, 0x5f, 0x52, 0x4f, 0x4d, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x52, 0x41, 0x4d, 0x5f, 0x43, 0x41, 0x52, 0x54, 0x52, 0x49, 0x44, 0x47, 0x45, 0x5f, 0x5f, 0x5f,
	// SEGA_CD_ROM.....RAM_CARTRIDGE___
};

// Pre-formatted 64K SRAM cart
static const uint8_t fmt64kSram[4*0x10] =
{
	0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x00, 0x00, 0x00, 0x00, 0x40,
	0x03, 0xfd, 0x03, 0xfd, 0x03, 0xfd, 0x03, 0xfd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x53, 0x45, 0x47, 0x41, 0x5f, 0x43, 0x44, 0x5f, 0x52, 0x4f, 0x4d, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x52, 0x41, 0x4d, 0x5f, 0x43, 0x41, 0x52, 0x54, 0x52, 0x49, 0x44, 0x47, 0x45, 0x5f, 0x5f, 0x5f,
	// SEGA_CD_ROM.....RAM_CARTRIDGE___
};

extern uint8_t bram[0x2000];

void scd_interruptSubCpu(unsigned irq);
void scd_resetSubCpu();
void scd_runSubCpu(unsigned cycles);
void scd_init();
void scd_deinit();
void scd_reset();
void scd_memmap();
void scd_update();
void scd_checkDma();
void scd_updateCddaVol();
int scd_saveState(uint8_t *state);
int scd_loadState(uint8_t *state, unsigned exVersion);

int Insert_CD(Mednafen::CDAccess *cd);
void Stop_CD();
