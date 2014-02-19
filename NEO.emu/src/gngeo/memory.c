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

#include "emu.h"
#include "video.h"
#include "memory.h"
#include "pd4990a.h"
#include "transpack.h"

#ifdef GP2X
#include "ym2610-940/940shared.h"
#endif

Uint8 (*mem68k_fetch_bksw_byte)(Uint32);
Uint16 (*mem68k_fetch_bksw_word)(Uint32);
Uint32 (*mem68k_fetch_bksw_long)(Uint32);
void (*mem68k_store_bksw_byte)(Uint32,Uint8);
void (*mem68k_store_bksw_word)(Uint32,Uint16);
void (*mem68k_store_bksw_long)(Uint32,Uint32);

neo_mem memory;

Uint8 *current_pal;
Uint32 *current_pc_pal;
Uint8 *current_fix;
Uint8 *fix_usage;
Uint8 sram_lock;
Uint8 sound_code;
Uint8 pending_command;
Uint8 result_code;
Uint16 z80_bank[4];

Uint32 bankaddress = 0;

void neogeo_sound_irq(int irq) {
	//printf("neogeo_sound_irq %d\n",irq);
#ifndef ENABLE_940T
	if (irq) {
		cpu_z80_raise_irq(0);
	} else
	cpu_z80_lower_irq();
#endif
	//printf("neogeo_sound_end %d\n",irq);
}

static __inline__ Uint16 read_neo_control(void) {
	unsigned int scan;

	if (!conf.raster) {

#if defined(__arm__)
#ifdef USE_CYCLONE
		scan = memory.vid.current_line;
		/*
		 printf("%d %d %d\n",current_line,
		 (cpu_68k_getcycle()/3)>>7,
		 (int)(cpu_68k_getcycle() / 766.28));
		 */
#else
		scan = cpu_68k_getcycle()/3;
		scan = scan>>7;
#endif
#else
		scan = cpu_68k_getcycle() / 766.28; /* current scanline */
#endif

		//	scan+=0x100;
		//	if (scan >=0x200) scan=scan-0x108;
		scan += 0xF8;

		return (scan << 7) | (conf.pal << 3) | (neogeo_frame_counter & 0x0007); /* frame counter */

	} else {
		scan = memory.vid.current_line /*+ 22*/; /* current scanline */
		//scan+=0x110;
		//if (scan >=0x200) scan=scan-0x108;
		scan += 0xF8;

		return (scan << 7) | (conf.pal << 3) | (neogeo_frame_counter & 0x0007); /* frame counter */
	}
}

__inline__ void write_neo_control(Uint16 data) {
	neogeo_frame_counter_speed = (((data >> 8) & 0xff) + 1);
	memory.vid.irq2control = data & 0xff;
	return;
}

__inline__ void write_irq2pos(Uint32 data) {
	memory.vid.irq2pos = data;
	if (memory.vid.irq2control & 0x20) {
		int line = (memory.vid.irq2pos + 0x3b) / 0x180; /* turfmast goes as low as 0x145 */
		memory.vid.irq2start = line + memory.vid.current_line;
	}
}

#ifndef ENABLE_940T
/* Z80 IO port handler */
Uint8 z80_port_read(Uint16 PortNo)
{
	//printf("z80_port_read PC=%04x p=%04x ",cpu_z80_get_pc(),PortNo);
	//printf("z80_port_read p=%04x \n",PortNo);
	switch (PortNo & 0xff) {
		case 0x0:
		pending_command = 0;
		//logMsg("Reseting command. Return sndcode %x",sound_code);
		return sound_code;
		break;

		case 0x4:
		//printf("v=%02x\n",YM2610_status_port_0_A_r(0));
		return YM2610_status_port_A_r(0);
		break;

		case 0x5:
		//printf("v=%02x\n",YM2610_read_port_0_r(0));
		return YM2610_read_port_r(0);
		break;

		case 0x6:
		//printf("v=%02x\n",YM2610_status_port_0_B_r(0));
		return YM2610_status_port_B_r(0);
		break;

		case 0x08:
		//printf("v=00 (sb3)\n");
		cpu_z80_switchbank(3, PortNo);
		return 0;
		break;

		case 0x09:
		//printf("v=00 (sb2)\n");
		cpu_z80_switchbank(2, PortNo);
		return 0;
		break;

		case 0x0a:
		//printf("v=00 (sb1)\n");
		cpu_z80_switchbank(1, PortNo);
		return 0;
		break;

		case 0x0b:
		//printf("v=00 (sb0)\n");
		cpu_z80_switchbank(0, PortNo);
		return 0;
		break;
	};

	return 0;
}

