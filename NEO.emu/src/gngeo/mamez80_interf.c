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


#ifdef HAVE_CONFIG_H
#include <gngeo-config.h>
#endif

#ifdef USE_MAMEZ80

#include "emu.h"
#include "memory.h"
#include "mamez80/z80.h"
#include "state.h"

static Uint8 *z80map1, *z80map2, *z80map3, *z80map4;

Uint8 mame_z80mem[0x10000];

#if 0
/* Memory and port IO handler */
void mame_z80_writemem16(Uint16 addr, Uint8 val)
{
    //  printf("Writemem %x=%x\n",addr,val);
    if (addr >= 0xf800)
	memory.z80_ram[addr - 0xf800] = val;

}

Uint8 mame_z80_readmem16(Uint16 addr)
{
    if (addr <= 0x7fff)
	return memory.rom.cpu_z80.p[addr];
    if (addr <= 0xbfff)
	return z80map1[addr - 0x8000];
    if (addr <= 0xdfff)
	return z80map2[addr - 0xc000];
    if (addr <= 0xefff)
	return z80map3[addr - 0xe000];
    if (addr <= 0xf7ff)
	return z80map4[addr - 0xf000];
    return memory.z80_ram[addr - 0xf800];
}


Uint8 mame_z80_readop(Uint16 addr)
{
    return mame_z80_readmem16(addr);
}

Uint8 mame_z80_readop_arg(Uint16 addr)
{
    return mame_z80_readmem16(addr);
}
#endif

void mame_z80_writeport16(Uint16 port, Uint8 value)
{
    //printf("Write port %d=%d\n",port,value);
    z80_port_write(port, value);
}

Uint8 mame_z80_readport16(Uint16 port)
{
    //printf("Read port %d\n",port);
    return z80_port_read(port);
}


/* cpu interface implementation */
void cpu_z80_switchbank(Uint8 bank, Uint16 PortNo)
{
    if (bank<=3)
	z80_bank[bank]=PortNo;

    switch (bank) {
    case 0:
		z80map1 = memory.rom.cpu_z80.p + (0x4000 * ((PortNo >> 8) & 0x0f));
		if ((0x4000 * ((PortNo >> 8) & 0x0f))<memory.rom.cpu_z80.size)
#ifdef GP2X
	memcpy(mame_z80mem + 0x8000, z80map1, 0x4000);
#else
	memcpy(mame_z80mem + 0x8000, z80map1, 0x4000);
#endif
	break;
    case 1:
	z80map2 = memory.rom.cpu_z80.p + (0x2000 * ((PortNo >> 8) & 0x1f));
		if ((0x2000 * ((PortNo >> 8) & 0x1f))<memory.rom.cpu_z80.size)
#ifdef GP2X
	memcpy(mame_z80mem + 0xc000, z80map2, 0x2000);
#else
	memcpy(mame_z80mem + 0xc000, z80map2, 0x2000);
#endif
	break;
    case 2:
	z80map3 = memory.rom.cpu_z80.p + (0x1000 * ((PortNo >> 8) & 0x3f));
	if ((0x1000 * ((PortNo >> 8) & 0x3f))<memory.rom.cpu_z80.size)
#ifdef GP2X
	memcpy(mame_z80mem + 0xe000, z80map3, 0x1000);
#else
	memcpy(mame_z80mem + 0xe000, z80map3, 0x1000);
#endif
	break;
    case 3:
	z80map4 = memory.rom.cpu_z80.p + (0x0800 * ((PortNo >> 8) & 0x7f));
	if ((0x0800 * ((PortNo >> 8) & 0x7f))<memory.rom.cpu_z80.size)
#ifdef GP2X
	memcpy(mame_z80mem + 0xf000, z80map3, 0x0800);
#else
	memcpy(mame_z80mem + 0xf000, z80map4, 0x0800);
#endif
	break;
    }
}

int mame_z80_irq_callback(int a)
{
    return 0;
}

void cpu_z80_mkstate(gzFile gzf,int mode) {
	mkstate_data(gzf, z80_stateData(), z80_stateDataSize, mode);
	mkstate_data(gzf, mame_z80mem, 0x10000, mode);
	if (mode==STREAD) {
		int i;
		for (i = 0; i < 4; i++) {
			cpu_z80_switchbank(i, z80_bank[i]);
		}
//		memcpy(mame_z80mem + 0xf800, memory.z80_ram, 0x800);
	}
}

void cpu_z80_init(void)
{
    //  init_mamez80_mem();
    z80_init();

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
    z80_reset(NULL);
    z80_set_irq_callback(mame_z80_irq_callback);
}

void cpu_z80_run(int nbcycle)
{
    //printf("%x\n",z80_get_reg(Z80_PC));
    z80_execute(nbcycle);
}
void cpu_z80_nmi(void)
{
    //z80_set_irq_line(IRQ_LINE_NMI, 1/*PULSE_LINE- INTERNAL_CLEAR_LINE*/);
    z80_set_irq_line(IRQ_LINE_NMI, ASSERT_LINE);
    z80_set_irq_line(IRQ_LINE_NMI, CLEAR_LINE);
}
void cpu_z80_raise_irq(int l)
{
    z80_set_irq_line(l, ASSERT_LINE);
}
void cpu_z80_lower_irq(void)
{
    z80_set_irq_line(0, CLEAR_LINE);
}

Uint16 cpu_z80_get_pc(void)
{
    return 0;
}

#endif
