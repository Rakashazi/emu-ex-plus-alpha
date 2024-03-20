/*  gngeo a neogeo emulator
 *  Copyright (C) 2001 Peponas Mathieu
 * 
 *  This program is free software; you can redistribute it and/or modify  
 *  it under the terms of the GNU General Public License as published by   
 *  the Free Software Foundation; either version 2 of the License, or    
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 */


#include <gngeo-config.h>
#include <imagine/util/utility.h>

extern "C"
{
	#include <gngeo/emu.h>
	#include <gngeo/memory.h>
	#include <gngeo/state.h>
}

#include <z80conf.hh>

static Z80CPU<z80Desc> Z80;
uint8_t mame_z80mem[0x10000];
static const uint8_t *z80map1, *z80map2, *z80map3, *z80map4;

/* cpu interface implementation */
CLINK void cpu_z80_switchbank(uint8_t bank, uint16_t PortNo)
{
	if(bank <= 3)
		z80_bank[bank] = PortNo;
	switch (bank)
	{
		case 0:
			z80map1 = memory.rom.cpu_z80.p + (0x4000 * ((PortNo >> 8) & 0x0f));
			if ((0x4000 * ((PortNo >> 8) & 0x0f))<memory.rom.cpu_z80.size)
				memcpy(mame_z80mem + 0x8000, z80map1, 0x4000);
			break;
		case 1:
			z80map2 = memory.rom.cpu_z80.p + (0x2000 * ((PortNo >> 8) & 0x1f));
			if ((0x2000 * ((PortNo >> 8) & 0x1f))<memory.rom.cpu_z80.size)
				memcpy(mame_z80mem + 0xc000, z80map2, 0x2000);
			break;
		case 2:
			z80map3 = memory.rom.cpu_z80.p + (0x1000 * ((PortNo >> 8) & 0x3f));
			if ((0x1000 * ((PortNo >> 8) & 0x3f))<memory.rom.cpu_z80.size)
				memcpy(mame_z80mem + 0xe000, z80map3, 0x1000);
			break;
		case 3:
			z80map4 = memory.rom.cpu_z80.p + (0x0800 * ((PortNo >> 8) & 0x7f));
			if ((0x0800 * ((PortNo >> 8) & 0x7f))<memory.rom.cpu_z80.size)
				memcpy(mame_z80mem + 0xf000, z80map4, 0x0800);
			break;
	}
}

template<class T>
static int makeStateData(Stream *gzf, T &data, int mode)
{
	return mkstate_data(gzf, &data, sizeof(T), mode);
}

CLINK void cpu_z80_mkstate(Stream *gzf,int mode)
{
	PAIR dummyPair{};
	makeStateData(gzf, dummyPair, mode); // PREPC in old state version
	makeStateData(gzf, Z80.pc, mode);
	makeStateData(gzf, Z80.sp, mode);
	makeStateData(gzf, Z80.af, mode);
	makeStateData(gzf, Z80.bc, mode);
	makeStateData(gzf, Z80.de, mode);
	makeStateData(gzf, Z80.hl, mode);
	makeStateData(gzf, Z80.ix, mode);
	makeStateData(gzf, Z80.iy, mode);
	makeStateData(gzf, Z80.af2, mode);
	makeStateData(gzf, Z80.bc2, mode);
	makeStateData(gzf, Z80.de2, mode);
	makeStateData(gzf, Z80.hl2, mode);
	makeStateData(gzf, Z80.r, mode);
	makeStateData(gzf, Z80.r2, mode);
	makeStateData(gzf, Z80.iff1, mode);
	makeStateData(gzf, Z80.iff2, mode);
	makeStateData(gzf, Z80.halt, mode);
	makeStateData(gzf, Z80.im, mode);
	makeStateData(gzf, Z80.i, mode);
	uint8_t dummyByte{};
	makeStateData(gzf, dummyByte, mode); // irq_max in old state version
	makeStateData(gzf, Z80.wz, mode); // ea in old state version, now used for wz
	int dummyInt = Z80.after_ei;
	makeStateData(gzf, dummyInt, mode); // after_ei was 'int' in old state version
	Z80.after_ei = dummyInt;
	makeStateData(gzf, dummyByte, mode); // request_irq in old state version
	makeStateData(gzf, dummyByte, mode); // service_irq in old state version
	makeStateData(gzf, Z80.nmi_state, mode);
	makeStateData(gzf, Z80.irq_state, mode);
	makeStateData(gzf, dummyByte, mode); // int_state[Z80_MAXDAISY] in old state version
	makeStateData(gzf, dummyByte, mode); // 3 padding bytes in old state version
	makeStateData(gzf, dummyByte, mode);
	makeStateData(gzf, dummyByte, mode);

	mkstate_data(gzf, mame_z80mem, 0x10000, mode);
	if (mode==STREAD)
	{
		z80map1 = memory.rom.cpu_z80.p + (0x4000 * ((z80_bank[0] >> 8) & 0x0f));
		z80map2 = memory.rom.cpu_z80.p + (0x2000 * ((z80_bank[1] >> 8) & 0x1f));
		z80map3 = memory.rom.cpu_z80.p + (0x1000 * ((z80_bank[2] >> 8) & 0x3f));
		z80map4 = memory.rom.cpu_z80.p + (0x0800 * ((z80_bank[3] >> 8) & 0x7f));
	}
}

CLINK void cpu_z80_init(void)
{
	Z80.init();

	/* bank initalisation */
	z80map1 = memory.rom.cpu_z80.p + 0x8000;
	z80map2 = memory.rom.cpu_z80.p + 0xc000;
	z80map3 = memory.rom.cpu_z80.p + 0xe000;
	z80map4 = memory.rom.cpu_z80.p + 0xf000;

	z80_bank[0]=0x8000;
	z80_bank[1]=0xc000;
	z80_bank[2]=0xe000;
	z80_bank[3]=0xf000;

	memcpy(mame_z80mem, memory.rom.cpu_z80.p, 0xf800);
	Z80.reset();
}

CLINK void cpu_z80_run(int cycles)
{
	Z80.cycleCount = 0;
	Z80.run(cycles);
}

CLINK void cpu_z80_nmi(void)
{
	Z80.setNmiLine(ASSERT_LINE);
	Z80.setNmiLine(CLEAR_LINE);
}

CLINK void cpu_z80_raise_irq(int l)
{
	Z80.irq_state = ASSERT_LINE;
}

CLINK void cpu_z80_lower_irq(void)
{
	Z80.irq_state = CLEAR_LINE;
}

uint16_t cpu_z80_get_pc(void)
{
	return 0;
}