void z80_port_write(Uint16 PortNb, Uint8 Value)
{
	Uint8 data = Value;
	//printf("z80_port_write PC=%04x OP=%02x p=%04x v=%02x\n",cpu_z80_get_pc(),memory.rom.cpu_z80.p[cpu_z80_get_pc()],PortNb,Value);
	//printf("Write port %04x %02x\n",PortNb,Value);
	switch (PortNb & 0xff) {
		case 0x4:
		YM2610_control_port_A_w(0, data);
		break;

		case 0x5:
		YM2610_data_port_A_w(0, data);
		break;

		case 0x6:
		YM2610_control_port_B_w(0, data);
		break;

		case 0x7:
		YM2610_data_port_B_w(0, data);
		break;

		case 0xC:
		//logMsg("Setting result code to %0x",Value);
		result_code = Value;
		break;
	}
}
#endif

/* Protection hack */
Uint16 protection_9a37(Uint32 addr) {
	return 0x9a37;
}

/* fetching function */
/**** INVALID FETCHING ****/
Uint8 mem68k_fetch_invalid_byte(Uint32 addr) {
	return 0xF0;
}

Uint16 mem68k_fetch_invalid_word(Uint32 addr) {
	return 0xF0F0;
}

Uint32 mem68k_fetch_invalid_long(Uint32 addr) {
	return 0xF0F0F0F0;
}

/**** RAM FETCHING ****/
Uint8 mem68k_fetch_ram_byte(Uint32 addr) {
	//  printf("mem68k_fetch_ram_byte %x\n",addr);
	addr &= 0xffff;
	return (READ_BYTE_ROM(memory.ram + addr));
}

Uint16 mem68k_fetch_ram_word(Uint32 addr) {
	//printf("mem68k_fetch_ram_word %08x %04x\n",addr,READ_WORD_RAM(memory.ram + (addr&0xffff)));
	addr &= 0xffff;
	return (READ_WORD_ROM(memory.ram + addr));
}

LONG_FETCH(mem68k_fetch_ram)
;

/**** CPU ****/
Uint8 mem68k_fetch_cpu_byte(Uint32 addr) {
	addr &= 0xFFFFF;

	return (READ_BYTE_ROM(memory.rom.cpu_m68k.p + addr));
}

Uint16 mem68k_fetch_cpu_word(Uint32 addr) {
	addr &= 0xFFFFF;

	return (READ_WORD_ROM(memory.rom.cpu_m68k.p + addr));
}

LONG_FETCH(mem68k_fetch_cpu)
;

/**** BIOS ****/
Uint8 mem68k_fetch_bios_byte(Uint32 addr) {
	addr &= 0x1FFFF;
	return (READ_BYTE_ROM(memory.rom.bios_m68k.p + addr));
}

Uint16 mem68k_fetch_bios_word(Uint32 addr) {
	addr &= 0x1FFFF;
	return (READ_WORD_ROM(memory.rom.bios_m68k.p + addr));
}

LONG_FETCH(mem68k_fetch_bios)
;

/**** SRAM ****/
Uint8 mem68k_fetch_sram_byte(Uint32 addr) {
	return memory.sram[addr - 0xd00000];
}

Uint16 mem68k_fetch_sram_word(Uint32 addr) {
	addr -= 0xd00000;
	return (memory.sram[addr] << 8) | (memory.sram[addr + 1] & 0xff);
}

LONG_FETCH(mem68k_fetch_sram)
;

/**** PALETTE ****/
Uint8 mem68k_fetch_pal_byte(Uint32 addr) {
	addr &= 0xffff;
	if (addr <= 0x1fff)
		return current_pal[addr];
	return 0;
}

Uint16 mem68k_fetch_pal_word(Uint32 addr) {
	addr &= 0xffff;
	if (addr <= 0x1fff)
		return READ_WORD(&current_pal[addr]);
	return 0;
}

LONG_FETCH(mem68k_fetch_pal)
;

