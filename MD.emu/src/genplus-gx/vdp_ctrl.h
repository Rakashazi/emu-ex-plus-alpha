/***************************************************************************************
 *  Genesis Plus
 *  Video Display Processor (68k & Z80 CPU interface)
 *
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003  Charles Mac Donald (original code)
 *  Eke-Eke (2007-2011), additional code & fixes for the GCN/Wii port
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ****************************************************************************************/

#ifndef _VDP_H_
#define _VDP_H_

#include <m68k/m68k.h>
#include <assert.h>

template <uint S>
union RamU
{
	uint8 b[S];
	uint16 s[S/2];
	uint32 l[S/4];

	// TODO: try using GCC 4.7's __builtin_assume_aligned & memcpy
	uint32a &getL(uint byteAddr) __attribute__ ((always_inline))
	{
		//assert((byteAddr & 0x3) == 0);
		return *((uint32a*)&b[byteAddr]);
		//return l[byteAddr >> 2];
	}

	uint16a &getS(uint byteAddr) __attribute__ ((always_inline))
	{
		//assert((byteAddr & 0x1) == 0);
		return *((uint16a*)&b[byteAddr]);
		//return s[byteAddr >> 1];
	}
};

/* VDP context */
extern uint8 reg[0x20] __attribute__ ((aligned (4)));
extern RamU<0x400> sat;
extern RamU<0x10000> vram;
extern RamU<0x80> cram;
extern RamU<0x80> vsram;
extern uint8 hint_pending;
extern uint8 vint_pending;
extern uint16 status;
extern uint32 dma_length;

/* Global variables */
extern uint16 ntab;
extern uint16 ntbb;
extern uint16 ntwb;
extern uint16 satb;
extern uint16 hscb;
extern uint8 bg_name_dirty[0x800] __attribute__ ((aligned (4)));
extern uint16 bg_name_list[0x800] __attribute__ ((aligned (4)));
extern uint16 bg_list_index;
extern uint8 bg_pattern_cache[0x80000] __attribute__ ((aligned (4)));
extern uint8 hscroll_mask;
extern uint8 playfield_shift;
extern uint8 playfield_col_mask;
extern uint16 playfield_row_mask;
extern uint8 odd_frame;
extern uint8 im2_flag;
extern uint8 interlaced;
extern uint8 vdp_pal;
extern uint16 v_counter;
extern uint16 vc_max;
//extern uint16 hscroll;
extern uint16 vscroll;
extern uint16 lines_per_frame;
extern int32 fifo_write_cnt;
extern uint32 fifo_lastwrite;
extern uint32 hvc_latch;
extern const uint8 *hctab;

/* Function pointers */
extern void (*vdp_68k_data_w)(unsigned int data);
extern void (*vdp_z80_data_w)(unsigned int data);
extern unsigned int (*vdp_68k_data_r)(void);
extern unsigned int (*vdp_z80_data_r)(void);

/* Function prototypes */
extern void vdp_init(void);
extern void vdp_reset(void);
extern int vdp_context_save(uint8 *state);
extern int vdp_context_load(uint8 *state);
extern void vdp_dma_update(unsigned int cycles);
extern void vdp_68k_ctrl_w(unsigned int data);
extern void vdp_z80_ctrl_w(unsigned int data);
extern void vdp_sms_ctrl_w(unsigned int data);
extern void vdp_tms_ctrl_w(unsigned int data);
extern unsigned int vdp_68k_ctrl_r(unsigned int cycles);
extern unsigned int vdp_z80_ctrl_r(unsigned int cycles);
extern unsigned int vdp_hvc_r(unsigned int cycles);
extern void vdp_test_w(unsigned int data);
#ifdef CONFIG_M68KEMU_CYCLONE
	extern int vdp_68k_irq_ack(int int_level);
#else
	extern int vdp_68k_irq_ack(M68KCPU &m68ki_cpu, int int_level);
#endif

#endif /* _VDP_H_ */
