#pragma once

#include "m68k.h"
#include "cell_map.c"
#include "LC89510.h"
#include "cd_sys.h"
#include "gfx_cd.h"
#include <genplus-gx/m68k/musashi/InstructionCycleTableSCD.hh>
#include <util/builtins.h>

struct SegaCD
{
	constexpr SegaCD(): cpu(m68kCyclesSCD), isActive(0), busreq(0),
		stopwatchTimer(0), counter75hz(0), timer_int3(0), gate CXX11_INIT_LIST({0}),
		CDD_Complete(0), Status_CDD(0), Status_CDC(0),
		Cur_LBA(0), Cur_Track(0), File_Add_Delay(0),
		bcramReg(0), audioTrack(0),
		subResetPending(0), delayedDMNA(0) { }
	M68KCPU cpu;
	fbool isActive;
	uchar busreq;
	uint stopwatchTimer;
	uint counter75hz;
	int timer_int3;

	uchar gate[0x200];

	_scd_toc TOC;
	fbool CDD_Complete;
	uint Status_CDD;
	uint Status_CDC;
	int Cur_LBA;
	uint Cur_Track;
	int File_Add_Delay;
	CDC cdc;
	CDD cdd;

	union PrgRam
	{
		constexpr PrgRam(): b CXX11_INIT_LIST({0}) { }
		uchar b[512 * 1024];
		uchar bank[4][128 * 1024];
	};
	PrgRam prg;

	union WordRam
	{
		constexpr WordRam(): ram2M CXX11_INIT_LIST({0}) { }
		uchar ram2M[0x40000];
		uchar ram1M[2][0x20000];
	};
	WordRam word;

	Rot_Comp rot_comp;

	union PCMRam
	{
		constexpr PCMRam(): b CXX11_INIT_LIST({0}) { }
		uchar b[0x10000];
		uchar bank[0x10][0x1000];
	};
	PCMRam pcmMem;

	struct PCM
	{
		constexpr PCM(): control(0), enabled(0), cur_ch(0), bank(0) { }
		uchar control; // reg7
		uchar enabled; // reg8
		uchar cur_ch;
		uchar bank;

		struct Channel // 08, size 0x10
		{
			constexpr Channel(): regs CXX11_INIT_LIST({0}), addr(0) { }
			uchar regs[8];
			uint  addr;	// .08: played sample address
		} ch[8];
	};
	PCM pcm;

	uchar bcramReg;
	uchar audioTrack;
	fbool subResetPending;
	fbool delayedDMNA;
};

extern SegaCD sCD;

// Pre-formatted internal BRAM
static const uchar fmtBram[4*0x10] =
{
	0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x00, 0x00, 0x00, 0x00, 0x40,
	0x00, 0x7d, 0x00, 0x7d, 0x00, 0x7d, 0x00, 0x7d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x53, 0x45, 0x47, 0x41, 0x5f, 0x43, 0x44, 0x5f, 0x52, 0x4f, 0x4d, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x52, 0x41, 0x4d, 0x5f, 0x43, 0x41, 0x52, 0x54, 0x52, 0x49, 0x44, 0x47, 0x45, 0x5f, 0x5f, 0x5f,
	// SEGA_CD_ROM.....RAM_CARTRIDGE___
};

// Pre-formatted 64K SRAM cart
static const uchar fmt64kSram[4*0x10] =
{
	0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x00, 0x00, 0x00, 0x00, 0x40,
	0x03, 0xfd, 0x03, 0xfd, 0x03, 0xfd, 0x03, 0xfd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x53, 0x45, 0x47, 0x41, 0x5f, 0x43, 0x44, 0x5f, 0x52, 0x4f, 0x4d, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x52, 0x41, 0x4d, 0x5f, 0x43, 0x41, 0x52, 0x54, 0x52, 0x49, 0x44, 0x47, 0x45, 0x5f, 0x5f, 0x5f,
	// SEGA_CD_ROM.....RAM_CARTRIDGE___
};

extern uchar bram[0x2000];

void scd_interruptSubCpu(uint irq);
void scd_resetSubCpu();
void scd_init();
void scd_deinit();
void scd_reset();
void scd_memmap();
void scd_update();
void scd_checkDma();
int scd_saveState(uint8 *state);
int scd_loadState(uint8 *state);

int Insert_CD(const char *iso_name, int is_bin);
void Stop_CD();