/**** VIDEO ****/
Uint8 mem68k_fetch_video_byte(Uint32 addr) {
	//printf("mem6k_fetch_video_byte %08x\n",addr);
	if (!(addr&0x1))
			return mem68k_fetch_video_word(addr)>>8;
	else {
		Uint32 lpc=cpu_68k_getpc()+2;
		switch((lpc&0xF00000)>>20) {
		case 0x0:
			return READ_WORD(&memory.rom.cpu_m68k.p+(lpc&0xFFFFF));
			break;
		case 0x2:
			return READ_WORD(&memory.rom.cpu_m68k.p+bankaddress+(lpc&0xFFFFF));
			break;
		case 0xC:
			if (lpc<=0xc1FFff)
				return READ_WORD(&memory.rom.bios_m68k.p+(lpc&0xFFFFF));
			break;
		}
	}
//	addr &= 0xFFFF;
//	if (addr == 0xe)
//		return 0xff;
	return 0xFF;
}

Uint16 mem68k_fetch_video_word(Uint32 addr) {
	//printf("mem68k_fetch_video_word %08x\n",addr);
	addr &= 0x7;
	/*
	 if (addr==0x00)
	 return vptr;
	 */
	if (addr == 0x00 || addr == 0x02 || addr == 0x0a)
		return memory.vid.rbuf;//READ_WORD(&memory.vid.ram[memory.vid.vptr << 1]);
	if (addr == 0x04)
		return memory.vid.modulo;
	if (addr == 0x06)
		return read_neo_control();
	return 0;
}
LONG_FETCH(mem68k_fetch_video)
;

/**** CONTROLLER ****/
Uint8 mem68k_fetch_ctl1_byte(Uint32 addr) {
	addr &= 0xFFFF;
	if (addr == 0x00)
		return memory.intern_p1;
	if (addr == 0x01)
		return (conf.test_switch ? 0xFE : 0xFF);

	if (addr == 0x81) {
		return (conf.test_switch ? 0x00 : 0x80);
	}

	return 0;
}

Uint16 mem68k_fetch_ctl1_word(Uint32 addr) {
	//  printf("mem68k_fetch_ctl1_word\n");
	return 0;
}

Uint32 mem68k_fetch_ctl1_long(Uint32 addr) {
	//  printf("mem68k_fetch_ctl1_long\n");
	return 0;
}

Uint8 mem68k_fetch_ctl2_byte(Uint32 addr) {
	if ((addr & 0xFFFF) == 0x00)
		return memory.intern_p2;
	if ((addr & 0xFFFF) == 0x01)
		return 0xFF;
	return 0;
}

Uint16 mem68k_fetch_ctl2_word(Uint32 addr) {
	return 0;
}

Uint32 mem68k_fetch_ctl2_long(Uint32 addr) {
	return 0;
}

Uint8 mem68k_fetch_ctl3_byte(Uint32 addr) {
	//printf("Fetch ctl3 byte %x\n",addr);
	if ((addr & 0xFFFF) == 0x0)
		return memory.intern_start;
	return 0;
}

Uint16 mem68k_fetch_ctl3_word(Uint32 addr) {
	/*
	 printf("Fetch ctl3 word %x\n",addr);
	 if ((addr & 0xFFFF) == 0x0)
	 return memory.intern_start | 0xFF00;
	 */
	return 0;
}

Uint32 mem68k_fetch_ctl3_long(Uint32 addr) {
	return 0;
}

Uint8 mem68k_fetch_coin_byte(Uint32 addr) {
	addr &= 0xFFFF;
	if (addr == 0x1) {
		int coinflip = read_4990_testbit();
		int databit = read_4990_databit();
		return memory.intern_coin ^ (coinflip << 6) ^ (databit << 7);
	}
	if (addr == 0x0) {
		int res = 0;
		if (conf.sound) {
			//printf("fetch coin byte, rescoe= %x\n",result_code);
#ifdef ENABLE_940T

			res |= shared_ctl->result_code;
			if (shared_ctl->pending_command)
				res &= 0x7f;
#else
			res |= result_code;
			if (pending_command)
			res &= 0x7f;
#endif
		} else {
			res |= 0x01;
		}
		return res;
	}
	return 0;
}

Uint16 mem68k_fetch_coin_word(Uint32 addr) {
	return 0;
}

Uint32 mem68k_fetch_coin_long(Uint32 addr) {
	return 0;
}

/**** MEMCARD ****/
/* Even byte are FF 
 Odd  byte are data;
 */
