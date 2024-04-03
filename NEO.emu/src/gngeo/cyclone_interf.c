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

/* cyclone interface */



#ifdef HAVE_CONFIG_H
#include <gngeo-config.h>
#endif

#include <stdlib.h>

#include "cyclone/Cyclone.h"
#include "memory.h"
#include "emu.h"
#include "state.h"
#include "debug.h"
#include "video.h"
#include "conf.h"
#include <imagine/logger/logger.h>

struct Cyclone MyCyclone;
static int total_cycles;

static void print_one_reg(Uint32 r) {
	logMsg("reg=%08x",r);
}

static void swap_memory(Uint8 * mem, Uint32 length)
{
    unsigned int i, j;

    /* swap bytes in each word */
    for (i = 0; i < length; i += 2) {
	j = mem[i];
	mem[i] = mem[i + 1];
	mem[i + 1] = j;
    }
}

static unsigned int   MyCheckPc(unsigned int pc) {
	int i;
	pc-=MyCyclone.membase; // Get the real program counter

	pc&=0xFFFFFF;
	MyCyclone.membase=-1;

/*	printf("## Check pc %08x\n",pc);
	for(i=0;i<8;i++) printf("  d%d=%08x a%d=%08x\n",i,MyCyclone.d[i],i,MyCyclone.a[i]); */

	//printf("PC %08x %08x\n",(pc&0xF00000),(pc&0xF00000)>>20);
	switch((pc&0xF00000)>>20) {
	case 0x0:
		MyCyclone.membase=(int)memory.rom.cpu_m68k.p;
		break;
	case 0x2:
		MyCyclone.membase=(int)(memory.rom.cpu_m68k.p+bankaddress)-0x200000;
		break;
	case 0x1:
		if (pc<=0x10FFff) 
			MyCyclone.membase=(int)memory.ram-0x100000;
		break;
	case 0xC:
		if (pc<=0xc1FFff)
			MyCyclone.membase=(int)memory.rom.bios_m68k.p-0xc00000;
		break;
	}

 	if (MyCyclone.membase==-1) {
 		logMsg("ERROROROOR %08x",pc);
 		exit(1);
 	}

	return MyCyclone.membase+pc; // New program counter
}

