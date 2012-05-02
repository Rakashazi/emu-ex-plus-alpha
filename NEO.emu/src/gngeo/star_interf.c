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

#ifdef USE_STARSCREAM

#include <stdlib.h>
#include "SDL.h"

#include "star/starcpu.h"
#include "star/cpudebug.h"
#include "raze/raze.h"
#include "emu.h"
#include "video.h"
#include "memory.h"
#include "pd4990a.h"
#include "state.h"
#include "debug.h"



struct STARSCREAM_PROGRAMREGION pretend_programfetch[] = {
	{0x100000, 0x10FFff, 0}, //RAM
	{0x200000, 0x2FFFfF, 0}, //CPU bank 1
	{0xc00000, 0xc1FFff, 0}, //BIOS
	{0x000000, 0xFFFFF, 0}, //CPU
    {0xd00000, 0xd0ffff, (Uintptr) memory.sram - 0xd00000},
    {-1, -1, 0}
};


struct STARSCREAM_DATAREGION pretend_readbyte[] = {
    {0x100000, 0x10FFff, NULL, 0},	/* RAM */
    {0x200000, 0x2fFFfF, NULL, 0},	/* CPU BANK */
    {0xc00000, 0xcfFFff, NULL, 0},	/* BIOS */
    {0x000000, 0xfffff,  NULL, 0},	/* CPU BANK 0 */
    {0xd00000, 0xd0ffff, mem68k_fetch_sram_byte, 0},
    {0x400000, 0x401fff, mem68k_fetch_pal_byte, 0},
    {0x3c0000, 0x3c0fff, mem68k_fetch_video_byte, 0},
    {0x300000, 0x300fff, mem68k_fetch_ctl1_byte, 0},
    {0x340000, 0x340fff, mem68k_fetch_ctl2_byte, 0},
    {0x380000, 0x380fff, mem68k_fetch_ctl3_byte, 0},
    {0x320000, 0x320fff, mem68k_fetch_coin_byte, 0},
    {0x800000, 0x800fff, mem68k_fetch_memcrd_byte, 0},
    {-1, -1, 0, 0}
};

struct STARSCREAM_DATAREGION pretend_readword[] = {
    
    {0x100000, 0x10FFff, NULL, 0},
    {0x200000, 0x2FFFfF, NULL, 0},
    {0xc00000, 0xcfFFff, NULL, 0},
    {0x000000, 0xfffff,  NULL, 0},
    {0xd00000, 0xd0ffff, mem68k_fetch_sram_word, 0},
    {0x400000, 0x401fff, mem68k_fetch_pal_word, 0},
    {0x3c0000, 0x3c0fff, mem68k_fetch_video_word, 0},
    {0x300000, 0x300fff, mem68k_fetch_ctl1_word, 0},
    {0x340000, 0x340fff, mem68k_fetch_ctl2_word, 0},
    {0x380000, 0x380fff, mem68k_fetch_ctl3_word, 0},
    {0x320000, 0x320fff, mem68k_fetch_coin_word, 0},
    {0x800000, 0x800fff, mem68k_fetch_memcrd_word, 0},
    {-1, -1, 0, 0}
};


struct STARSCREAM_DATAREGION pretend_writebyte[] = {
    {0x100000, 0x10FFff, NULL, 0},
    {0x3c0000, 0x3c0fff, mem68k_store_video_byte, 0},
    {0x400000, 0x401fff, mem68k_store_pal_byte, 0},
    {0xd00000, 0xd0ffff, mem68k_store_sram_byte, 0},
    {0x380000, 0x380fff, mem68k_store_pd4990_byte, 0},
    {0x320000, 0x320fff, mem68k_store_z80_byte, 0},
    {0x3a0000, 0x3a0fff, mem68k_store_setting_byte, 0},
    {0x200000, 0x2fffff, NULL, 0},
    {0x800000, 0x800fff, mem68k_store_memcrd_byte, 0},
    {0x300000, 0x310000, mem68k_store_invalid_byte,0},
    {-1, -1, 0, 0}
};