Uint8 mem68k_fetch_memcrd_byte(Uint32 addr) {
	addr &= 0xFFF;
	if (addr & 1)
		return 0xFF;
	else
		return memory.memcard[addr >> 1];
}

Uint16 mem68k_fetch_memcrd_word(Uint32 addr) {
	addr &= 0xFFF;
	return memory.memcard[addr >> 1] | 0xff00;
}

Uint32 mem68k_fetch_memcrd_long(Uint32 addr) {
	return 0;
}

/* storring function */
/**** INVALID STORE ****/
void mem68k_store_invalid_byte(Uint32 addr, Uint8 data) {
	if (addr != 0x300001)
		printf("Invalid write b %x %x \n", addr, data);
	else {
		memory.watchdog = 0;
		//printf("restet_watchdog\n");
	}
}
void mem68k_store_invalid_word(Uint32 addr, Uint16 data) {
	logMsg("Invalid write w %x %x \n", addr, data);
}
void mem68k_store_invalid_long(Uint32 addr, Uint32 data) {
	logMsg("Invalid write l %x %x \n", addr, data);
}

/**** RAM ****/
void mem68k_store_ram_byte(Uint32 addr, Uint8 data) {
	addr &= 0xffff;
	WRITE_BYTE_ROM(memory.ram + addr,data);
	return;
}

void mem68k_store_ram_word(Uint32 addr, Uint16 data) {
	//printf("Store rom word %08x %04x\n",addr,data);
	addr &= 0xffff;
	WRITE_WORD_ROM(memory.ram + addr,data);
	return;
}

LONG_STORE(mem68k_store_ram)
;

/**** SRAM ****/
void mem68k_store_sram_byte(Uint32 addr, Uint8 data) {
	if (sram_lock)
		return;
	/*
	 if (addr == 0xd00000 + sram_protection_hack && ((data & 0xff) == 0x01))
	 return;
	 */
	memory.sram[addr - 0xd00000] = data;
}

void mem68k_store_sram_word(Uint32 addr, Uint16 data) {
	if (sram_lock)
		return;
	/*
	 if (addr == 0xd00000 + sram_protection_hack
	 && ((data & 0xffff) == 0x01))
	 return;
	 */
	addr -= 0xd00000;
	memory.sram[addr] = data >> 8;
	memory.sram[addr + 1] = data & 0xff;
}

LONG_STORE(mem68k_store_sram)
;

/**** PALETTE ****/
/*static __inline__ */Uint16 convert_pal(Uint16 npal) {
	int r = 0, g = 0, b = 0;
	r = ((npal >> 7) & 0x1e) | ((npal >> 14) & 0x01);
	g = ((npal >> 3) & 0x1e) | ((npal >> 13) & 0x01);
	b = ((npal << 1) & 0x1e) | ((npal >> 12) & 0x01);

	return (r << 11) + (g << 6) + b;
}

void update_all_pal(void) {
	int i;
	Uint32 *pc_pal1 = (Uint32*) memory.vid.pal_host[0];
	Uint32 *pc_pal2 = (Uint32*) memory.vid.pal_host[1];
	for (i = 0; i < 0x1000; i++) {
		//pc_pal1[i] = convert_pal(READ_WORD_ROM(&memory.pal1[i<<1]));
		//pc_pal2[i] = convert_pal(READ_WORD_ROM(&memory.pal2[i<<1]));
		pc_pal1[i] = convert_pal(READ_WORD(&memory.vid.pal_neo[0][i<<1]));
		pc_pal2[i] = convert_pal(READ_WORD(&memory.vid.pal_neo[1][i<<1]));
	}
}

void mem68k_store_pal_byte(Uint32 addr, Uint8 data) {
	/* TODO: verify this */
	addr &= 0xffff;
	if (addr <= 0x1fff) {
		Uint16 a = READ_WORD(&current_pal[addr & 0xfffe]);
		if (addr & 0x1)
			a = data | (a & 0xff00);
		else
			a = (a & 0xff) | (data << 8);
		WRITE_WORD(&current_pal[addr & 0xfffe], a);
		if ((addr >> 1) & 0xF)
			current_pc_pal[(addr) >> 1] = convert_pal(a);
		else
			current_pc_pal[(addr) >> 1] = 0xF81F;
	}
}

