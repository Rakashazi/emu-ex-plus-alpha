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

#ifdef USE_DRZ80
#include "emu.h"
#include "memory.h"
#include "drz80/DrZ80.h"
#include "state.h"
#ifdef GP2X
#include "ym2610-940/940shared.h"
#endif

#ifndef ENABLE_940T

static Uint8 *z80map1, *z80map2, *z80map3, *z80map4;
Uint8 drz80mem[0x10000];
Uint32 mydrz80_Z80PC,mydrz80_Z80SP;

struct DrZ80 mydrz80;
//extern Z80_Regs Z80;

unsigned int drz80_rebasePC(unsigned short address)
{
	//if (address==0x66) 
	printf("Rebase PC %x\n",address);
        mydrz80.Z80PC_BASE = (unsigned int)drz80mem;
	mydrz80.Z80PC = mydrz80.Z80PC_BASE + address;
        return mydrz80.Z80PC_BASE + address;
}

unsigned int drz80_rebaseSP(unsigned short address)
{
	printf("Rebase SP %x\n",address);
        mydrz80.Z80SP_BASE = (unsigned int)drz80mem;
	mydrz80.Z80SP = mydrz80.Z80SP_BASE + address;
	return mydrz80.Z80SP_BASE + address;
}

unsigned char drz80_read8(unsigned short address) {
        return (drz80mem[address&0xFFFF]);
}

unsigned short drz80_read16(unsigned short address) {
        return drz80_read8(address) | (drz80_read8(address + 1) << 8);
}

void drz80_write8(unsigned char data,unsigned short address) {
        if (address>=0xf800) drz80mem[address&0xFFFF]=data;
}

void drz80_write16(unsigned short data,unsigned short address) {
        drz80_write8(data & 0xFF,address);
        drz80_write8(data >> 8,address + 1);
}

void drz80_writeport16(Uint16 port, Uint8 value)
{
	printf("Write port %d=%d\n",port,value);
    z80_port_write(port, value);
}

Uint8 drz80_readport16(Uint16 port)
{
	printf("Read port %d\n",port);
    return z80_port_read(port);
}

/* cpu interface implementation */
void cpu_z80_switchbank(Uint8 bank, Uint16 PortNo)
{
	printf("Switch bank %x %x\n",bank,PortNo);
    if (bank<=3)
	z80_bank[bank]=PortNo;

    switch (bank) {
    case 0:
	z80map1 = memory.rom.cpu_z80.p + (0x4000 * ((PortNo >> 8) & 0x0f));
	memcpy(drz80mem + 0x8000, z80map1, 0x4000);
	break;
    case 1:
	z80map2 = memory.rom.cpu_z80.p + (0x2000 * ((PortNo >> 8) & 0x1f));
	memcpy(drz80mem + 0xc000, z80map2, 0x2000);
	break;
    case 2:
	z80map3 = memory.rom.cpu_z80.p + (0x1000 * ((PortNo >> 8) & 0x3f));
	memcpy(drz80mem + 0xe000, z80map3, 0x1000);
	break;
    case 3:
	z80map4 = memory.rom.cpu_z80.p + (0x0800 * ((PortNo >> 8) & 0x7f));
	memcpy(drz80mem + 0xf000, z80map4, 0x0800);
	break;
    }
}

void drz80_irq_callback(void)
{
        //if (mydrz80.Z80_IRQ ==0x2) 
	//mydrz80.Z80_IRQ = 0x00;
	//printf("Irq have been accepted %x %x\n",mydrz80.Z80_IRQ,mydrz80.Z80IF);
}
static void pre_save_state(void) {

    memcpy(memory.z80_ram,drz80mem+0xf800,0x800);
    mydrz80_Z80PC=mydrz80.Z80PC-mydrz80.Z80PC_BASE;
    mydrz80_Z80SP=mydrz80.Z80SP-mydrz80.Z80SP_BASE;
}

static void post_load_state(void) {
    int i;

    mydrz80.z80_rebasePC=drz80_rebasePC;
    mydrz80.z80_rebaseSP=drz80_rebaseSP;
    mydrz80.z80_read8   =drz80_read8;
    mydrz80.z80_read16  =drz80_read16;
    mydrz80.z80_write8  =drz80_write8;
    mydrz80.z80_write16 =drz80_write16;
    mydrz80.z80_in      =drz80_readport16; /*z80_in*/
    mydrz80.z80_out     =drz80_writeport16; /*z80_out*/

    drz80_rebasePC(mydrz80_Z80PC);
    drz80_rebaseSP(mydrz80_Z80SP);

    for (i=0;i<4;i++) {
	cpu_z80_switchbank(i,z80_bank[i]);
    }
    memcpy(drz80mem+0xf800,memory.z80_ram,0x800);
    
}