struct STARSCREAM_DATAREGION pretend_writeword[] = {
    {0x100000, 0x10FFff, NULL, 0},
    {0x3c0000, 0x3c0fff, mem68k_store_video_word, 0},
    {0x400000, 0x401fff, mem68k_store_pal_word, 0},
    {0xd00000, 0xd0ffff, mem68k_store_sram_word, 0},
    {0x380000, 0x380fff, mem68k_store_pd4990_word, 0},
    {0x320000, 0x320fff, mem68k_store_z80_word, 0},
    {0x3a0000, 0x3a0fff, mem68k_store_setting_word, 0},
    {0x200000, 0x2fffff, NULL, 0},
    {0x800000, 0x800fff, mem68k_store_memcrd_word, 0},
    {-1, -1, 0, 0}
};

void cpu_68k_bankswitch(Uint32 address)
{
    pretend_readbyte[1].userdata = memory.rom.cpu_m68k.p + address;
    pretend_readword[1].userdata = memory.rom.cpu_m68k.p + address;

    pretend_programfetch[1].offset =
	(Uintptr) memory.rom.cpu_m68k.p + address - 0x200000;

    bankaddress=address;
};

void cpu_68k_reset(void)
{
    s68000reset();
//    printf("Reset Pc=%08x\n",s68000context.pc);
}

int cpu_68k_getcycle(void)
{
    return s68000readOdometer();
}

static void cpu_68k_post_load_state(void) {
    struct S68000CONTEXT star_context;
    int i;
    s68000GetContext(&star_context);
    for (i=0;i<8;i++) {
	star_context.dreg[i]=s68000context.dreg[i];
	star_context.areg[i]=s68000context.areg[i];
    }
    star_context.pc=s68000context.pc;
    star_context.asp=s68000context.asp;
    star_context.sr=s68000context.sr;
    s68000SetContext(&star_context);
    cpu_68k_bankswitch(bankaddress);
}

static void cpu_68k_init_save_state(void) {
    create_state_register(ST_68k,"dreg",1,(void *)s68000context.dreg,sizeof(Uint32)*8,REG_UINT32);
    create_state_register(ST_68k,"areg",1,(void *)s68000context.areg,sizeof(Uint32)*8,REG_UINT32);
    create_state_register(ST_68k,"pc",1,(void *)&s68000context.pc,sizeof(Uint32),REG_UINT32);
    create_state_register(ST_68k,"asp",1,(void *)&s68000context.asp,sizeof(Uint32),REG_UINT32);
    create_state_register(ST_68k,"sr",1,(void *)&s68000context.sr,sizeof(Uint32),REG_UINT32);
    create_state_register(ST_68k,"bank",1,(void *)&bankaddress,sizeof(Uint32),REG_UINT32);
    create_state_register(ST_68k,"ram",1,(void *)memory.ram,0x10000,REG_UINT8);
    //    create_state_register(ST_68k,"kof2003_bksw",1,(void *)memory.kof2003_bksw,0x1000,REG_UINT8);
    create_state_register(ST_68k,"current_vector",1,(void *)memory.rom.cpu_m68k.p,0x80,REG_UINT8);
    set_post_load_function(ST_68k,cpu_68k_post_load_state);
}

void bankswitcher_init() {
    pretend_readbyte[1].memorycall=mem68k_fetch_bk_normal_byte;
    pretend_readword[1].memorycall=mem68k_fetch_bk_normal_word;
    pretend_writebyte[7].memorycall=mem68k_store_bk_normal_byte;
    pretend_writeword[7].memorycall=mem68k_store_bk_normal_word;
}

void cpu_68k_mkstate(gzFile gzf,int mode) {
	/* TODO */
}

