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
#include <imagine/util/mayAliasInt.h>

template <unsigned S>
union RamU
{
	uint8 b[S];
	uint16 s[S/2];
	uint32 l[S/4];

	// TODO: try using GCC 4.7's __builtin_assume_aligned & memcpy
	uint32a &getL(unsigned byteAddr) __attribute__ ((always_inline))
	{
		//assert((byteAddr & 0x3) == 0);
		return *((uint32a*)&b[byteAddr]);
		//return l[byteAddr >> 2];
	}

	uint16a &getS(unsigned byteAddr) __attribute__ ((always_inline))
	{
		//assert((byteAddr & 0x1) == 0);
		return *((uint16a*)&b[byteAddr]);
		//return s[byteAddr >> 1];
	}
};

/* VDP context */
struct VDP
{
uint8 reg[0x20] __attribute__ ((aligned (4)));
RamU<0x400> sat;
RamU<0x10000> vram;
RamU<0x80> cram;
RamU<0x80> vsram;
uint8 hint_pending;
uint8 vint_pending;
uint16 status;
uint32 dma_length;

/* Global variables */
uint16 ntab;
uint16 ntbb;
uint16 ntwb;
uint16 satb;
uint16 hscb;
uint8 bg_name_dirty[0x800] __attribute__ ((aligned (4)));
uint16 bg_name_list[0x800] __attribute__ ((aligned (4)));
uint16 bg_list_index;
uint8 bg_pattern_cache[0x80000] __attribute__ ((aligned (4)));
uint8 hscroll_mask;
uint8 playfield_shift;
uint8 playfield_col_mask;
uint16 playfield_row_mask;
uint8 odd_frame;
uint8 im2_flag;
uint8 interlaced;
uint8 vdp_pal;
uint16 v_counter;
uint16 vc_max;
//uint16 hscroll;
uint16 vscroll;
uint16 lines_per_frame;
int32 fifo_write_cnt;
uint32 fifo_lastwrite;
uint32 hvc_latch;
const uint8 *hctab;

/* Function pointers */
void (*vdp_68k_data_w)(unsigned int data);
void (*vdp_z80_data_w)(unsigned int data);
unsigned int (*vdp_68k_data_r)(void);
unsigned int (*vdp_z80_data_r)(void);
};

extern VDP vdp;

static auto &reg = vdp.reg;
static auto &sat = vdp.sat;
static auto &vram = vdp.vram;
static auto &cram = vdp.cram;
static auto &vsram = vdp.vsram;
static auto &hint_pending = vdp.hint_pending;
static auto &vint_pending = vdp.vint_pending;
static auto &status = vdp.status;
static auto &dma_length = vdp.dma_length;

/* Global variables */
static auto &ntab = vdp.ntab;
static auto &ntbb = vdp.ntbb;
static auto &ntwb = vdp.ntwb;
static auto &satb = vdp.satb;
static auto &hscb = vdp.hscb;
static auto &bg_name_dirty = vdp.bg_name_dirty;
static auto &bg_name_list = vdp.bg_name_list;
static auto &bg_list_index = vdp.bg_list_index;
static auto &bg_pattern_cache = vdp.bg_pattern_cache;
static auto &hscroll_mask = vdp.hscroll_mask;
static auto &playfield_shift = vdp.playfield_shift;
static auto &playfield_col_mask = vdp.playfield_col_mask;
static auto &playfield_row_mask = vdp.playfield_row_mask;
static auto &odd_frame = vdp.odd_frame;
static auto &im2_flag = vdp.im2_flag;
static auto &interlaced = vdp.interlaced;
static auto &vdp_pal = vdp.vdp_pal;
static auto &v_counter = vdp.v_counter;
static auto &vc_max = vdp.vc_max;
static auto &vscroll = vdp.vscroll;
static auto &lines_per_frame = vdp.lines_per_frame;
static auto &fifo_write_cnt = vdp.fifo_write_cnt;
static auto &fifo_lastwrite = vdp.fifo_lastwrite;
static auto &hvc_latch = vdp.hvc_latch;
static auto &hctab = vdp.hctab;

/* Function pointers */
static auto &vdp_68k_data_w = vdp.vdp_68k_data_w;
static auto &vdp_z80_data_w = vdp.vdp_z80_data_w;
static auto &vdp_68k_data_r = vdp.vdp_68k_data_r;
static auto &vdp_z80_data_r = vdp.vdp_z80_data_r;

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