static void z80_init_save_state(void) {

	create_state_register(ST_Z80,"drz80",1,(void *)&mydrz80,sizeof(mydrz80),REG_UINT8);
	create_state_register(ST_Z80,"pc",1,(void *)&mydrz80_Z80PC,sizeof(Uint16),REG_UINT32);
	create_state_register(ST_Z80,"sp",1,(void *)&mydrz80_Z80SP,sizeof(Uint16),REG_UINT32);
	create_state_register(ST_Z80,"bank",1,(void *)z80_bank,sizeof(Uint16)*4,REG_UINT16);
	create_state_register(ST_Z80,"z80_ram",1,(void *)memory.z80_ram,sizeof(Uint8)*0x800,REG_UINT8);
    
    set_post_load_function(ST_Z80,post_load_state);
    set_pre_save_function(ST_Z80,pre_save_state);
}
void cpu_z80_mkstate(gzFile gzf,int mode) {
	/* TODO */
}
void cpu_z80_init(void)
{
        memset (&mydrz80, 0, sizeof(mydrz80));
        mydrz80.z80_rebasePC=drz80_rebasePC;
        mydrz80.z80_rebaseSP=drz80_rebaseSP;
        mydrz80.z80_read8   =drz80_read8;
        mydrz80.z80_read16  =drz80_read16;
        mydrz80.z80_write8  =drz80_write8;
        mydrz80.z80_write16 =drz80_write16;
        mydrz80.z80_in      =drz80_readport16; /*z80_in*/
        mydrz80.z80_out     =drz80_writeport16; /*z80_out*/
        //mydrz80.z80_irq_callback=drz80_irq_callback;
        //mydrz80.Z80A = 0x00 <<24;
        mydrz80.Z80F = (1<<2); /* set ZFlag */
        //mydrz80.Z80BC = 0x0000 <<16;
        //mydrz80.Z80DE = 0x0000 <<16;
        //mydrz80.Z80HL = 0x0000 <<16;
        //mydrz80.Z80A2 = 0x00 <<24;
        mydrz80.Z80F2 = 1<<2;  /* set ZFlag */
        //mydrz80.Z80BC2 = 0x0000 <<16;
        //mydrz80.Z80DE2 = 0x0000 <<16;
        //mydrz80.Z80HL2 = 0x0000 <<16;
        mydrz80.Z80IX = 0xFFFF;// <<16;
        mydrz80.Z80IY = 0xFFFF;// <<16;
        mydrz80.Z80I = 0x00;
        mydrz80.Z80IM = 0x01;
        mydrz80.Z80_IRQ = 0x00;
        mydrz80.Z80IF = 0x00;
        mydrz80.Z80PC=mydrz80.z80_rebasePC(0);
       // mydrz80.Z80SP=mydrz80.z80_rebaseSP(0xffff);/*0xf000;*/

/* bank initalisation */
	z80map1 = memory.rom.cpu_z80.p + 0x8000;
	z80map2 = memory.rom.cpu_z80.p + 0xc000;
	z80map3 = memory.rom.cpu_z80.p + 0xe000;
	z80map4 = memory.rom.cpu_z80.p + 0xf000;
	
	z80_bank[0]=0x8000;
	z80_bank[1]=0xc000;
	z80_bank[2]=0xe000;
	z80_bank[3]=0xf000;
	
	memcpy(drz80mem, memory.rom.cpu_z80.p, 0xf800);
	//z80_init_save_state();
}
void cpu_z80_run(int nbcycle) {
	//printf("Drz80run %d\n",nbcycle);
	DrZ80Run(&mydrz80, nbcycle);
}
/*
#define PUSH_PC() { mydrz80.Z80SP-=2; mydrz80.z80_write16(mydrz80.Z80PC - mydrz80.Z80PC_BASE,mydrz80.Z80SP); }
*/

void cpu_z80_nmi(void)
{
	//printf("Cause NMI %x %x\n",mydrz80.Z80_IRQ,mydrz80.Z80IF);
	mydrz80.Z80_IRQ |= 0x02;
	//DrZ80Run(&mydrz80, 30);
}
void cpu_z80_raise_irq(int l)
{
	//printf("raise irq %x %x\n",mydrz80.Z80_IRQ,mydrz80.Z80IF);
	mydrz80.Z80_IRQ |= 0x1;
	//mydrz80.z80irqvector= l & 0xff;
}
void cpu_z80_lower_irq(void)
{
	//printf("Lower irq %x %x\n",mydrz80.Z80_IRQ,mydrz80.Z80IF);
	mydrz80.Z80_IRQ &= ~0x1;
}
#endif /* ENABLE_940T */
#endif
