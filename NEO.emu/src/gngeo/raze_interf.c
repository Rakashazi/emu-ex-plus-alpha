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

#ifdef USE_RAZE
#include "emu.h"
#include "memory.h"
#include "raze/raze.h"
//#include "2610intf.h"



void cpu_z80_switchbank(Uint8 bank, Uint16 PortNo)
{
    if (bank<3)
	z80_bank[bank]=PortNo;

    switch (bank) {
    case 0:
	z80_map_fetch(0x8000, 0xbfff,
		      memory.rom.cpu_z80.p + (0x4000 * ((PortNo >> 8) & 0x0f)));
	z80_map_read(0x8000, 0xbfff,
		     memory.rom.cpu_z80.p + (0x4000 * ((PortNo >> 8) & 0x0f)));
	break;
    case 1:
	z80_map_fetch(0xc000, 0xdfff,
		      memory.rom.cpu_z80.p + (0x2000 * ((PortNo >> 8) & 0x1f)));
	z80_map_read(0xc000, 0xdfff,
		     memory.rom.cpu_z80.p + (0x2000 * ((PortNo >> 8) & 0x1f)));
	break;
    case 2:
	z80_map_fetch(0xe000, 0xefff,
		      memory.rom.cpu_z80.p + (0x1000 * ((PortNo >> 8) & 0x3f)));
	z80_map_read(0xe000, 0xefff,
		     memory.rom.cpu_z80.p + (0x1000 * ((PortNo >> 8) & 0x3f)));
	break;
    case 3:
	z80_map_fetch(0xf000, 0xf7ff,
		      memory.rom.cpu_z80.p + (0x0800 * ((PortNo >> 8) & 0x7f)));
	z80_map_read(0xf000, 0xf7ff,
		     memory.rom.cpu_z80.p + (0x0800 * ((PortNo >> 8) & 0x7f)));
	break;
    }
}


static Z80_STATE z80_st;
#if 0
static void* raze_context=NULL;

static void z80dumpreg(void){
    printf("PC %04x\n",z80_get_reg(Z80_REG_PC));
    printf("SP %04x\n",z80_get_reg(Z80_REG_SP));
    printf("AF %04x\n",z80_get_reg(Z80_REG_AF));
    printf("BC %04x\n",z80_get_reg(Z80_REG_BC));
    printf("DE %04x\n",z80_get_reg(Z80_REG_DE));
    printf("HL %04x\n",z80_get_reg(Z80_REG_HL));
    printf("IX %04x\n",z80_get_reg(Z80_REG_IX));
    printf("IY %04x\n",z80_get_reg(Z80_REG_IY));

    printf("AF2 %04x\n",z80_get_reg(Z80_REG_AF2));
    printf("BC2 %04x\n",z80_get_reg(Z80_REG_BC2));
    printf("DE2 %04x\n",z80_get_reg(Z80_REG_DE2));
    printf("HL2 %04x\n",z80_get_reg(Z80_REG_HL2));
    
    printf("IFF1 %04x\n",z80_get_reg(Z80_REG_IFF1));
    printf("IFF2 %04x\n",z80_get_reg(Z80_REG_IFF2));
    printf("IM %04x\n",z80_get_reg(Z80_REG_IM));

    printf("IRQV %04x\n",z80_get_reg(Z80_REG_IRQVector));
    printf("IRQL %04x\n",z80_get_reg(Z80_REG_IRQLine));
}

#endif
void cpu_z80_post_load_state(void) 
{
    int i;

#if 1
    z80_set_reg(Z80_REG_PC,z80_st.PC);
    z80_set_reg(Z80_REG_SP,z80_st.SP);
    z80_set_reg(Z80_REG_AF,z80_st.AF);
    z80_set_reg(Z80_REG_BC,z80_st.BC);
    z80_set_reg(Z80_REG_DE,z80_st.DE);
    z80_set_reg(Z80_REG_HL,z80_st.HL);
    z80_set_reg(Z80_REG_IX,z80_st.IX);
    z80_set_reg(Z80_REG_IY,z80_st.IY);

    z80_set_reg(Z80_REG_AF2,z80_st.AF2);
    z80_set_reg(Z80_REG_BC2,z80_st.BC2);
    z80_set_reg(Z80_REG_DE2,z80_st.DE2);
    z80_set_reg(Z80_REG_HL2,z80_st.HL2);
    
//    z80_set_reg(Z80_REG_R,z80_st.R);
//    z80_set_reg(Z80_REG_R2,z80_st.R2);
    z80_set_reg(Z80_REG_IFF1,z80_st.IFF1);
    z80_set_reg(Z80_REG_IFF2,z80_st.IFF2);
    z80_set_reg(Z80_REG_IM,z80_st.IM);
    z80_set_reg(Z80_REG_IRQVector,z80_st.IRQV);
    z80_set_reg(Z80_REG_IRQLine,z80_st.IRQL);
//    z80_set_reg(Z80_REG_I,z80_st.I);
#endif
/*
    z80_reset();
    z80_set_context(raze_context);
*/

    for (i=0;i<4;i++) {
	cpu_z80_switchbank(i,z80_bank[i]);
    }

//    z80dumpreg();
}



