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
#include <config.h>
#endif

#ifdef USE_MAMEZ80

#include "emu.h"
#include "memory.h"
#include "mamez80/z80.h"
#include "state.h"

static Uint8 *z80map1, *z80map2, *z80map3, *z80map4;

Uint8 mame_z80mem[0x10000];
//static Z80_STATE z80_st;

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

//static void* mz80_context;

#if 0
static void pre_save_state(void) {
    //z80_get_context(mz80_context);
    z80_st.PC=z80_get_reg(Z80_PC);
    z80_st.SP=z80_get_reg(Z80_SP);
    z80_st.AF=z80_get_reg(Z80_AF);
    z80_st.BC=z80_get_reg(Z80_BC);
    z80_st.DE=z80_get_reg(Z80_DE);
    z80_st.HL=z80_get_reg(Z80_HL);
    z80_st.IX=z80_get_reg(Z80_IX);
    z80_st.IY=z80_get_reg(Z80_IY);

    z80_st.AF2=z80_get_reg(Z80_AF2);
    z80_st.BC2=z80_get_reg(Z80_BC2);
    z80_st.DE2=z80_get_reg(Z80_DE2);
    z80_st.HL2=z80_get_reg(Z80_HL2);
    
    z80_st.IFF1=z80_get_reg(Z80_IFF1);
    z80_st.IFF2=z80_get_reg(Z80_IFF2);
    z80_st.IM=  z80_get_reg(Z80_IM);

    //memcpy(memory.z80_ram,mame_z80mem+0xf800,0x800);

}

static void post_load_state(void) {
    int i;
    //z80_set_context(mz80_context);
    z80_set_reg(Z80_PC,z80_st.PC);
    z80_set_reg(Z80_SP,z80_st.SP);
    z80_set_reg(Z80_AF,z80_st.AF);
    z80_set_reg(Z80_BC,z80_st.BC);
    z80_set_reg(Z80_DE,z80_st.DE);
    z80_set_reg(Z80_HL,z80_st.HL);
    z80_set_reg(Z80_IX,z80_st.IX);
    z80_set_reg(Z80_IY,z80_st.IY);

    z80_set_reg(Z80_AF2,z80_st.AF2);
    z80_set_reg(Z80_BC2,z80_st.BC2);
    z80_set_reg(Z80_DE2,z80_st.DE2);
    z80_set_reg(Z80_HL2,z80_st.HL2);
    
    z80_set_reg(Z80_IFF1,z80_st.IFF1);
    z80_set_reg(Z80_IFF2,z80_st.IFF2);
    z80_set_reg(Z80_IM,z80_st.IM);

    /*for (i=0;i<4;i++) {
	cpu_z80_switchbank(i,z80_bank[i]);
    }
    memcpy(mame_z80mem+0xf800,memory.z80_ram,0x800);*/
}

static void z80_init_save_state(void) {
    /*
      int size=z80_get_context(NULL);
      mz80_context=(void*)malloc(size);
    */
    create_state_register(ST_Z80,"pc",1,(void *)&z80_st.PC,sizeof(Uint16),REG_UINT16);
    create_state_register(ST_Z80,"sp",1,(void *)&z80_st.SP,sizeof(Uint16),REG_UINT16);
    create_state_register(ST_Z80,"af",1,(void *)&z80_st.AF,sizeof(Uint16),REG_UINT16);
    create_state_register(ST_Z80,"bc",1,(void *)&z80_st.BC,sizeof(Uint16),REG_UINT16);
    create_state_register(ST_Z80,"de",1,(void *)&z80_st.DE,sizeof(Uint16),REG_UINT16);
    create_state_register(ST_Z80,"hl",1,(void *)&z80_st.HL,sizeof(Uint16),REG_UINT16);
    create_state_register(ST_Z80,"ix",1,(void *)&z80_st.IX,sizeof(Uint16),REG_UINT16);
    create_state_register(ST_Z80,"iy",1,(void *)&z80_st.IY,sizeof(Uint16),REG_UINT16);

    create_state_register(ST_Z80,"af2",1,(void *)&z80_st.AF2,sizeof(Uint16),REG_UINT16);
    create_state_register(ST_Z80,"bc2",1,(void *)&z80_st.BC2,sizeof(Uint16),REG_UINT16);
    create_state_register(ST_Z80,"de2",1,(void *)&z80_st.DE2,sizeof(Uint16),REG_UINT16);
    create_state_register(ST_Z80,"hl2",1,(void *)&z80_st.HL2,sizeof(Uint16),REG_UINT16);

    create_state_register(ST_Z80,"iff1",1,(void *)&z80_st.IFF1,sizeof(Uint8),REG_UINT8);
    create_state_register(ST_Z80,"iff2",1,(void *)&z80_st.IFF2,sizeof(Uint8),REG_UINT8);
    create_state_register(ST_Z80,"im",1,(void *)&z80_st.IM,sizeof(Uint8),REG_UINT8);


    create_state_register(ST_Z80,"bank",1,(void *)z80_bank,sizeof(Uint16)*4,REG_UINT16);
    create_state_register(ST_Z80,"z80_ram",1,(void *)memory.z80_ram,sizeof(Uint8)*0x800,REG_UINT8);
    
    set_post_load_function(ST_Z80,post_load_state);
    set_pre_save_function(ST_Z80,pre_save_state);
}
#endif

