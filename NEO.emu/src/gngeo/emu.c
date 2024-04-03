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

#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "emu.h"
#include "memory.h"
#include "pd4990a.h"
#include "debug.h"

#include "timer.h"
#include "ym2610/2610intf.h"
#include "sound.h"
#include "neocrypt.h"
#include "conf.h"
#include "menu.h"
#include <imagine/logger/logger.h>

static int z80TimeslicesRan = 0;

void setup_misc_patch(char *name) {


	if (!strcmp(name, "ssideki")) {
		WRITE_WORD_ROM(&memory.rom.cpu_m68k.p[0x2240], 0x4e71);
	}

	//if (!strcmp(name, "fatfury3")) {
	//	WRITE_WORD_ROM(memory.rom.cpu_m68k.p, 0x0010);
	//}

	if (!strcmp(name, "mslugx")) {
		/* patch out protection checks */
		int i;
		Uint8 *RAM = memory.rom.cpu_m68k.p;
		for (i = 0; i < (int)memory.rom.cpu_m68k.size; i += 2) {
			if ((READ_WORD_ROM(&RAM[i + 0]) == 0x0243)
					&& (READ_WORD_ROM(&RAM[i + 2]) == 0x0001) && /* andi.w  #$1, D3 */
			(READ_WORD_ROM(&RAM[i + 4]) == 0x6600)) { /* bne xxxx */

				WRITE_WORD_ROM(&RAM[i + 4], 0x4e71);
				WRITE_WORD_ROM(&RAM[i + 6], 0x4e71);
			}
		}

		WRITE_WORD_ROM(&RAM[0x3bdc], 0x4e71);
		WRITE_WORD_ROM(&RAM[0x3bde], 0x4e71);
		WRITE_WORD_ROM(&RAM[0x3be0], 0x4e71);
		WRITE_WORD_ROM(&RAM[0x3c0c], 0x4e71);
		WRITE_WORD_ROM(&RAM[0x3c0e], 0x4e71);
		WRITE_WORD_ROM(&RAM[0x3c10], 0x4e71);

		WRITE_WORD_ROM(&RAM[0x3c36], 0x4e71);
		WRITE_WORD_ROM(&RAM[0x3c38], 0x4e71);
	}

}

void neogeo_reset(void) {
    //	memory.vid.modulo = 1; /* TODO: Move to init_video */
	memset(memory.ram, 0 , sizeof(memory.ram));
	memset(memory.z80_ram, 0 , sizeof(memory.z80_ram));
	memcpy(memory.rom.cpu_m68k.p, memory.rom.bios_m68k.p, 0x80);
	memset(pvcMem, 0 , sizeof(pvcMem));
	memory.current_vector=0;
	memory.vid.current_line = 0;
	init_timer();
	sram_lock = 0;
	sound_code = 0;
	pending_command = 0;
	result_code = 0;
	if (memory.rom.cpu_m68k.size > 0x100000)
		cpu_68k_bankswitch(0x100000);
	else
		cpu_68k_bankswitch(0);
	cpu_68k_reset();
}

void init_sound(void)
{
	cpu_z80_init();
	YM2610_sh_start();
	conf.snd_st_reg_create = 1;
}

void init_neo(void) {
	//neogeo_init_save_state();

	cpu_68k_init();
//	neogeo_reset();
	pd4990a_init();
//	setup_misc_patch(rom_name);

	init_sound();

	neogeo_reset();
}

unsigned int fc;
int last_line;

static inline int neo_interrupt(int skip_this_frame, void *emuTaskCtxPtr, void *neoSystemPtr, void *emuVideoPtr) {
    static int frames;

	pd4990a_addretrace();
	// printf("neogeo_frame_counter_speed %d\n",neogeo_frame_counter_speed);
	if (!(memory.vid.irq2control & 0x8)) {
		if (fc >= neogeo_frame_counter_speed) {
			fc = 0;
			neogeo_frame_counter++;
		}
		fc++;
	}

	if (!skip_this_frame)
	{
		draw_screen(emuTaskCtxPtr, neoSystemPtr, emuVideoPtr);
	}
    /*
    frames++;
    printf("FRAME %d\n",frames);
    if (frames==262) {
        FILE *f;
        sleep(1);
        f=fopen("/tmp/video.dmp","wb");
        fwrite(&memory.vid.ram,0x20000,1,f);
        fclose(f);
    }
    */
	return 1;
}

static inline void update_screen(int skip_this_frame, void *emuTaskCtxPtr, void *neoSystemPtr, void *emuVideoPtr) {

	if (memory.vid.irq2control & 0x40)
		memory.vid.irq2start = (memory.vid.irq2pos + 3) / 0x180; /* ridhero gives 0x17d */
	else
		memory.vid.irq2start = 1000;

	if (!skip_this_frame) {
		if (last_line < 21) { /* there was no IRQ2 while the beam was in the
							 * visible area -> no need for scanline rendering */
			draw_screen(emuTaskCtxPtr, neoSystemPtr, emuVideoPtr);
		} else {
			draw_screen_scanline(last_line - 21, 262, 1, emuTaskCtxPtr, neoSystemPtr, emuVideoPtr);
		}
	}

	last_line = 0;

	pd4990a_addretrace();
	if (fc >= neogeo_frame_counter_speed) {
		fc = 0;
		neogeo_frame_counter++;
	}
	fc++;
}

