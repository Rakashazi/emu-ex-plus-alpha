#include	"..\DLL\d_iNES.h"
#include	"..\Hardware\h_OneBus.h"

namespace {
void	sync () {
	OneBus::syncPRG(0x0FFF, 0);
	OneBus::syncCHR(0x7FFF, 0);
	OneBus::syncMirror();
}

static const uint8_t ppuMangle[16][6] = {
	{ 0, 1, 2, 3, 4, 5 }, 	// Submapper 0: Normal
	{ 1, 0, 5, 4, 3, 2 }, 	// Submapper 1: Waixing VT03
	{ 0, 1, 2, 3, 4, 5 }, 	// Submapper 2: Trump Grand
	{ 5, 4, 3, 2, 0, 1 }, 	// Submapper 3: Zechess
	{ 2, 5, 0, 4, 3, 1 }, 	// Submapper 4: Qishenglong
	{ 1, 0, 5, 4, 3, 2 }, 	// Submapper 5: Waixing VT02
	{ 0, 1, 2, 3, 4, 5 }, 	// Submapper 6: unused so far
	{ 0, 1, 2, 3, 4, 5 }, 	// Submapper 7: unused so far
	{ 0, 1, 2, 3, 4, 5 }, 	// Submapper 8: unused so far
	{ 0, 1, 2, 3, 4, 5 }, 	// Submapper 9: unused so far
	{ 0, 1, 2, 3, 4, 5 }, 	// Submapper A: unused so far
	{ 0, 1, 2, 3, 4, 5 }, 	// Submapper B: unused so far
	{ 0, 1, 2, 3, 4, 5 }, 	// Submapper C: unused so far
	{ 0, 1, 2, 3, 4, 5 }, 	// Submapper D: Cube Tech (CPU opcode encryption only)
	{ 0, 1, 2, 3, 4, 5 }, 	// Submapper E: Karaoto (CPU opcode encryption only)
	{ 0, 1, 2, 3, 4, 5 }  	// Submapper F: Jungletac (CPU opcode encryption only)
};

static const uint8_t cpuMangle[16][4] = {
	{ 0, 1, 2, 3 }, 	// Submapper 0: Normal
	{ 0, 1, 2, 3 }, 	// Submapper 1: Waixing VT03
	{ 1, 0, 2, 3 }, 	// Submapper 2: Trump Grand
	{ 0, 1, 2, 3 }, 	// Submapper 3: Zechess
	{ 0, 1, 2, 3 }, 	// Submapper 4: Qishenglong
	{ 0, 1, 2, 3 }, 	// Submapper 5: Waixing VT02
	{ 0, 1, 2, 3 }, 	// Submapper 6: unused so far
	{ 0, 1, 2, 3 }, 	// Submapper 7: unused so far
	{ 0, 1, 2, 3 }, 	// Submapper 8: unused so far
	{ 0, 1, 2, 3 }, 	// Submapper 9: unused so far
	{ 0, 1, 2, 3 }, 	// Submapper A: unused so far
	{ 0, 1, 2, 3 }, 	// Submapper B: unused so far
	{ 0, 1, 2, 3 }, 	// Submapper C: unused so far
	{ 0, 1, 2, 3 }, 	// Submapper D: Cube Tech (CPU opcode encryption only)
	{ 0, 1, 2, 3 }, 	// Submapper E: Karaoto (CPU opcode encryption only)
	{ 0, 1, 2, 3 }  	// Submapper F: Jungletac (CPU opcode encryption only)
};

// Note that OneBus::WriteMMC3 calls its own WriteHandler for Banks 2 and 4, not the Mapper256's.
// So if the 2012-2017 registers are mangled but the MMC3 registers are unmangled, MMC3Mangle is 0,1,2,3,4,5,6,7.
static const uint8_t mmc3Mangle[16][8] = {
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, 	// Submapper 0: Normal
	{ 5, 4, 3, 2, 1, 0, 6, 7 }, 	// Submapper 1: Waixing VT03
	{ 0, 1, 2, 3, 4, 5, 7, 6 }, 	// Submapper 2: Trump Grand
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, 	// Submapper 3: Zechess
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, 	// Submapper 4: Qishenglong
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, 	// Submapper 5: Waixing VT02
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, 	// Submapper 6: unused so far
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, 	// Submapper 7: unused so far
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, 	// Submapper 8: unused so far
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, 	// Submapper 9: unused so far
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, 	// Submapper A: unused so far
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, 	// Submapper B: unused so far
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, 	// Submapper C: unused so far
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, 	// Submapper D: Cube Tech (CPU opcode encryption only)
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, 	// Submapper E: Karaoto (CPU opcode encryption only)
	{ 0, 1, 2, 3, 4, 5, 6, 7 }  	// Submapper F: Jungletac (CPU opcode encryption only)
};

void	MAPINT	write2 (int bank, int addr, int val) {
	if (addr >=0x012 && addr <=0x017) addr =0x012 +ppuMangle[ROM->INES2_SubMapper][addr -0x012];
	OneBus::writePPU(bank, addr, val);
}

void	MAPINT	write4 (int bank, int addr, int val) {
	if (addr >=0x107 && addr <=0x10A) addr =0x107 +cpuMangle[ROM->INES2_SubMapper][addr -0x107];
	OneBus::writeAPU(bank, addr, val);
}

void	MAPINT	write8 (int bank, int addr, int val) {
	if (bank <=0x9 && ~addr &1) val =val &0xF8 | mmc3Mangle[ROM->INES2_SubMapper][val &0x07];
	OneBus::writeMMC3(bank, addr, val);
}

BOOL	MAPINT	load (void) {
	iNES_SetSRAM();
	OneBus::load(sync);
	return TRUE;
}

void	MAPINT	reset (RESET_TYPE resetType) {
	OneBus::reset(resetType);
	
	EMU->SetCPUWriteHandler(0x2, write2);
	EMU->SetCPUWriteHandler(0x4, write4);
	for (int bank =0x8; bank<=0x9; bank++) EMU->SetCPUWriteHandler(bank, write8);
}

uint16_t mapperNum =256;
} // namespace


MapperInfo MapperInfo_256 = {
	&mapperNum,
	_T("OneBus"),
	COMPAT_FULL,
	load,
	reset,
	OneBus::unload,
	OneBus::cpuCycle,
	OneBus::ppuCycle,
	OneBus::saveLoad,
	NULL,
	NULL
};