void mem68k_store_pal_word(Uint32 addr, Uint16 data) {
	//printf("Store pal word @ %08x %08x %04x\n",cpu_68k_getpc(),addr,data);
	addr &= 0xffff;
	if (addr <= 0x1fff) {
		WRITE_WORD(&current_pal[addr], data);
		if ((addr >> 1) & 0xF)
			current_pc_pal[(addr) >> 1] = convert_pal(data);
		else
			current_pc_pal[(addr) >> 1] = 0xF81F;
	}
}

LONG_STORE(mem68k_store_pal)
;

/**** VIDEO ****/
void mem68k_store_video_byte(Uint32 addr, Uint8 data) {
	/* garou write at 3c001f, 3c000f, 3c0015 */
	/* wjammers write, and fetch at 3c0000 .... */
	//printf("mem68k_store_video_byte %08x %02x @pc=%08x\n",addr,data,cpu_68k_getpc());
	if (!(addr&0x1)) {
		mem68k_store_video_word(addr,(data<<8)|data);
	}
}

void mem68k_store_video_word(Uint32 addr, Uint16 data) {
    //data&=0xFFFF;
    //printf("mem68k_store_video_word %08x %04x @pc=%08x\n",addr,data,cpu_68k_getpc());
	addr &= 0xF;
	switch (addr) {
	case 0x0:
		memory.vid.vptr = data & 0xffff;
		memory.vid.rbuf = READ_WORD(&memory.vid.ram[memory.vid.vptr << 1]);
		break;
	case 0x2:
		//printf("Store %04x to video %08x @pc=%08x\n",data,vptr<<1,cpu_68k_getpc());
		WRITE_WORD(&memory.vid.ram[memory.vid.vptr << 1], data);
		memory.vid.vptr = (memory.vid.vptr & 0x8000) + ((memory.vid.vptr
				+ memory.vid.modulo) & 0x7fff);
		memory.vid.rbuf = READ_WORD(&memory.vid.ram[memory.vid.vptr << 1]);
		break;
	case 0x4:
		if (data&0x4000)
			data|=0x8000;
		else
			data&=0x7FFF;

		memory.vid.modulo = (int) data;
		break;
	case 0x6:
		write_neo_control(data);
		break;
	case 0x8:
		write_irq2pos((memory.vid.irq2pos & 0xffff) | ((Uint32) data << 16));
		break;
	case 0xa:
		write_irq2pos((memory.vid.irq2pos & 0xffff0000) | (Uint32) data);
		break;
	case 0xc:
		/* games write 7 or 4 at 0x3c000c at every frame */
		/* IRQ acknowledge */
		break;
	}

}
LONG_STORE(mem68k_store_video)
;


/**** PD4990 ****/
void mem68k_store_pd4990_byte(Uint32 addr, Uint8 data) {
	write_4990_control_w(addr, data);
}

void mem68k_store_pd4990_word(Uint32 addr, Uint16 data) {
	write_4990_control_w(addr, data);
}

void mem68k_store_pd4990_long(Uint32 addr, Uint32 data) {
	write_4990_control_w(addr, data);
}

/**** Z80 ****/
void mem68k_store_z80_byte(Uint32 addr, Uint8 data) {
	if (addr == 0x320000) {
		sound_code = data & 0xff;
		pending_command = 1;
		//printf("B Pending command. Sound_code=%02x\n",sound_code);
		if (conf.sound) {
#ifdef ENABLE_940T
			//printf("%d\n",shared_ctl->pending_command);
			shared_ctl->sound_code = sound_code;
			shared_ctl->pending_command = pending_command;
			//shared_ctl->pending_command=pending_command++;
			shared_ctl->nmi_pending = 1;

			if (conf.accurate940) {
				while (CHECK_BUSY(JOB940_RUN_Z80)
						&& shared_ctl->pending_command)
					;
				if (shared_ctl->nmi_pending) {
					gp2x_add_job940(JOB940_RUN_Z80_NMI);
					while (CHECK_BUSY(JOB940_RUN_Z80_NMI))
						;
				}
			}
#else
			cpu_z80_nmi();
			cpu_z80_run(300);
#endif
		}
	}
}
void mem68k_store_z80_word(Uint32 addr, Uint16 data) {
	/* tpgolf use word store for sound */
	if (addr == 0x320000) {
		sound_code = data >> 8;
		pending_command = 1;
		//printf("W Pending command. Sound_code=%02x\n",sound_code);
		if (conf.sound) {
#ifdef ENABLE_940T
			shared_ctl->sound_code = sound_code;
			shared_ctl->pending_command = pending_command;
			shared_ctl->nmi_pending = 1;
			if (conf.accurate940) {
				while (CHECK_BUSY(JOB940_RUN_Z80))
					;
				if (shared_ctl->nmi_pending) {
					gp2x_add_job940(JOB940_RUN_Z80_NMI);
					while (CHECK_BUSY(JOB940_RUN_Z80_NMI))
						;
				}
			}
#else
			cpu_z80_nmi();
			cpu_z80_run(300);
#endif
		}
	}
}
void mem68k_store_z80_long(Uint32 addr, Uint32 data) {
	/* I don't think any game will use long store for sound.... */
	logMsg("Z80L %x %04x\n", addr, data);
}