void cpu_z80_pre_save_state(void) {
    int i;
/*
    z80_get_context(raze_context);
    z80dumpreg();
*/
    //printf("ram[0]=%02x\n",memory.z80_ram[0]);
#if 1
    z80_st.PC=z80_get_reg(Z80_REG_PC);
    z80_st.SP=z80_get_reg(Z80_REG_SP);
    z80_st.AF=z80_get_reg(Z80_REG_AF);
    z80_st.BC=z80_get_reg(Z80_REG_BC);
    z80_st.DE=z80_get_reg(Z80_REG_DE);
    z80_st.HL=z80_get_reg(Z80_REG_HL);
    z80_st.IX=z80_get_reg(Z80_REG_IX);
    z80_st.IY=z80_get_reg(Z80_REG_IY);

    z80_st.AF2=z80_get_reg(Z80_REG_AF2);
    z80_st.BC2=z80_get_reg(Z80_REG_BC2);
    z80_st.DE2=z80_get_reg(Z80_REG_DE2);
    z80_st.HL2=z80_get_reg(Z80_REG_HL2);
    
//    z80_st.R=   z80_get_reg(Z80_REG_R);
//    z80_st.R2=  z80_get_reg(Z80_REG_R2);
    z80_st.IFF1=z80_get_reg(Z80_REG_IFF1);
    z80_st.IFF2=z80_get_reg(Z80_REG_IFF2);
    z80_st.IM=  z80_get_reg(Z80_REG_IM);
//    z80_st.I=   z80_get_reg(Z80_REG_I);
    z80_st.IRQV=   z80_get_reg(Z80_REG_IRQVector);
    z80_st.IRQL=   z80_get_reg(Z80_REG_IRQLine);
#endif
	
}

void cpu_z80_init_save_state(void) {
#if 1
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
/*
    create_state_register(ST_Z80,"IRQVector",1,(void *)&z80_st.IRQV,sizeof(Uint8));
    create_state_register(ST_Z80,"IRQLine",1,(void *)&z80_st.IRQL,sizeof(Uint8));
*/
#endif
    
//    printf("size %d\n",z80_get_context_size());
/*
    raze_context=(void*)malloc(z80_get_context_size());
    create_state_register(ST_Z80,"context",1,(void *)raze_context,z80_get_context_size());
*/
    create_state_register(ST_Z80,"bank",1,(void *)z80_bank,sizeof(Uint16)*4,REG_UINT16);
    create_state_register(ST_Z80,"z80_ram",1,(void *)memory.z80_ram,sizeof(Uint8)*0x800,REG_UINT8);

    set_post_load_function(ST_Z80,cpu_z80_post_load_state);
    set_pre_save_function(ST_Z80,cpu_z80_pre_save_state);
}
#if 0
static void debug(UWORD pc)
{
    //if (pc == 0x1234) printf("Breakpoint hit!\n");
    if (pc==0x145c) {
	printf("Z %04x %02x\n",pc,memory.rom.cpu_z80.p[pc]);
	z80dumpreg();
    }
    if (pc==0x1450) {
	printf("Z %04x %02x\n",pc,memory.rom.cpu_z80.p[pc]);
	z80dumpreg();

    }
}
#endif

void cpu_z80_mkstate(gzFile gzf,int mode) {
	/* TODO */
}