static unsigned int  MyRead8  (unsigned int a) {
	unsigned int addr=a&0xFFFFF;
	unsigned int b=((a&0xF0000)>>16);
	a&=0xFFFFFF;
	switch((a&0xFF0000)>>20) {
	case 0x0:
		return mem68k_fetch_cpu_byte(a)&0xFF;
	case 0x2:
		return mem68k_fetch_bk_normal_byte(a)&0xFF;
	case 0x1:
		return mem68k_fetch_ram_byte(a)&0xFF;
	case 0xC:
		return mem68k_fetch_bios_byte(a)&0xFF;
	case 0xd:
		return mem68k_fetch_sram_byte(a)&0xFF;
	case 0x4 ... 0x7:
		return mem68k_fetch_pal_byte(a)&0xFF;
	case 0x3:
		if (b==0xC) return mem68k_fetch_video_byte(a)&0xFF;
		if (b==0) return mem68k_fetch_ctl1_byte(a)&0xFF;
		if (b==4) return mem68k_fetch_ctl2_byte(a)&0xFF;
		if (b==8) return mem68k_fetch_ctl3_byte(a)&0xFF;
		if (b==2) return mem68k_fetch_coin_byte(a)&0xFF;
		break;
	case 0x8:
		if (b==0) return mem68k_fetch_memcrd_byte(a)&0xFF;
		break;
	}

	return mem68k_fetch_invalid_byte(a)&0xFF;
}
static unsigned int MyRead16 (unsigned int a) {
	unsigned int addr=a&0xFFFFF;
	unsigned int b=((a&0xF0000)>>16);
	//printf("read 32 %08x\n",a);
	a&=0xFFFFFF;

	switch((a&0xFF0000)>>20) {
	case 0x0:
		return mem68k_fetch_cpu_word(a)&0xFFFF;
	case 0x2:
		return mem68k_fetch_bk_normal_word(a)&0xFFFF;
	case 0x1:
		return mem68k_fetch_ram_word(a)&0xFFFF;
	case 0xC:
		return mem68k_fetch_bios_word(a)&0xFFFF;
	case 0xd:
		return mem68k_fetch_sram_word(a)&0xFFFF;
	case 0x4 ... 0x7:
		return mem68k_fetch_pal_word(a)&0xFFFF;
	case 0x3:
		if (b==0xC) return mem68k_fetch_video_word(a)&0xFFFF;
		if (b==0) return mem68k_fetch_ctl1_word(a)&0xFFFF;
		if (b==4) return mem68k_fetch_ctl2_word(a)&0xFFFF;
		if (b==8) return mem68k_fetch_ctl3_word(a)&0xFFFF;
		if (b==2) return mem68k_fetch_coin_word(a)&0xFFFF;
		break;
	case 0x8:
		if (b==0) return mem68k_fetch_memcrd_word(a)&0xFFFF;
		break;
	}

	return mem68k_fetch_invalid_word(a)&0xFFFF;
}
static unsigned int   MyRead32 (unsigned int a) {
	//int i;
	unsigned int addr=a&0xFFFFF;
	unsigned int b=((a&0xF0000)>>16);
	a&=0xFFFFFF;

	switch((a&0xFF0000)>>20) {
	case 0x0:
		return mem68k_fetch_cpu_long(a);
	case 0x2:
		return mem68k_fetch_bk_normal_long(a);
	case 0x1:
		return mem68k_fetch_ram_long(a);
	case 0xC:
		return mem68k_fetch_bios_long(a);
	case 0xd:
		return mem68k_fetch_sram_long(a);
	case 0x4 ... 0x7:
		return mem68k_fetch_pal_long(a);
	case 0x3:
		if (b==0xC) return mem68k_fetch_video_long(a);
		if (b==0) return mem68k_fetch_ctl1_long(a);
		if (b==4) return mem68k_fetch_ctl2_long(a);
		if (b==8) return mem68k_fetch_ctl3_long(a);
		if (b==2) return mem68k_fetch_coin_long(a);
		break;
	case 0x8:
		if (b==0) return mem68k_fetch_memcrd_long(a);
		break;
	}

	return mem68k_fetch_invalid_long(a);
}
static void MyWrite8 (unsigned int a,unsigned int  d) {
	unsigned int b=((a&0xF0000)>>16);
	a&=0xFFFFFF;
    d&=0xFF;
	switch((a&0xFF0000)>>20) {
	case 0x1:
		mem68k_store_ram_byte(a, d);
		return;
	case 0x3:
		if (b==0xc) {mem68k_store_video_byte(a,d);return;}
		if (b==8) {mem68k_store_pd4990_byte(a,d);return;}
		if (b==2) {mem68k_store_z80_byte(a,d);return;}
		if (b==0xA) {mem68k_store_setting_byte(a,d);return;}
		break;
	case 0x4 ... 0x7:
		mem68k_store_pal_byte(a,d);
		return;
	case 0xD:
		mem68k_store_sram_byte(a,d);
		return;
	case 0x2:
		if (b==0xF) {mem68k_store_bk_normal_byte(a,d);return;}
		break;
	case 0x8:
		if (b==0) {mem68k_store_memcrd_byte(a,d);return;}
		break;

	}

	mem68k_store_invalid_byte(a, d);
}
static void MyWrite16(unsigned int a,unsigned int d) {
	unsigned int b=((a&0xF0000)>>16);
	a&=0xFFFFFF;
    d&=0xFFFF;
    //if (d&0x8000) printf("WEIRD %x %x\n",a,d);

	switch((a&0xFF0000)>>20) {
	case 0x1:
		mem68k_store_ram_word(a, d);
		return;
	case 0x3:
		if (b==0xc) {mem68k_store_video_word(a,d);return;}
		if (b==8) {mem68k_store_pd4990_word(a,d);return;}
		if (b==2) {mem68k_store_z80_word(a,d);return;}
		if (b==0xA) {mem68k_store_setting_word(a,d);return;}
		break;	
	case 0x4 ... 0x7:
		mem68k_store_pal_word(a,d);
		return;
	case 0xD:
		mem68k_store_sram_word(a,d);
		return;
	case 0x2:
		if (b==0xF) {mem68k_store_bk_normal_word(a,d);return;}
		break;
	case 0x8:
		if (b==0) {mem68k_store_memcrd_word(a,d);return;}
		break;
	}

	mem68k_store_invalid_word(a, d);
}
static void MyWrite32(unsigned int a,unsigned int   d) {
	unsigned int b=((a&0xF0000)>>16);
	a&=0xFFFFFF;
    d&=0xFFFFFFFF;
		switch((a&0xFF0000)>>20) {
	case 0x1:
		mem68k_store_ram_long(a, d);
		return;
	case 0x3:
		if (b==0xc) {mem68k_store_video_long(a, d);return;}
		if (b==8) {mem68k_store_pd4990_long(a,d);return;}
		if (b==2) {mem68k_store_z80_long(a,d);return;}
		if (b==0xA) {mem68k_store_setting_long(a,d);return;}
		break;	
	case 0x4 ... 0x7:
		mem68k_store_pal_long(a,d);
		return;
	case 0xD:
		mem68k_store_sram_long(a,d);
		return;
	case 0x2:
		if (b==0xF) {mem68k_store_bk_normal_long(a,d);return;}
		break;
	case 0x8:
		if (b==0) {mem68k_store_memcrd_long(a,d);return;}
		break;
	}

	mem68k_store_invalid_long(a, d);
}