/**** SETTINGS ****/
void mem68k_store_setting_byte(Uint32 addr, Uint8 data) {
	//printf("mem68k_store_setting_byte %08x\n",addr);
	addr &= 0xFFFF;
	if (addr == 0x0003) {
		logMsg("Selecting Bios Vector");
		memcpy(memory.rom.cpu_m68k.p, memory.rom.bios_m68k.p, 0x80);
		memory.current_vector=0;
	}

	if (addr == 0x0013) {
		logMsg("Selecting Game Vector");
		memcpy(memory.rom.cpu_m68k.p, memory.game_vector, 0x80);
		memory.current_vector=1;
	}

	if (addr == 0x000b) { /* select board fix */
		current_fix = memory.rom.bios_sfix.p;
		fix_usage = memory.fix_board_usage;
		memory.vid.currentfix=0;
		return;
	}
	if (addr == 0x001b) { /* select game fix */
		current_fix = memory.rom.game_sfix.p;
		fix_usage = memory.fix_game_usage;
		memory.vid.currentfix=1;
		return;
	}
	if (addr == 0x000d) { /* sram lock */
		sram_lock = 1;
		return;
	}
	if (addr == 0x001d) { /* sram unlock */
		sram_lock = 0;
		return;
	}
	if (addr == 0x000f) { /* set palette 2 */
		current_pal = memory.vid.pal_neo[1];
		current_pc_pal = (Uint32 *) memory.vid.pal_host[1];
		memory.vid.currentpal=1;
		return;
	}
	if (addr == 0x001f) { /* set palette 1 */
		current_pal = memory.vid.pal_neo[0];
		current_pc_pal = (Uint32 *) memory.vid.pal_host[0];
		memory.vid.currentpal = 0;
		return;
	}
	/* garou write 0 to 3a0001 -> enable display, 3a0011 -> disable display */
	//printf("unknow mem68k_store_setting_byte %x %x\n",addr,data);

}

void mem68k_store_setting_word(Uint32 addr, Uint16 data) {
	/* TODO: Some game use it */
	// printf("mem68k_store_setting_word USED????\n");
	mem68k_store_setting_byte(addr,data);
	return;
	addr &= 0xFFFFFe;
	if (addr == 0x3a0002) {
		memcpy(memory.rom.cpu_m68k.p, memory.rom.bios_m68k.p, 0x80);
	}

	if (addr == 0x3a0012) {
		memcpy(memory.rom.cpu_m68k.p, memory.game_vector, 0x80);
	}
	if (addr == 0x3a000a) {
		current_fix = memory.rom.bios_sfix.p;
		fix_usage = memory.fix_board_usage;
		return;
	}
	if (addr == 0x3a001a) {
		current_fix = memory.rom.game_sfix.p;
		fix_usage = memory.fix_game_usage;
		return;
	}
	if (addr == 0x3a000c) {
		sram_lock = 1;
		return;
	}
	if (addr == 0x3a001c) {
		sram_lock = 0;
		return;
	}
	if (addr == 0x3a000e) {
		current_pal = memory.vid.pal_neo[1];
		current_pc_pal = (Uint32 *) memory.vid.pal_host[1];
		return;
	}
	if (addr == 0x3a001e) {
		current_pal = memory.vid.pal_neo[0];
		current_pc_pal = (Uint32 *) memory.vid.pal_host[0];
		return;
	}
}

void mem68k_store_setting_long(Uint32 addr, Uint32 data) {
	//printf("setting long\n");
}