void cpu_z80_mkstate(gzFile gzf,int mode) {
	mkstate_data(gzf, z80_stateData(), z80_stateDataSize, mode);
	/*if (mode==STWRITE)
	{
		uint8 val8;
		uint16 val16;
		val16=z80_get_reg(Z80_PC); mkstate_data(gzf, &val16, sizeof(uint16), mode);
		val16=z80_get_reg(Z80_SP); mkstate_data(gzf, &val16, sizeof(uint16), mode);
		val16=z80_get_reg(Z80_AF); mkstate_data(gzf, &val16, sizeof(uint16), mode);
		val16=z80_get_reg(Z80_BC); mkstate_data(gzf, &val16, sizeof(uint16), mode);
		val16=z80_get_reg(Z80_DE); mkstate_data(gzf, &val16, sizeof(uint16), mode);
		val16=z80_get_reg(Z80_HL); mkstate_data(gzf, &val16, sizeof(uint16), mode);
		val16=z80_get_reg(Z80_IX); mkstate_data(gzf, &val16, sizeof(uint16), mode);
		val16=z80_get_reg(Z80_IY); mkstate_data(gzf, &val16, sizeof(uint16), mode);

		val16=z80_get_reg(Z80_AF2); mkstate_data(gzf, &val16, sizeof(uint16), mode);
		val16=z80_get_reg(Z80_BC2); mkstate_data(gzf, &val16, sizeof(uint16), mode);
		val16=z80_get_reg(Z80_DE2); mkstate_data(gzf, &val16, sizeof(uint16), mode);
		val16=z80_get_reg(Z80_HL2); mkstate_data(gzf, &val16, sizeof(uint16), mode);

		val8=z80_get_reg(Z80_IFF1); mkstate_data(gzf, &val8, sizeof(uint8), mode);
		val8=z80_get_reg(Z80_IFF2); mkstate_data(gzf, &val8, sizeof(uint8), mode);
		val8=  z80_get_reg(Z80_IM); mkstate_data(gzf, &val8, sizeof(uint8), mode);
	}
	else
	{
		uint8 val8;
		uint16 val16;
		mkstate_data(gzf, &val16, sizeof(uint16), mode); z80_set_reg(Z80_PC,val16);
		mkstate_data(gzf, &val16, sizeof(uint16), mode); z80_set_reg(Z80_SP,val16);
		mkstate_data(gzf, &val16, sizeof(uint16), mode); z80_set_reg(Z80_AF,val16);
		mkstate_data(gzf, &val16, sizeof(uint16), mode); z80_set_reg(Z80_BC,val16);
		mkstate_data(gzf, &val16, sizeof(uint16), mode); z80_set_reg(Z80_DE,val16);
		mkstate_data(gzf, &val16, sizeof(uint16), mode); z80_set_reg(Z80_HL,val16);
		mkstate_data(gzf, &val16, sizeof(uint16), mode); z80_set_reg(Z80_IX,val16);
		mkstate_data(gzf, &val16, sizeof(uint16), mode); z80_set_reg(Z80_IY,val16);

		mkstate_data(gzf, &val16, sizeof(uint16), mode); z80_set_reg(Z80_AF2,val16);
		mkstate_data(gzf, &val16, sizeof(uint16), mode); z80_set_reg(Z80_BC2,val16);
		mkstate_data(gzf, &val16, sizeof(uint16), mode); z80_set_reg(Z80_DE2,val16);
		mkstate_data(gzf, &val16, sizeof(uint16), mode); z80_set_reg(Z80_HL2,val16);

		mkstate_data(gzf, &val8, sizeof(uint8), mode); z80_set_reg(Z80_IFF1,val8);
		mkstate_data(gzf, &val8, sizeof(uint8), mode); z80_set_reg(Z80_IFF2,val8);
		mkstate_data(gzf, &val8, sizeof(uint8), mode); z80_set_reg(Z80_IM,val8);
	}*/
	//mkstate_data(gzf, &z80_st, sizeof (z80_st), mode);
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
    //z80_init_save_state();
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