void cpu_z80_init(void)
{
    z80_init_memmap();
    z80_map_fetch(0x0000, 0x7fff, memory.rom.cpu_z80.p);
    z80_map_fetch(0x8000, 0xbfff, memory.rom.cpu_z80.p + 0x8000);
    z80_map_fetch(0xc000, 0xdfff, memory.rom.cpu_z80.p + 0xc000);
    z80_map_fetch(0xe000, 0xefff, memory.rom.cpu_z80.p + 0xe000);
    z80_map_fetch(0xf000, 0xf7ff, memory.rom.cpu_z80.p + 0xf000);
    z80_map_fetch(0xf800, 0xffff, memory.z80_ram);

    z80_add_read(0x0000, 0x7fff, Z80_MAP_DIRECT, memory.rom.cpu_z80.p);
    z80_map_read(0x8000, 0xbfff, memory.rom.cpu_z80.p + 0x8000);
    z80_map_read(0xc000, 0xdfff, memory.rom.cpu_z80.p + 0xc000);
    z80_map_read(0xe000, 0xefff, memory.rom.cpu_z80.p + 0xe000);
    z80_map_read(0xf000, 0xf7ff, memory.rom.cpu_z80.p + 0xf000);
    z80_add_read(0xf800, 0xffff, Z80_MAP_DIRECT, memory.z80_ram);

    z80_bank[0]=0x8000;
    z80_bank[1]=0xc000;
    z80_bank[2]=0xe000;
    z80_bank[3]=0xf000;

    z80_add_write(0xf800, 0xffff, Z80_MAP_DIRECT, memory.z80_ram);

    z80_end_memmap();

    z80_set_in(z80_port_read);
    z80_set_out(z80_port_write);

/*    z80_set_fetch_callback(&debug);*/

    cpu_z80_init_save_state();

    z80_reset();
}

void cpu_z80_run(int nbcycle)
{
    z80_emulate(nbcycle);
}

void cpu_z80_nmi(void)
{
    z80_cause_NMI();
}

void cpu_z80_raise_irq(int l)
{
    z80_raise_IRQ(l);
}

void cpu_z80_lower_irq(void)
{
    z80_lower_IRQ();
}

Uint16 cpu_z80_get_pc(void) {
    return z80_get_reg(Z80_REG_PC);
}

void cpu_z80_set_state(Z80_STATE *st) 
{
    z80_set_reg(Z80_REG_PC,st->PC);
    z80_set_reg(Z80_REG_SP,st->SP);
    z80_set_reg(Z80_REG_AF,st->AF);
    z80_set_reg(Z80_REG_BC,st->BC);
    z80_set_reg(Z80_REG_DE,st->DE);
    z80_set_reg(Z80_REG_HL,st->HL);
    z80_set_reg(Z80_REG_IX,st->IX);
    z80_set_reg(Z80_REG_IY,st->IY);

    z80_set_reg(Z80_REG_AF2,st->AF2);
    z80_set_reg(Z80_REG_BC2,st->BC2);
    z80_set_reg(Z80_REG_DE2,st->DE2);
    z80_set_reg(Z80_REG_HL2,st->HL2);
    
//    z80_set_reg(Z80_REG_R,st->R);
//    z80_set_reg(Z80_REG_R2,st->R2);
    z80_set_reg(Z80_REG_IFF1,st->IFF1);
    z80_set_reg(Z80_REG_IFF2,st->IFF2);
    z80_set_reg(Z80_REG_IM,st->IM);
//    z80_set_reg(Z80_REG_I,st->I);
    
}


void cpu_z80_fill_state(Z80_STATE *st) {
    st->PC=z80_get_reg(Z80_REG_PC);
    st->SP=z80_get_reg(Z80_REG_SP);
    st->AF=z80_get_reg(Z80_REG_AF);
    st->BC=z80_get_reg(Z80_REG_BC);
    st->DE=z80_get_reg(Z80_REG_DE);
    st->HL=z80_get_reg(Z80_REG_HL);
    st->IX=z80_get_reg(Z80_REG_IX);
    st->IY=z80_get_reg(Z80_REG_IY);

    st->AF2=z80_get_reg(Z80_REG_AF2);
    st->BC2=z80_get_reg(Z80_REG_BC2);
    st->DE2=z80_get_reg(Z80_REG_DE2);
    st->HL2=z80_get_reg(Z80_REG_HL2);
    
//    st->R=   z80_get_reg(Z80_REG_R);
//    st->R2=  z80_get_reg(Z80_REG_R2);
    st->IFF1=z80_get_reg(Z80_REG_IFF1);
    st->IFF2=z80_get_reg(Z80_REG_IFF2);
    st->IM=  z80_get_reg(Z80_REG_IM);
//    st->I=   z80_get_reg(Z80_REG_I);
}


#endif