/**** MEMCARD ****/
void mem68k_store_memcrd_byte(Uint32 addr, Uint8 data) {
	addr &= 0xFFF;
	memory.memcard[addr >> 1] = data;
}
void mem68k_store_memcrd_word(Uint32 addr, Uint16 data) {
	addr &= 0xFFF;
	memory.memcard[addr >> 1] = data & 0xff;
}
void mem68k_store_memcrd_long(Uint32 addr, Uint32 data) {
}

/**** bankswitchers ****/
static Uint16 neogeo_rng = 0x2345;

Uint16 sma_random(void) {
	Uint16 old = neogeo_rng;

	Uint16 newbit = ((neogeo_rng >> 2) ^ (neogeo_rng >> 3) ^ (neogeo_rng >> 5)
			^ (neogeo_rng >> 6) ^ (neogeo_rng >> 7) ^ (neogeo_rng >> 11)
			^ (neogeo_rng >> 12) ^ (neogeo_rng >> 15)) & 1;

	neogeo_rng = (neogeo_rng << 1) | newbit;

	return old;
}

/* Normal bankswitcher */
Uint8 mem68k_fetch_bk_normal_byte(Uint32 addr) {
	addr &= 0xFFFFF;
    if (memory.bksw_unscramble) { /* SMA prot & random number generator */
        Uint32 a=addr&0xFFFFFE;
		if (a == 0xfe446) {
			//printf("Prot reading B %08x\n", addr);
			return (addr&0x1?0x9a:0x37);
		}
		if (memory.sma_rng_addr && addr>=0x2fff00 &&
            (((a & 0xFF) == (memory.sma_rng_addr & 0xFF)) || 
             ((a & 0xFF) == memory.sma_rng_addr >> 8))) {
            //printf("SMA_Random B %08x\n",addr);
			return (addr&0x1?sma_random()>>8:sma_random()&0xFF);
        }
	}
	return (READ_BYTE_ROM(memory.rom.cpu_m68k.p + bankaddress + addr));
}

Uint16 mem68k_fetch_bk_normal_word(Uint32 addr) {
	addr &= 0xFFFFF;
	if (memory.bksw_unscramble) { /* SMA prot & random number generator */
		if (addr == 0xfe446) {
			//printf("Prot reading W %08x\n", addr);
			return 0x9a37;
		}
		if (memory.sma_rng_addr && addr>=0x2fff00 &&
            (((addr & 0xFF) == (memory.sma_rng_addr & 0xFF)) || 
             ((addr & 0xFF) == memory.sma_rng_addr >> 8))) {
            //printf("SMA_Random W %08x\n",addr);
			return sma_random();
        }
	}
	return (READ_WORD_ROM(memory.rom.cpu_m68k.p + bankaddress + addr));
}

LONG_FETCH(mem68k_fetch_bk_normal)
;

static void bankswitch(Uint32 address, Uint8 data) {

	if (memory.rom.cpu_m68k.size <= 0x100000)
		return;

	if (address >= 0x2FFFF0) {
		data = data & 0x7;
		bankaddress = (data + 1) * 0x100000;
	} else
		return;

	if (bankaddress >= memory.rom.cpu_m68k.size)
		bankaddress = 0x100000;
	cpu_68k_bankswitch(bankaddress);
}

void mem68k_store_bk_normal_byte(Uint32 addr, Uint8 data) {
	//if (addr<0x2FFFF0)
	//printf("bankswitch_b %x %x\n", addr, data);
	bankswitch(addr, data);
}

void mem68k_store_bk_normal_word(Uint32 addr, Uint16 data) {
	//if (addr<0x2FFFF0) 
    //printf("bankswitch_w %x %x\n",addr,data);
	if (memory.bksw_unscramble && (addr & 0xFF) == memory.bksw_unscramble[0]) {
		/* unscramble bank number */
		data = 
            (((data >> memory.bksw_unscramble[1]) & 1) << 0) + 
            (((data	>> memory.bksw_unscramble[2]) & 1) << 1) + 
            (((data	>> memory.bksw_unscramble[3]) & 1) << 2) + 
            (((data	>> memory.bksw_unscramble[4]) & 1) << 3) + 
            (((data	>> memory.bksw_unscramble[5]) & 1) << 4) + 
            (((data	>> memory.bksw_unscramble[6]) & 1) << 5);

		bankaddress = 0x100000 + memory.bksw_offset[data];
		cpu_68k_bankswitch(bankaddress);
	} else
		bankswitch(addr, data);
}

LONG_STORE(mem68k_store_bk_normal)
;