void cpu_68k_mkstate(Stream *gzf,int mode) {
	Uint8 save_buffer[128];
	logMsg("Save state mode %s PC=%08x",(mode==STREAD?"READ":"WRITE"),MyCyclone.pc-MyCyclone.membase);
	if (mode==STWRITE) CyclonePack(&MyCyclone, save_buffer);
	mkstate_data(gzf, save_buffer, 128, mode);
	if (mode == STREAD) CycloneUnpack(&MyCyclone, save_buffer);
	logMsg("Save state Phase 2 PC=%08x", (unsigned int)(MyCyclone.pc - MyCyclone.membase));
}
int cpu_68k_getcycle(void) {
	return total_cycles-MyCyclone.cycles;
}
void bankswitcher_init() {
	bankaddress=0;
}
int cyclone_debug(unsigned short o) {
	logMsg("CYCLONE DEBUG %04x",o);
	return 0;
}

void cpu_68k_init(void) {
	int overclk=0;//CF_VAL(cf_get_item_by_name("68kclock"));
	//printf("INIT \n");
	CycloneInit();
	memset(&MyCyclone, 0,sizeof(MyCyclone));
	/*
	swap_memory(memory.rom.cpu_m68k.p, memory.rom.cpu_m68k.size);
	swap_memory(memory.rom.bios_m68k.p, memory.rom.bios_m68k.size);
	swap_memory(memory.game_vector, 0x80);
	*/
	MyCyclone.read8=MyRead8;
	MyCyclone.read16=MyRead16;
	MyCyclone.read32=MyRead32;

	MyCyclone.write8=MyWrite8;
	MyCyclone.write16=MyWrite16;
	MyCyclone.write32=MyWrite32;

	MyCyclone.checkpc=MyCheckPc;

	MyCyclone.fetch8  =MyRead8;
        MyCyclone.fetch16 =MyRead16;
        MyCyclone.fetch32 =MyRead32;

	//MyCyclone.InvalidOpCallback=cyclone_debug;
	//MyCyclone.print_reg=print_one_reg;
	bankswitcher_init();

	

	if (memory.rom.cpu_m68k.size > 0x100000) {
		bankaddress = 0x100000;
	}
	//cpu_68k_init_save_state();


	/*time_slice=(overclk==0?
		    200000:200000+(overclk*200000/100.0))/264.0;*/
}

void cpu_68k_reset(void) {

	//printf("Reset \n");
	MyCyclone.srh=0x27; // Set supervisor mode
	//CycloneSetSr(&MyCyclone,0x27);
	//MyCyclone.srh=0x20;
	//MyCyclone.irq=7;
	MyCyclone.irq=0;
	MyCyclone.a[7]=MyCyclone.read32(0);
	
	MyCyclone.membase=0;
	MyCyclone.pc=MyCyclone.checkpc(MyCyclone.read32(4)); // Get Program Counter

	//printf("PC=%08x\n SP=%08x\n",MyCyclone.pc-MyCyclone.membase,MyCyclone.a[7]);
}

int cpu_68k_run(Uint32 nb_cycle) {
	if (conf.raster) {
		total_cycles=nb_cycle;MyCyclone.cycles=nb_cycle;	
		CycloneRun(&MyCyclone);
		return -MyCyclone.cycles;
	} else {
		memory.vid.current_line=0;
		
		total_cycles=nb_cycle;
		MyCyclone.cycles=0;
		const int cyclesPerLine = nb_cycle / 264.0;
		for(int i = 0; i < 265; i++) {
			MyCyclone.cycles+=cyclesPerLine;
			CycloneRun(&MyCyclone);
			memory.vid.current_line++;
		}
		return -MyCyclone.cycles;
	}
}

void cpu_68k_interrupt(int a) {
	//printf("Interrupt %d\n",a);
	MyCyclone.irq=a;
}

void cpu_68k_disassemble(int pc, int nb_instr) {
	/* TODO */
}

void cpu_68k_dumpreg(void) {
	int i;
	for(i=0;i<8;i++) logMsg("  d%d=%08x a%d=%08x",i,MyCyclone.d[i],i,MyCyclone.a[i]);
	/*printf("stack:\n");
	for (i=0;i<10;i++) {
		printf("%02d - %08x\n",i,mem68k_fetch_ram_long(MyCyclone.a[7]+i*4));
		}*/
	
}

int cpu_68k_run_step(void) {
	MyCyclone.cycles=0;
	CycloneRun(&MyCyclone);
	return -MyCyclone.cycles;
}

Uint32 cpu_68k_getpc(void) {
	return MyCyclone.pc-MyCyclone.membase;
}

void cpu_68k_fill_state(M68K_STATE *st) {
}


void cpu_68k_set_state(M68K_STATE *st) {
}

int cpu_68k_debuger(void (*execstep)(int, void *, void *, void *),void (*dump)(void)) {
	/* TODO */
	return 0;
}