void cpu_68k_init(void)
{
    s68000init();
    bankswitcher_init();
    pretend_writebyte[0].userdata = memory.ram;
    pretend_writeword[0].userdata = memory.ram;
    pretend_readbyte[0].userdata = memory.ram;
    pretend_readword[0].userdata = memory.ram;

    pretend_readbyte[1].userdata = memory.rom.cpu_m68k.p;
    pretend_readword[1].userdata = memory.rom.cpu_m68k.p;
    
    
    pretend_readbyte[3].userdata = memory.rom.cpu_m68k.p;
    pretend_readword[3].userdata = memory.rom.cpu_m68k.p;

    pretend_readbyte[2].userdata = memory.rom.bios_m68k.p;
    pretend_readword[2].userdata = memory.rom.bios_m68k.p;

    pretend_programfetch[0].offset = (Uintptr) memory.ram - 0x100000;
    pretend_programfetch[1].offset = (Uintptr) memory.rom.cpu_m68k.p - 0x200000;
    pretend_programfetch[2].offset = (Uintptr) memory.rom.bios_m68k.p - 0xC00000;
    pretend_programfetch[3].offset = (Uintptr) memory.rom.cpu_m68k.p;

    s68000context.s_fetch = pretend_programfetch;
    s68000context.u_fetch = pretend_programfetch;

    s68000context.s_readbyte = pretend_readbyte;
    s68000context.u_readbyte = pretend_readbyte;
    s68000context.s_readword = pretend_readword;
    s68000context.u_readword = pretend_readword;
    s68000context.s_writebyte = pretend_writebyte;
    s68000context.u_writebyte = pretend_writebyte;
    s68000context.s_writeword = pretend_writeword;
    s68000context.u_writeword = pretend_writeword;


    if (memory.rom.cpu_m68k.size > 0x100000) {
	pretend_readbyte[1].userdata = memory.rom.cpu_m68k.p + 0x100000;
	pretend_readword[1].userdata = memory.rom.cpu_m68k.p + 0x100000;

	pretend_programfetch[1].offset =
	    (Uintptr) memory.rom.cpu_m68k.p + 0x100000 - 0x200000;
    }
    cpu_68k_init_save_state();
}

int cpu_68k_run(Uint32 nb_cycle)
{
    int a;
    //printf("Exec Pc=%08x ",s68000context.pc);
    s68000exec(nb_cycle);
    a=s68000tripOdometer()-nb_cycle;
    //printf("nb_cycle=%d %d\n",a,nb_cycle);
    return a;
}

void cpu_68k_interrupt(int a)
{
    s68000interrupt(a, -1);
}

/* Debug interface */
/*
void cpu_68k_disassemble(int pc, int nb_instr)
{
}

void cpu_68k_dumpreg(void)
{
}
*/
int cpu_68k_run_step(void)
{
    int a;
    s68000exec(1);
    a= s68000tripOdometer();
    //printf("Clock: %d\n",a);
    return a;
}
static void dbg_put(const char *c) {
    printf("%s",c);
    fflush(stdout);
}
static void dbg_get(char* s, int size) {
    //char buf[size];
    char *args,*argsend;
    int pc;

    fgets(s, size, stdin);

    args = s + 1;
    while((*args) && ((*args) < 32)) args++;

    switch (s[0]) {
    case '?':
	printf("B [address]           Add a breakpoint at [address]\n"
	       "N [address]           Del breakpoint at [address]\n"
	       "R                     Run until breakpoint\n");
	return;
    case 'B':
	if (args) {
	    pc=strtoul(args,&argsend,0);
	    if (args != argsend)
		add_bp(pc);
	    else
		printf("Invalid input\n");
	}
	s[0]=0;
	break;
    case 'N':
	if (args) {
	    pc=strtoul(args,&argsend,0);
	    if (args != argsend)
		del_bp(pc);
	    else
		printf("Invalid input\n");
	}
	s[0]=0;
	break;
    case 'R':
	while(check_bp(cpu_68k_getpc())!=SDL_TRUE && dbg_step==0) {
	     cpu_68k_dpg_step();
	}
	if (dbg_step) dbg_step=0;
	s[0]=0;
	break;
    }
    
}

int cpu_68k_debuger(void (*execstep)(void),void (*dump)(void)) {
    return cpudebug_interactive(1,dbg_put,dbg_get,execstep,dump);
}


Uint32 cpu_68k_getpc(void)
{
    return s68000readPC();
}

/* 
   fill st with information from the starscream context
*/
void cpu_68k_fill_state(M68K_STATE *st)
{
    struct S68000CONTEXT star_context;
    int i;
    s68000GetContext(&star_context);
    for (i=0;i<8;i++) {
	st->dreg[i]=star_context.dreg[i];
	st->areg[i]=star_context.areg[i];
    }
    st->pc=star_context.pc;
    st->asp=star_context.asp;
    st->sr=star_context.sr;
}

void cpu_68k_set_state(M68K_STATE *st)
{
    struct S68000CONTEXT star_context;
    int i;
    s68000GetContext(&star_context);
    for (i=0;i<8;i++) {
	star_context.dreg[i]=st->dreg[i];
	star_context.areg[i]=st->areg[i];
    }
    star_context.pc=st->pc;
    star_context.asp=st->asp;
    star_context.sr=st->sr;
    s68000SetContext(&star_context);
}
#endif