static inline int update_scanline(int skip_this_frame, void *emuTaskCtxPtr, void *neoSystemPtr, void *emuVideoPtr) {
	memory.vid.irq2taken = 0;

	if (memory.vid.irq2control & 0x10) {

		if (memory.vid.current_line == memory.vid.irq2start) {
			if (memory.vid.irq2control & 0x80)
				memory.vid.irq2start += (memory.vid.irq2pos + 3) / 0x180;
			memory.vid.irq2taken = 1;
		}
	}

	if (memory.vid.irq2taken) {
		if (!skip_this_frame) {
			if (last_line < 21)
				last_line = 21;
			if (memory.vid.current_line < 20)
				memory.vid.current_line = 20;
			draw_screen_scanline(last_line - 21, memory.vid.current_line - 20, 0, emuTaskCtxPtr, neoSystemPtr, emuVideoPtr);
		}
		last_line = memory.vid.current_line;
	}
	memory.vid.current_line++;
	return memory.vid.irq2taken;
}

void syncZ80()
{
	const unsigned z80_overclk = 0;
	const Uint32 cpu_z80_timeslice = (z80_overclk == 0 ? 73333 : 73333 + (z80_overclk
			* 73333 / 100.0));
	const Uint32 cpu_z80_timeslice_interlace = cpu_z80_timeslice
			/ (float) nb_interlace;
	int currTS = currentZ80Timeslice();
	if(currTS != 256 && currTS > z80TimeslicesRan)
	{
		//logMsg("syncing Z80 to line %d, time slice %d", memory.vid.current_line, currTS);
	}
	while(currTS > z80TimeslicesRan)
	{
		cpu_z80_run(cpu_z80_timeslice_interlace);
		my_timer();
		z80TimeslicesRan++;
	}
}

void main_frame(void *emuTaskCtxPtr, void *neoSystemPtr, void *emuVideoPtr) {
	const int skip_this_frame = !emuVideoPtr;
	unsigned m68k_overclk = 0;
	#ifdef USE_MUSASHI
	const int baseTimeslice = 250000;
	#else
	const int baseTimeslice = 200000;
	#endif
	const int cpu_68k_timeslice = (m68k_overclk == 0 ? baseTimeslice : baseTimeslice
			+ (m68k_overclk * baseTimeslice / 100.0));
	const int cpu_68k_timeslice_scanline = cpu_68k_timeslice / 264.0;
	z80TimeslicesRan = 0;
	// run one frame
	if(!conf.debug)
	{
		if(conf.raster)
		{
			int tm_cycle = 0;
			memory.vid.current_line = 0;
			while(memory.vid.current_line < 264)
			{
				tm_cycle = cpu_68k_run(cpu_68k_timeslice_scanline - tm_cycle);
				if(update_scanline(skip_this_frame, emuTaskCtxPtr, neoSystemPtr, emuVideoPtr))
				{
					//logMsg("irq 2");
					cpu_68k_interrupt(2);
				}
			}
			tm_cycle = cpu_68k_run(cpu_68k_timeslice_scanline - tm_cycle);
			update_screen(skip_this_frame, emuTaskCtxPtr, neoSystemPtr, emuVideoPtr);
			memory.watchdog++;
			if(memory.watchdog > 7)
			{
				logMsg("WATCHDOG RESET\n");
				cpu_68k_reset();
			}
			cpu_68k_interrupt(1);
		}
		else
		{
			cpu_68k_run(cpu_68k_timeslice);
			int a = neo_interrupt(skip_this_frame, emuTaskCtxPtr, neoSystemPtr, emuVideoPtr);
			memory.watchdog++;
			if(memory.watchdog > 7)
			{ /* Watchdog reset after ~0.13 == ~7.8 frames */
				logMsg("WATCHDOG RESET %d\n",memory.watchdog);
				cpu_68k_reset();
			}
			if(a)
			{
				//logMsg("irq %d", a);
				cpu_68k_interrupt(a);
			}
		}
	}
	syncZ80();
	assert(z80TimeslicesRan == 256);
	if(conf.test_switch == 1)
		conf.test_switch = 0;
}

void cpu_68k_dpg_step(int skip_this_frame, void *emuTaskCtxPtr, void *neoSystemPtr, void *emuVideoPtr) {
	static Uint32 nb_cycle;
	static Uint32 line_cycle;
	Uint32 cpu_68k_timeslice = 200000;
	Uint32 cpu_68k_timeslice_scanline = 200000 / (float) 262;
	Uint32 cycle;
	if (nb_cycle == 0) {
		main_loop(); /* update event etc. */
	}
	cycle = cpu_68k_run_step();
	add_bt(cpu_68k_getpc());
	line_cycle += cycle;
	nb_cycle += cycle;
	if (nb_cycle >= cpu_68k_timeslice) {
		nb_cycle = line_cycle = 0;
		if (conf.raster) {
			update_screen(skip_this_frame, emuTaskCtxPtr, neoSystemPtr, emuVideoPtr);
		} else {
			neo_interrupt(skip_this_frame, emuTaskCtxPtr, neoSystemPtr, emuVideoPtr);
		}
		//state_handling(pending_save_state, pending_load_state);
		cpu_68k_interrupt(1);
	} else {
		if (line_cycle >= cpu_68k_timeslice_scanline) {
			line_cycle = 0;
			if (conf.raster) {
				if (update_scanline(skip_this_frame, emuTaskCtxPtr, neoSystemPtr, emuVideoPtr))
					cpu_68k_interrupt(2);
			}
		}
	}
}

void debug_loop(void) {
	int a;
	do {
	  a = cpu_68k_debuger(cpu_68k_dpg_step, /*dump_hardware_reg*/NULL);
	} while (a != -1);
}
