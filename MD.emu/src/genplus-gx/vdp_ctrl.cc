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

#include "shared.h"
#include "vdp_render.h"
#include "hvc.h"
#include <imagine/logger/logger.h>
#include <imagine/pixmap/Pixmap.hh>
#ifndef NO_SCD
#include <scd/scd.h>
#endif

/* Mark a pattern as dirty */
#define MARK_BG_DIRTY(addr)                         \
{                                                   \
  name = (addr >> 5) & 0x7FF;                       \
  if(bg_name_dirty[name] == 0)                      \
  {                                                 \
    bg_name_list[bg_list_index++] = name;           \
  }                                                 \
  bg_name_dirty[name] |= (1 << ((addr >> 2) & 7));  \
}

#if 0
/* VDP context */
RamU<0x400> sat;     /* Internal copy of sprite attribute table */
RamU<0x10000> vram;  /* Video RAM (64K x 8-bit) */
RamU<0x80> cram;     /* On-chip color RAM (64 x 9-bit) */
RamU<0x80> vsram;    /* On-chip vertical scroll RAM (40 x 11-bit) */
uint8 reg[0x20] __attribute__ ((aligned (4)));      /* Internal VDP registers (23 x 8-bit) */
uint8 hint_pending;   /* 0= Line interrupt is pending */
uint8 vint_pending;   /* 1= Frame interrupt is pending */
uint16 status;        /* VDP status flags */
uint32 dma_length;    /* DMA remaining length */

/* Global variables */
uint16 ntab;                      /* Name table A base address */
uint16 ntbb;                      /* Name table B base address */
uint16 ntwb;                      /* Name table W base address */
uint16 satb;                      /* Sprite attribute table base address */
uint16 hscb;                      /* Horizontal scroll table base address */
uint8 bg_name_dirty[0x800] __attribute__ ((aligned (4)));       /* 1= This pattern is dirty */
uint16 bg_name_list[0x800] __attribute__ ((aligned (4)));       /* List of modified pattern indices */
uint16 bg_list_index;             /* # of modified patterns in list */
uint8 bg_pattern_cache[0x80000] __attribute__ ((aligned (4)));  /* Cached and flipped patterns */
uint8 hscroll_mask;               /* Horizontal Scrolling line mask */
uint8 playfield_shift;            /* Width of planes A, B (in bits) */
uint8 playfield_col_mask;         /* Playfield column mask */
uint16 playfield_row_mask;        /* Playfield row mask */
uint16 vscroll;                   /* Latched vertical scroll value */
uint8 odd_frame;                  /* 1: odd field, 0: even field */
uint8 im2_flag;                   /* 1= Interlace mode 2 is being used */
uint8 interlaced;                 /* 1: Interlaced mode 1 or 2 */
uint8 vdp_pal;                    /* 1: PAL , 0: NTSC (default) */
uint16 v_counter;                 /* Vertical counter */
uint16 vc_max;                    /* Vertical counter overflow value */
uint16 lines_per_frame;           /* PAL: 313 lines, NTSC: 262 lines */
int32 fifo_write_cnt;             /* VDP writes fifo count */
uint32 fifo_lastwrite;            /* last VDP write cycle */
uint32 hvc_latch;                 /* latched HV counter */
const uint8 *hctab;               /* pointer to H Counter table */
//static bool dmaStarting = 0;

/* Function pointers */
void (*vdp_68k_data_w)(unsigned int data);
void (*vdp_z80_data_w)(unsigned int data);
unsigned int (*vdp_68k_data_r)(void);
unsigned int (*vdp_z80_data_r)(void);
#endif

static_assert(RENDER_BPP == 32 || RENDER_BPP == 16);
static constexpr auto fbPixelFormat = RENDER_BPP == 32 ? IG::PixelFmtRGBA8888 : IG::PixelFmtRGB565;

static Pixel frameBufferData[320 * 240];

VDP vdp;

/* Tables that define the playfield layout */
static const uint8 hscroll_mask_table[] = { 0x00, 0x07, 0xF8, 0xFF };
static const uint8 shift_table[]        = { 6, 7, 0, 8 };
static const uint8 col_mask_table[]     = { 0x0F, 0x1F, 0x0F, 0x3F };
static const uint16 row_mask_table[]    = { 0x0FF, 0x1FF, 0x2FF, 0x3FF };

static uint8 border;          /* Border color index */
static uint8 pending;         /* Pending write flag */
static uint8 code;            /* Code register */
static uint8 dma_type;        /* DMA mode */
static uint16 dmafill;        /* DMA Fill setup */
static uint16 addr;           /* Address register */
static uint16 addr_latch;     /* Latched A15, A14 of address */
static uint16 sat_base_mask;  /* Base bits of SAT */
static uint16 sat_addr_mask;  /* Index bits of SAT */
static uint32 dma_endCycles;  /* 68k cycles to DMA end */
static uint32 fifo_latency;   /* CPU access latency */
static int32 cached_write;      /* 2nd part of 32-bit CTRL port write */
static uint16 fifo[4] __attribute__ ((aligned (4)));        /* FIFO buffer */

/* set Z80 or 68k interrupt lines */
static void (*set_irq_line)(unsigned int level);
static void (*set_irq_line_delay)(unsigned int level);

/* DMA Timings */
static const uint8 dma_timing[2][2] =
{
/* H32, H40 */
  {16 , 18},  /* active display */
  {167, 205}  /* blank display */
};

/* Vertical counter overflow values (see hvc.h) */
static const uint16 vc_table[4][2] =
{
  /* NTSC, PAL */
  {0xDA , 0xF2},  /* Mode 4 (192 lines) */
  {0xEA , 0x102}, /* Mode 5 (224 lines) */
  {0xDA , 0xF2},  /* Mode 4 (192 lines) */
  {0x106, 0x10A}  /* Mode 5 (240 lines) */
};


/*--------------------------------------------------------------------------*/
/* Function prototypes                                                      */
/*--------------------------------------------------------------------------*/

static void vdp_68k_data_w_m4(unsigned int data);
static void vdp_68k_data_w_m5(unsigned int data);
static unsigned int vdp_68k_data_r_m4(void);
static unsigned int vdp_68k_data_r_m5(void);
static void vdp_z80_data_w_m4(unsigned int data);
static void vdp_z80_data_w_m5(unsigned int data);
static unsigned int vdp_z80_data_r_m4(void);
static unsigned int vdp_z80_data_r_m5(void);
static void vdp_bus_w(unsigned int data);
static void vdp_fifo_update(unsigned int cycles);
static void vdp_reg_w(unsigned int r, unsigned int d, unsigned int cycles);
static void vdp_dma_copy(int length);
static void vdp_dma_vbus(int length);
static void vdp_dma_fill(unsigned int data, int length);

static void z80_set_irq_line(unsigned int level)
{
	Z80.setIRQ(level);
}

static void m68k_set_irq(unsigned int level)
{
	mm68k.setIRQ(level);
}

static void m68k_set_irq_delay(unsigned int level)
{
	mm68k.setIRQDelay(level);
}

/*--------------------------------------------------------------------------*/
/* Init, reset, context functions                                           */
/*--------------------------------------------------------------------------*/

void vdp_init(void)
{
  /* PAL/NTSC timings */
  lines_per_frame = vdp_pal ? 313: 262;

  /* CPU interrupt line(s)*/
  switch (system_hw)
  {
    case SYSTEM_GENESIS:
    //case SYSTEM_PICO:
    {
      /* 68k cpu */
      set_irq_line = m68k_set_irq;
      set_irq_line_delay = m68k_set_irq_delay;
      break;
    }

    default:
    {
      /* Z80 cpu */
      set_irq_line = z80_set_irq_line;
      set_irq_line_delay = z80_set_irq_line;
      break;
    }
  }
}

void vdp_reset(void)
{
  memset ((char *) sat.b, 0, sizeof (sat));
  memset ((char *) vram.b, 0, sizeof (vram));
  memset ((char *) cram.b, 0, sizeof (cram));
  memset ((char *) vsram.b, 0, sizeof (vsram));
  memset ((char *) reg, 0, sizeof (reg));

  //dmaStarting = 0;

  addr            = 0;
  addr_latch      = 0;
  code            = 0;
  pending         = 0;
  border          = 0;
  hint_pending    = 0;
  vint_pending    = 0;
  dmafill         = 0;
  dma_type        = 0;
  dma_length      = 0;
  dma_endCycles   = 0;
  odd_frame       = 0;
  im2_flag        = 0;
  interlaced      = 0;
  fifo_write_cnt  = 0;
  fifo_lastwrite  = 0;
  cached_write   = -1;

  ntab = 0;
  ntbb = 0;
  ntwb = 0;
  satb = 0;
  hscb = 0;

  vscroll = 0;

  hscroll_mask        = 0x00;
  playfield_shift     = 6;
  playfield_col_mask  = 0x0F;
  playfield_row_mask  = 0x0FF;
  sat_base_mask       = 0xFE00;
  sat_addr_mask       = 0x01FF;

  /* clear pattern cache */
  bg_list_index = 0;
  memset ((char *) bg_name_dirty, 0, sizeof (bg_name_dirty));
  memset ((char *) bg_name_list, 0, sizeof (bg_name_list));
  memset ((char *) bg_pattern_cache, 0, sizeof (bg_pattern_cache));

  /* default HVC */
  hvc_latch = 0x10000;
  hctab = cycle2hc32;
  vc_max = vc_table[0][vdp_pal];
  v_counter = lines_per_frame - 1;

  /* default Window clipping */
  window_clip(0,0);

  /* default FIFO timings */
  fifo_latency = 214;

  /* reset VDP status (FIFO empty flag is set) */
  status = vdp_pal | 0x200;

  /* default display area */
  bitmap.viewport.w   = 256;
  bitmap.viewport.h   = 192;
  bitmap.viewport.ow  = 256;
  bitmap.viewport.oh  = 192;

  /* default overscan area */
  bitmap.viewport.x = (config_overscan & 2) * 7;
  bitmap.viewport.y = (config_overscan & 1) * 24 * (vdp_pal + 1);

  /* default rendering mode */
  update_bg_pattern_cache = update_bg_pattern_cache_m4;
  render_bg = render_bg_m4;
  render_obj = render_obj_m4;
  parse_satb = parse_satb_m4;

  /* default bus access mode */
  vdp_68k_data_w = vdp_68k_data_w_m4;
  vdp_68k_data_r = vdp_68k_data_r_m4;

  vdp_z80_data_w = vdp_z80_data_w_m4;
  vdp_z80_data_r = vdp_z80_data_r_m4;

  /* initialize some registers if OS ROM is simulated */
  if (config.tmss == 1)
  {
    vdp_reg_w(0 , 0x04, 0);  /* Palette bit set */
    vdp_reg_w(1 , 0x04, 0);  /* Mode 5 enabled */
    vdp_reg_w(10, 0xff, 0);  /* HINT disabled */
    vdp_reg_w(12, 0x81, 0);  /* H40 mode */
    vdp_reg_w(15, 0x02, 0);  /* auto increment */
  }

  /* reset palette */
  int i;
  for(i = 0; i < 0x20; i ++)
  {
    color_update_m4(i, 0x00);
  }
  color_update_m4(0x40, 0x00);
}

int vdp_context_save(uint8 *state)
{
	//logMsg("saving VDP context");
  int bufferptr = 0;

  save_param(sat.b, sizeof(sat));
  save_param(vram.b, sizeof(vram));
  save_param(cram.b, sizeof(cram));
  save_param(vsram.b, sizeof(vsram));
  save_param(reg, sizeof(reg));
  save_param(&addr, sizeof(addr));
  save_param(&addr_latch, sizeof(addr_latch));
  save_param(&code, sizeof(code));
  save_param(&pending, sizeof(pending));
  save_param(&status, sizeof(status));
  save_param(&dmafill, sizeof(dmafill));
  save_param(&hint_pending, sizeof(hint_pending));
  save_param(&vint_pending, sizeof(vint_pending));
  //save_param(&mm68k.irq_state, sizeof(mm68k.irq_state));
  uint8 dummyIrqState = 0;
  save_param(&dummyIrqState, sizeof(dummyIrqState));
  save_param(&dma_length, sizeof(dma_length));
  save_param(&dma_type, sizeof(dma_type));
  save_param(&cached_write, sizeof(cached_write));

  return bufferptr;
}

int vdp_context_load(uint8 *state)
{
	//logMsg("loading VDP context");
  int i, bufferptr = 0;
  uint8 temp_reg[0x20];

  load_param(sat.b, sizeof(sat));
  load_param(vram.b, sizeof(vram));
  load_param(cram.b, sizeof(cram));
  load_param(vsram.b, sizeof(vsram));
  load_param(temp_reg, sizeof(temp_reg));
  load_param(&addr, sizeof(addr));
  load_param(&addr_latch, sizeof(addr_latch));
  load_param(&code, sizeof(code));
  load_param(&pending, sizeof(pending));
  load_param(&status, sizeof(status));
  load_param(&dmafill, sizeof(dmafill));
  load_param(&hint_pending, sizeof(hint_pending));
  load_param(&vint_pending, sizeof(vint_pending));
  uint8 dummyIrqState;
  load_param(&dummyIrqState, sizeof(dummyIrqState));
  load_param(&dma_length, sizeof(dma_length));
  load_param(&dma_type, sizeof(dma_type));
  load_param(&cached_write, sizeof(cached_write));

  /* restore VDP registers */
  for (i=0;i<0x20;i++)
  {
    vdp_reg_w(i, temp_reg[i], 0);
  }

  /* restore FIFO timings */
  fifo_latency = 214 - (reg[12] & 1) * 24;
  fifo_latency <<= ((code & 0x0F) == 0x01);

  /* restore current NTSC/PAL mode */
  status = (status & ~1) | vdp_pal;

  if (reg[1] & 0x04)
  {
    /* Mode 5 */
    bg_list_index = 0x800;

    /* reinitialize palette */
    color_update_m5(0, cram.getS(border << 1));
    for(i = 1; i < 0x40; i++)
    {
      color_update_m5(i, cram.getS(i << 1));
    }
  }
  else
  {
    /* Mode 4 */
    bg_list_index = 0x200;

    /* reinitialize palette */
    for(i = 0; i < 0x20; i ++)
    {
      color_update_m4(i, cram.getS(i << 1));
    }
    color_update_m4(0x40, cram.getS((0x10 | (border & 0x0F)) << 1));
  }

  /* invalidate cache */
  for (i=0;i<bg_list_index;i++)
  {
    bg_name_list[i]=i;
    bg_name_dirty[i]=0xFF;
  }

  return bufferptr;
}


/*--------------------------------------------------------------------------*/
/* DMA update function                                                      */
/*--------------------------------------------------------------------------*/

void vdp_dma_update(unsigned int cycles)
{
  int dma_cycles;

  /* DMA transfer rate (bytes per line)

     According to the manual, here's a table that describes the transfer
   rates of each of the three DMA types:

      DMA Mode      Width       Display      Transfer Count
      -----------------------------------------------------
      68K > VDP     32-cell     Active       16
                                Blanking     167
                    40-cell     Active       18
                                Blanking     205
      VRAM Fill     32-cell     Active       15
                                Blanking     166
                    40-cell     Active       17
                                Blanking     204
      VRAM Copy     32-cell     Active       8
                                Blanking     83
                    40-cell     Active       9
                                Blanking     102

   'Active' is the active display period, 'Blanking' is either the vertical
   blanking period or when the display is forcibly blanked via register #1.

   The above transfer counts are all in bytes, unless the destination is
   CRAM or VSRAM for a 68K > VDP transfer, in which case it is in words.
  */
  unsigned int rate = dma_timing[(status & 8) || !(reg[1] & 0x40)][reg[12] & 1];

  /* Adjust for 68k bus DMA to VRAM (one word = 2 access) or DMA Copy (one read + one write = 2 access) */
  rate = rate >> (dma_type & 1);

  /* Remaining DMA cycles */
  if (status & 8)
  {
    /* Process DMA until the end of VBLANK (speed optimization) */
    /* Note: This is not 100% accurate since rate could change if display width */
    /* is changed during VBLANK but no games seem to do this. */
    dma_cycles = (lines_per_frame * MCYCLES_PER_LINE) - cycles;
  }
  else
  {
    /* Process DMA until the end of current line */
    dma_cycles = (mcycles_vdp + MCYCLES_PER_LINE) - cycles;
  }

  /* Remaining DMA bytes for that line */
  unsigned dma_bytes = (dma_cycles * rate) / MCYCLES_PER_LINE;

#ifdef LOGVDP
  error("[%d(%d)][%d(%d)] DMA type %d (%d access/line)(%d cycles left)-> %d access (%d remaining) (%x)\n", v_counter, mm68k.cycleCount/MCYCLES_PER_LINE, mm68k.cycleCount, mm68k.cycleCount%MCYCLES_PER_LINE,dma_type/4, rate, dma_cycles, dma_bytes, dma_length, m68k_get_reg (NULL, M68K_REG_PC));
#endif

  /* Check if DMA can be finished before the end of current line */
  if (dma_length < dma_bytes)
  {
    /* Adjust remaining DMA bytes */
    dma_bytes = dma_length;
    dma_cycles = (dma_bytes * MCYCLES_PER_LINE) / rate;
  }

  /* Update DMA timings */
  if (dma_type < 2)
  {
    /* 68K is frozen during DMA from V-Bus */
    mm68k.cycleCount = cycles + dma_cycles;
  #ifdef LOGVDP
    error("-->CPU frozen for %d cycles\n", dma_cycles);
  #endif
  }
  else
  {
    /* Set DMA Busy flag */
    status |= 0x02;

    /* 68K is still running, set DMA end cycle */
    dma_endCycles = cycles + dma_cycles;
#ifdef LOGVDP
    error("-->DMA ends in %d cycles\n", dma_cycles);
#endif
  }

  /* Process DMA */
  if (dma_bytes > 0)
  {
    /* Update DMA length */
    dma_length -= dma_bytes;

    /* Select DMA operation */
    switch (dma_type)
    {
      case 0:
      case 1:
      {
        /* 68K bus to VRAM, CRAM or VSRAM */
        vdp_dma_vbus(dma_bytes);
        break;
      }

      case 2:
      {
        /* VRAM Fill */
        vdp_dma_fill(dmafill, dma_bytes);
        break;
      }

      case 3:
      {
        /* VRAM Copy */
        vdp_dma_copy(dma_bytes);
        break;
      }
    }

    /* Check if DMA is finished */
    if (!dma_length)
    {
      /* Reset DMA length registers */
      reg[19] = reg[20] = 0;

      /* Perform cached write, if any */
      if (cached_write >= 0)
      {
        vdp_68k_ctrl_w(cached_write);
        cached_write = -1;
      }
    }
  }
}


/*--------------------------------------------------------------------------*/
/* Control port access functions                                            */
/*--------------------------------------------------------------------------*/

void vdp_68k_ctrl_w(unsigned int data)
{
  /* Check pending flag */
  if (pending == 0)
  {
    /* A single long word write instruction could have started DMA with the first word */
    if (dma_length)
    {
      /* 68k is frozen during 68k bus DMA */
      /* Second word should be written after DMA completion */
      /* See Formula One & Kawasaki Superbike Challenge */
      if (dma_type < 2)
      {
        /* Latch second control word for later */
        cached_write = data;
        return;
      }
    }

    /* Check CD0-CD1 bits */
    if ((data & 0xC000) == 0x8000)
    {
      /* VDP register write */
    	//logMsg("VDP reg write from 68K");
      vdp_reg_w((data >> 8) & 0x1F, data & 0xFF, mm68k.cycleCount);
    }
    else
    {
      /* Set pending flag (Mode 5 only) */
      pending = reg[1] & 4;
    }

    /* Update address and code registers */
    addr = addr_latch | (data & 0x3FFF);
    code = ((code & 0x3C) | ((data >> 14) & 0x03));
    //logMsg("updated VDP addr 0x%X code 0x%X", addr, code);
  }
  else
  {
    /* Clear pending flag */
    pending = 0;

    /* Save address bits A15 and A14 */
    addr_latch = (data & 3) << 14;

    /* Update address and code registers */
    addr = addr_latch | (addr & 0x3FFF);
    code = ((code & 0x03) | ((data >> 2) & 0x3C));
    //logMsg("updated VDP addr 0x%X code 0x%X", addr, code);

    /* Detect DMA operation (CD5 bit set) */
    if ((code & 0x20) && (reg[1] & 0x10))
    {
      /* DMA type */
      switch (reg[23] >> 6)
      {
        case 2:
        {
          /* VRAM write operation only (Williams Greatest Hits after soft reset) */
          if ((code & 0x0F) == 1)
          {
            /* VRAM fill will be triggered by next write to DATA port */
            dmafill = 0x100;
          }
          break;
        }

        case 3:
        {
          /* VRAM read/write operation only */
          if ((code & 0x1F) == 0x10)
          {
            /* DMA length */
            dma_length = (reg[20] << 8) | reg[19];

            /* Zero DMA length */
            if (!dma_length)
            {
              dma_length = 0x10000;
            }

            /* VRAM copy */
            dma_type = 3;
            vdp_dma_update(mm68k.cycleCount);
          }
          break;
        }

        default:
        {
          /* DMA length */
          dma_length = (reg[20] << 8) | reg[19];

          /* Zero DMA length */
          if (!dma_length)
          {
            dma_length = 0x10000;
          }

					#ifndef NO_SCD
          unsigned int source = (reg[23] << 17 | reg[22] << 9 | reg[21] << 1) & 0xFFFFFE;
          if(sCD.isActive && (source>>16 >= 0x20) && (source>>16 <= 0x23))
					{
						//logMsg("dma latency from 0x%X to 0x%X, length 0x%X", source, addr, dma_length);
						addr += 2;
						dma_length--;
					}
					#endif

          /* SVP RAM transfer latency */
		  #ifndef NO_SVP
          reg[21] -= (svp && !(reg[23] & 0x60));
		  #endif

          /* 68k to VDP DMA */
          dma_type = (code & 0x06) ? 0 : 1;
          vdp_dma_update(mm68k.cycleCount);
          break;
        }
      }
    }
  }

  /*
     FIFO emulation (Chaos Engine/Soldier of Fortune, Double Clutch, Sol Deace)
     --------------------------------------------------------------------------

      CPU access per line is limited during active display:
         H32: 16 access --> 3420/16 = ~214 Mcycles between access
         H40: 18 access --> 3420/18 = ~190 Mcycles between access

      This is an approximation, on real hardware, the delay between access is
      more likely 16 pixels (128 or 160 Mcycles) with no access allowed during
      HBLANK (~860 Mcycles), H40 mode being probably a little more restricted.

      Each VRAM access is byte wide, so one VRAM write (word) need twice cycles.

  */
  fifo_latency = 214 - (reg[12] & 1) * 24;
  fifo_latency <<= ((code & 0x0F) == 0x01);
}

void vdp_z80_ctrl_w(unsigned int data)
{
  switch (pending)
  {
    case 0:
    {
      /* Latch LSB */
      addr_latch = data;

      /* Set LSB pending flag */
      pending = 1;
      return;
    }

    case 1:
    {
      /* Update address and code registers */
      addr = (addr & 0xC000) | ((data & 0x3F) << 8) | addr_latch ;
      code = ((code & 0x3C) | ((data >> 6) & 0x03));
      //logMsg("updated VDP addr 0x%X code 0x%X by Z80", addr, code);

      if ((code & 0x03) == 0x02)
      {
        /* VDP register write */
      	//logMsg("VDP reg write from Z80");
        vdp_reg_w(data & 0x1F, addr_latch, Z80.cycleCount);

        /* Clear pending flag  */
        pending = 0;
        return;
      }

      /* Set Mode 5 pending flag  */
      pending = (reg[1] & 4) >> 1;

      if (!pending && !(code & 0x03))
      {
        /* Process VRAM read */
        fifo[0] = vram.b[addr & 0x3FFF];

        /* Increment address register */
        addr += (reg[15] + 1);
      }
      return;
    }

    case 2:
    {
      /* Latch LSB */
      addr_latch = data;

      /* Set LSB pending flag */
      pending = 3;
      return;
    }

    case 3:
    {
      /* Clear pending flag  */
      pending = 0;

      /* Update address and code registers */
      addr = ((addr_latch & 3) << 14) | (addr & 0x3FFF);
      code = ((code & 0x03) | ((addr_latch >> 2) & 0x3C));
      //logMsg("updated VDP addr 0x%X code 0x%X", addr, code);

      /* Detect DMA operation */
      if ((code & 0x20) && (reg[1] & 0x10))
      {
        switch (reg[23] >> 6)
        {
          case 2:
          {
            /* VRAM write operation only (Williams Greatest Hits after soft reset) */
            if ((code & 0x0F) == 1)
            {
              /* VRAM fill will be triggered by next write to DATA port */
              dmafill = 0x100;
            }
            break;
          }

          case 3:
          {
            /* VRAM read/write operation only */
            if ((code & 0x1F) == 0x10)
            {
              /* DMA length */
              dma_length = (reg[20] << 8) | reg[19];

              /* Zero DMA length */
              if (!dma_length)
              {
                dma_length = 0x10000;
              }

              /* VRAM copy */
              dma_type = 3;
              vdp_dma_update(Z80.cycleCount);
            }
            break;
          }

          default:
          {
            /* DMA from V-Bus does not work when Z80 is in control */
            break;
          }
        }
      }
    }
    return;
  }
}

/*
 * Status register
 *
 * Bits
 * 0  NTSC(0)/PAL(1)
 * 1  DMA Busy
 * 2  During HBlank
 * 3  During VBlank
 * 4  0:1 even:odd field (interlaced modes only)
 * 5  Sprite collision
 * 6  Too many sprites per line
 * 7  v interrupt occurred
 * 8  Write FIFO full
 * 9  Write FIFO empty
 * 10 - 15  Open Bus
 */
unsigned int vdp_68k_ctrl_r(unsigned int cycles)
{
  /* Update FIFO flags */
  vdp_fifo_update(cycles);

  /* Update DMA Busy flag */
  if ((status & 2) && !dma_length && (cycles >= dma_endCycles))
  {
    status &= 0xFFFD;
  }

  /* Return VDP status */
  unsigned int temp = status;

  /* Clear pending flag */
  pending = 0;

  /* Clear SOVR & SCOL flags */
  status &= 0xFF9F;

  /* Display OFF: VBLANK flag is set */
  if (!(reg[1] & 0x40))
  {
    temp |= 0x08;
  }

  /* HBLANK flag (Sonic 3 and Sonic 2 "VS Modes", Lemmings 2, Mega Turrican, V.R Troopers, Gouketsuji Ichizoku,...) */
  /* NB: this is not 100% accurate and need to be verified on real hardware */
  if ((cycles % MCYCLES_PER_LINE) < 588)
  {
    temp |= 0x04;
  }

#if 0
  /* Mode 4 specific */
  if (!(reg[1] & 4))
  {
    /* Cycle-accurate VINT flag (required by some Master System games) */
    if ((v_counter == bitmap.viewport.h) && ((cycles / MCYCLES_PER_LINE) > (v_counter + 1)))
    {
      temp |= 0x80;
    }

    /* Clear HINT & VINT pending flags */
    hint_pending = vint_pending = 0;
    //*irq_line = 0x10;

    /* Clear VINT flag */
    status &= ~0x80;
  }
#endif

#ifdef LOGVDP
  error("[%d(%d)][%d(%d)] VDP status read -> 0x%x (0x%x) (%x)\n", v_counter, cycles/MCYCLES_PER_LINE, cycles, cycles%MCYCLES_PER_LINE, temp, status, m68k_get_reg (NULL, M68K_REG_PC));
#endif
  return (temp);
}

unsigned int vdp_z80_ctrl_r(unsigned int cycles)
{
  /* Update DMA Busy flag (Mega Drive VDP specific) */
  if (/*(system_hw & SYSTEM_MD) &&*/ (status & 2) && !dma_length && (cycles >= dma_endCycles))
  {
    status &= 0xFD;
  }

  /* Cycle-accurate SOVR & VINT flags */
  int line = (lines_per_frame + (Z80.cycleCount / MCYCLES_PER_LINE) - 1) % lines_per_frame;

  /* Check if we are already on next line */
  if (line > v_counter)
  {
    v_counter = line;
    if (line == (bitmap.viewport.h + 1))
    {
      /* set VINT flag (immediately cleared after) */
      status |= 0x80;
    }
    else if ((line >= 0) && (line < bitmap.viewport.h) && !(work_ram[0x1ffb] & cart.special))
    {
      /* Check sprites overflow & collision */
      render_line(line, framebufferPixmap());
    }
  }

  /* Return VDP status */
  unsigned int temp = status;

  /* Clear pending flag */
  pending = 0;

  /* Clear VINT, SOVR & SCOL flags */
  status &= 0xFF1F;

  /* Mega Drive VDP specific */
  //if (system_hw & SYSTEM_MD)
  {
    /* Display OFF: VBLANK flag is set */
    if (!(reg[1] & 0x40))
    {
      temp |= 0x08;
    }

    /* HBLANK flag */
    if ((cycles % MCYCLES_PER_LINE) < 588)
    {
      temp |= 0x04;
    }
  }
#if 0
  else if (reg[0] & 0x04)
  {
    /* Mode 4 unused bits (fixes PGA Tour Golf) */
    temp |= 0x1F;
  }
#endif

  /* Cycle-accurate SCOL flag */
  if ((temp & 0x20) && (line == (spr_col >> 8)))
  {
    //if (system_hw & SYSTEM_MD)
    {
      /* COL flag is set at HCount 0xFF on MD */
      if ((cycles % MCYCLES_PER_LINE) < 105)
      {
        status |= 0x20;
        temp &= ~0x20;
      }
    }
#if 0
    else
    {
      /* COL flag is set at the pixel it occurs */
      uint8 hc = hctab[(cycles + SMS_CYCLE_OFFSET + 15) % MCYCLES_PER_LINE];
      if ((hc < (spr_col & 0xff)) || (hc > 0xf3))
      {
        status |= 0x20;
        temp &= ~0x20;
      }
    }
#endif
  }

  /* Clear HINT & VINT pending flags */
  hint_pending = vint_pending = 0;

  /* Clear Z80 interrupt */
  Z80.irq_state = CLEAR_LINE;

#ifdef LOGVDP
  error("[%d(%d)][%d(%d)] VDP Z80 status read -> 0x%x (0x%x) (%x)\n", v_counter, cycles/MCYCLES_PER_LINE-1, cycles, cycles%MCYCLES_PER_LINE, temp, status, Z80.pc.w.l);
#endif
  return (temp);
}

/*--------------------------------------------------------------------------*/
/* HV Counters                                                              */
/*--------------------------------------------------------------------------*/

unsigned int vdp_hvc_r(unsigned int cycles)
{
  /* VCounter */
  int vc = (cycles / MCYCLES_PER_LINE) - 1;

  /* Check counter overflow */
  if (vc > vc_max)
  {
    vc -= lines_per_frame;
  }

  /* Check interlaced modes */
  if (interlaced)
  {
    /* Interlace mode 2 (Sonic the Hedgehog 2, Combat Cars) */
    vc <<= im2_flag;

    /* Replace bit 0 with bit 8 */
    vc = (vc & ~1) | ((vc >> 8) & 1);
  }

  /* Returned value */
  unsigned int temp = (vc & 0xff) << 8;

  /* Check if HVC is frozen */
  if (!hvc_latch)
  {
    /* Cycle-accurate HCounter (Striker, Mickey Mania, Skitchin, Road Rash I,II,III, Sonic 3D Blast...) */
    temp |= hctab[cycles % MCYCLES_PER_LINE];
  }
  else
  {
    if (reg[1] & 4)
    {
      /* Mode 5: both counters are frozen (Lightgun games, Sunset Riders) */
      temp = hvc_latch & 0xffff;
    }
    else
    {
      /* Mode 4: VCounter runs normally, HCounter is frozen */
      temp |= (hvc_latch & 0xff);
    }
  }

#ifdef LOGVDP
  error("[%d(%d)][%d(%d)] HVC read -> 0x%x (%x)\n", v_counter, cycles/MCYCLES_PER_LINE, cycles, cycles%MCYCLES_PER_LINE, temp, m68k_get_reg (NULL, M68K_REG_PC));
#endif
  return (temp);
}


/*--------------------------------------------------------------------------*/
/* Test registers                                                           */
/*--------------------------------------------------------------------------*/

void vdp_test_w(unsigned int data)
{
	logMsg("Unused VDP Write 0x%x (%08x)", data, m68k_get_reg (mm68k, M68K_REG_PC));
}


/*--------------------------------------------------------------------------*/
/* 68k interrupt handler (TODO: check how interrupts are handled in Mode 4) */
/*--------------------------------------------------------------------------*/

#ifdef CONFIG_M68KEMU_CYCLONE
int vdp_68k_irq_ack(int int_level)
#else
int vdp_68k_irq_ack(M68KCPU &m68ki_cpu, int int_level)
#endif
{
#ifdef LOGVDP
  error("[%d(%d)][%d(%d)] INT Level %d ack (%x)\n", v_counter, mm68k.cycleCount/MCYCLES_PER_LINE, mm68k.cycleCount, mm68k.cycleCount%MCYCLES_PER_LINE,int_level, m68k_get_reg (NULL, M68K_REG_PC));
#endif

  /* VINT has higher priority (Fatal Rewind) */
  if (vint_pending & reg[1])
  {
#ifdef LOGVDP
    error("---> VINT cleared\n");
#endif

    /* Clear VINT pending flag */
    vint_pending = 0;
    status &= ~0x80;

    /* Update IRQ status */
    if (hint_pending & reg[0])
    {
    	m68ki_cpu.setIRQ(4);
    }
    else
    {
    	m68ki_cpu.setIRQ(0);
    }
  }
  else
  {
#ifdef LOGVDP
    error("---> HINT cleared\n");
#endif

    /* Clear HINT pending flag */
    hint_pending = 0;

    /* Update IRQ status */
    m68ki_cpu.setIRQ(0);
  }

  return M68K_INT_ACK_AUTOVECTOR;
}


/*--------------------------------------------------------------------------*/
/* Internal registers access function                                       */
/*--------------------------------------------------------------------------*/

static void vdp_reg_w(unsigned int r, unsigned int d, unsigned int cycles)
{
#ifdef LOGVDP
  error("[%d(%d)][%d(%d)] VDP register %d write -> 0x%x (%x)\n", v_counter, cycles/MCYCLES_PER_LINE, cycles, cycles%MCYCLES_PER_LINE, r, d, m68k_get_reg (NULL, M68K_REG_PC));
#endif

  /* VDP registers #11 to #23 cannot be updated in Mode 4 (Captain Planet & Avengers, Bass Master Classic Pro Edition) */
  if (!(reg[1] & 4) && (r > 10))
  {
    return;
  }

  switch(r)
  {
    case 0: /* CTRL #1 */
    {
      /* Look for changed bits */
      r = d ^ reg[0];
      reg[0] = d;

      /* Line Interrupt */
      if ((r & 0x10) && hint_pending)
      {
        /* Update IRQ status */
        if (vint_pending & reg[1])
        {
        	set_irq_line(6);
        }
        else if (d & 0x10)
        {
        	set_irq_line_delay(4);
        }
        else
        {
        	set_irq_line(0);
        }
      }

      /* Palette selection */
      if (r & 0x04)
      {
        /* Reset color palette */
        int i;
        if (reg[1] & 0x04)
        {
          /* Mode 5 */
          color_update_m5(0x00, cram.getS(border << 1));
          for (i = 1; i < 0x40; i++)
          {
            color_update_m5(i, cram.getS(i << 1));
          }
        }
        else
        {
          /* Mode 4 */
          for (i = 0; i < 0x20; i++)
          {
            color_update_m4(i, cram.getS(i << 1));
          }
          color_update_m4(0x40, cram.getS((0x10 | (border & 0x0F)) << 1));
        }
      }

      /* HVC latch (Sunset Riders, Lightgun games) */
      if (r & 0x02)
      {
        /* Mode 5 only */
        if (reg[1] & 0x04)
        {
          if (d & 0x02)
          {
            /* Latch current HVC */
            hvc_latch = vdp_hvc_r(cycles) | 0x10000;
          }
          else
          {
            /* Free-running HVC */
            hvc_latch = 0;
          }
        }
      }
      break;
    }

    case 1: /* CTRL #2 */
    {
      /* Look for changed bits */
      r = d ^ reg[1];
      reg[1] = d;

      //logMsg("vdp reg 1 %X", d);
      /*if(d & 0x10 && !dmaStarting)
      {
      	unsigned int source = (reg[23] << 17 | reg[22] << 9 | reg[21] << 1) & 0xFFFFFE;
      	//logMsg("dma enable with source %X", source);
      	dmaStarting = 1;
      }
      else if(!(r & 0x10))
      {
      	//logMsg("dma disable");
      }*/

      /* Display status (modified during active display) */
      if ((r & 0x40) && (v_counter < bitmap.viewport.h))
      {
        /* Cycle offset vs HBLANK */
        int offset = cycles - mcycles_vdp - 860;
        if (offset <= 0)
        {
          /* If display was disabled during HBLANK (Mickey Mania 3D level), sprite rendering is limited  */
          if ((d & 0x40) && (object_count > 5) && (offset >= -500))
          {
            object_count = 5;
          }

          /* Redraw entire line (Legend of Galahad, Lemmings 2, Formula One, Kawasaki Super Bike, Deadly Moves,...) */
          render_line(v_counter, framebufferPixmap());

#ifdef LOGVDP
          error("Line redrawn (%d sprites) \n",object_count);
#endif
        }
        else
        {
          /* Active pixel offset  */
          if (reg[12] & 1)
          {
            /* dot clock = MCLK / 8 */
            offset = (offset / 8);
          }
          else
          {
            /* dot clock = MCLK / 10 */
            offset = (offset / 10) + 16;
          }

          /* Line is partially blanked (Nigel Mansell's World Championship Racing , Ren & Stimpy Show, ...) */
          if (offset < bitmap.viewport.w)
          {
#ifdef LOGVDP
            error("Line %d redrawn from pixel %d\n",v_counter,offset);
#endif
            if (d & 0x40)
            {
              render_line(v_counter, framebufferPixmap());
              blank_line(v_counter, 0, offset);
            }
            else
            {
              blank_line(v_counter, offset, bitmap.viewport.w - offset);
            }
          }
        }
      }

      /* Frame Interrupt */
      if ((r & 0x20) && vint_pending)
      {
        /* Update IRQ status */
        if (d & 0x20)
        {
        	set_irq_line_delay(6);
        }
        else if (hint_pending & reg[0])
        {
        	set_irq_line(4);
        }
        else
        {
        	set_irq_line(0);
        }
      }

      /* Active display height (Mode 5 only) */
      if (r & 0x08)
      {
        /* Mode 5 only */
        if (d & 0x04)
        {
          if (v_counter < bitmap.viewport.h)
          {
            /* Update active display height */
            bitmap.viewport.h = 224 + ((d & 8) << 1);
            bitmap.viewport.y = (config_overscan & 1) * (8 - (d & 8) + 24*vdp_pal);
          }
          else
          {
            /* Changes should be applied on next frame */
            bitmap.viewport.changed |= 2;
          }

          /* Update vertical counter max value */
          vc_max = vc_table[(d >> 2) & 3][vdp_pal];
        }
      }

      /* Rendering mode */
      if (r & 0x04)
      {
        /* Mega Drive VDP only */
        if(1)//(system_hw == SYSTEM_GENESIS)
        {
          int i;
          if (d & 0x04)
          {
            /* Mode 5 rendering */
            parse_satb = parse_satb_m5;
            update_bg_pattern_cache = update_bg_pattern_cache_m5;
            if (im2_flag)
            {
              render_bg = (reg[11] & 0x04) ? render_bg_m5_im2_vs : render_bg_m5_im2;
              render_obj = (reg[12] & 0x08) ? render_obj_m5_im2_ste : render_obj_m5_im2;
            }
            else
            {
              render_bg = (reg[11] & 0x04) ? render_bg_m5_vs : render_bg_m5;
              render_obj = (reg[12] & 0x08) ? render_obj_m5_ste : render_obj_m5;
            }

            /* Reset color palette */
            color_update_m5(0x00, cram.getS(border << 1));
            for (i = 1; i < 0x40; i++)
            {
              color_update_m5(i, cram.getS(i << 1));
            }

            /* Mode 5 bus access */
            vdp_68k_data_w = vdp_68k_data_w_m5;
            vdp_z80_data_w = vdp_z80_data_w_m5;
            vdp_68k_data_r = vdp_68k_data_r_m5;
            vdp_z80_data_r = vdp_z80_data_r_m5;

            /* Change display height */
            if (v_counter < bitmap.viewport.h)
            {
              /* Update active display */
              bitmap.viewport.h = 224 + ((d & 8) << 1);
              bitmap.viewport.y = (config_overscan & 1) * (8 - (d & 8) + 24*vdp_pal);
            }
            else
            {
              /* Changes should be applied on next frame */
              bitmap.viewport.changed |= 2;
            }

            /* Clear HVC latched value */
            hvc_latch = 0;

            /* Check if HVC latch bit is set */
            if (reg[0] & 0x02)
            {
              /* Latch current HVC */
              hvc_latch = vdp_hvc_r(cycles) | 0x10000;
            }

            /* max tiles to invalidate */
            bg_list_index = 0x800;
          }
          else
          {
            /* Mode 4 rendering */
            parse_satb = parse_satb_m4;
            update_bg_pattern_cache = update_bg_pattern_cache_m4;
            render_bg = render_bg_m4;
            render_obj = render_obj_m4;

            /* Reset color palette */
            for (i = 0; i < 0x20; i++)
            {
              color_update_m4(i, cram.getS(i << 1));
            }
            color_update_m4(0x40, cram.getS((0x10 | (border & 0x0F)) << 1));

            /* Mode 4 bus access */
            vdp_68k_data_w = vdp_68k_data_w_m4;
            vdp_z80_data_w = vdp_z80_data_w_m4;
            vdp_68k_data_r = vdp_68k_data_r_m4;
            vdp_z80_data_r = vdp_z80_data_r_m4;

            if (v_counter < bitmap.viewport.h)
            {
              /* Update active display height */
              bitmap.viewport.h = 192;
              bitmap.viewport.y = (config_overscan & 1) * 24 * (vdp_pal + 1);
            }
            else
            {
              /* Changes should be applied on next frame */
              bitmap.viewport.changed |= 2;
            }

            /* Latch current HVC */
            hvc_latch = vdp_hvc_r(cycles) | 0x10000;

            /* max tiles to invalidate */
            bg_list_index = 0x200;
          }

          /* Invalidate pattern cache */
          for (i=0;i<bg_list_index;i++)
          {
            bg_name_list[i] = i;
            bg_name_dirty[i] = 0xFF;
          }

          /* Update vertical counter max value */
          vc_max = vc_table[(d >> 2) & 3][vdp_pal];
        }
        else
        {
          /* No effect (cleared to avoid mode 5 detection elsewhere) */
          reg[1] &= ~0x04;
        }
      }
      break;
    }

    case 2: /* Plane A Name Table Base */
    {
      reg[2] = d;
      ntab = (d << 10) & 0xE000;

      /* Plane A Name Table Base changed during HBLANK */
      if ((v_counter < bitmap.viewport.h) && (reg[1] & 0x40) && (cycles <= (mcycles_vdp + 860)))
      {
        /* render entire line */
        render_line(v_counter, framebufferPixmap());
      }
      break;
    }

    case 3: /* Window Plane Name Table Base */
    {
      reg[3] = d;
      if (reg[12] & 0x01)
      {
        ntwb = (d << 10) & 0xF000;
      }
      else
      {
        ntwb = (d << 10) & 0xF800;
      }

			/* Window Plane Name Table Base changed during HBLANK */
			if ((v_counter < bitmap.viewport.h) && (reg[1] & 0x40) && (cycles <= (mcycles_vdp + 860)))
			{
				/* render entire line */
				render_line(v_counter, framebufferPixmap());
			}
      break;
    }

    case 4: /* Plane B Name Table Base */
    {
      reg[4] = d;
      ntbb = (d << 13) & 0xE000;

      /* Plane B Name Table Base changed during HBLANK (Adventures of Batman & Robin) */
		  if ((v_counter < bitmap.viewport.h) && (reg[1] & 0x40) && (cycles <= (mcycles_vdp + 860)))
		  {
		 	  /* render entire line */
	 		  render_line(v_counter, framebufferPixmap());
 		  }

      break;
    }

    case 5: /* Sprite Attribute Table Base */
    {
      reg[5] = d;
      satb = (d << 9) & sat_base_mask;
      break;
    }

    case 7: /* Backdrop color */
    {
      reg[7] = d;

      /* Check if backdrop color changed */
      d &= 0x3F;

      if (d != border)
      {
        /* Update backdrop color */
        border = d;

        /* Reset palette entry */
        if (reg[1] & 4)
        {
          /* Mode 5 */
          color_update_m5(0x00, cram.getS(d << 1));
        }
        else
        {
          /* Mode 4 */
          color_update_m4(0x40, cram.getS((0x10 | (d & 0x0F)) << 1));
        }

        /* Backdrop color modified during HBLANK (Road Rash 1,2,3)*/
        if ((v_counter < bitmap.viewport.h) && (cycles <= (mcycles_vdp + 860)))
        {
          /* remap entire line */
          remap_line(v_counter, framebufferPixmap());
        }
      }
      break;
    }

    case 8:   /* Horizontal Scroll (Mode 4 only) */
    {
      /* Hscroll is latched at HCount 0xF3, HCount 0xF6 on MD */
      /* Line starts at HCount 0xF4, HCount 0xF6 on MD */
      /*if (system_hw < SYSTEM_MD)
      {
        cycles = cycles + 15;
      }*/

      /* Make sure Hscroll has not already been latched */
      int line = (lines_per_frame + (cycles / MCYCLES_PER_LINE) - 1) % lines_per_frame;
      if ((line > v_counter) && (line < bitmap.viewport.h) && !(work_ram[0x1ffb] & cart.special))
      {
        v_counter = line;
        render_line(line, framebufferPixmap());
      }

      reg[8] = d;
      break;
    }

    case 11:  /* CTRL #3 */
    {
      reg[11] = d;

      /* Horizontal scrolling mode */
      hscroll_mask = hscroll_mask_table[d & 0x03];

      /* Vertical Scrolling mode */
      if (d & 0x04)
      {
        render_bg = im2_flag ? render_bg_m5_im2_vs : render_bg_m5_vs;
      }
      else
      {
        render_bg = im2_flag ? render_bg_m5_im2 : render_bg_m5;
      }
      break;
    }

    case 12:  /* CTRL #4 */
    {
      /* Look for changed bits */
      r = d ^ reg[12];
      reg[12] = d;

      /* Shadow & Highlight mode */
      if (r & 0x08)
      {
        /* Reset color palette */
        int i;
        color_update_m5(0x00, cram.getS(border << 1));
        for (i = 1; i < 0x40; i++)
        {
          color_update_m5(i, cram.getS(i << 1));
        }

        /* Update sprite rendering function */
        if (d & 0x08)
        {
          render_obj = im2_flag ? render_obj_m5_im2_ste : render_obj_m5_ste;
        }
        else
        {
          render_obj = im2_flag ? render_obj_m5_im2 : render_obj_m5;
        }
      }

      /* Interlaced modes */
      if (r & 0x06)
      {
        /* changes should be applied on next frame */
        bitmap.viewport.changed |= 2;
      }

      /* Active display width */
      if (r & 0x01)
      {
        if (d & 0x01)
        {
          /* Update display-dependant registers */
          ntwb = (reg[3] << 10) & 0xF000;
          satb = (reg[5] << 9) & 0xFC00;
          sat_base_mask = 0xFC00;
          sat_addr_mask = 0x03FF;

          /* Update HC table */
          hctab = cycle2hc40;

          /* Update clipping */
          window_clip(reg[17], 1);

          /* Update fifo timings */
          fifo_latency = 190;
        }
        else
        {
          /* Update display-dependant registers */
          ntwb = (reg[3] << 10) & 0xF800;
          satb = (reg[5] << 9) & 0xFE00;
          sat_base_mask = 0xFE00;
          sat_addr_mask = 0x01FF;

          /* Update HC table */
          hctab = cycle2hc32;

          /* Update clipping */
          window_clip(reg[17], 0);

          /* Update FIFO timings */
          fifo_latency = 214;
        }

        /* Adjust FIFO timings for VRAM writes */
        fifo_latency <<= ((code & 0x0F) == 0x01);

        /* Active display width modified during HBLANK (Bugs Bunny Double Trouble) */
        if ((v_counter < bitmap.viewport.h) && (cycles <= (mcycles_vdp + 860)))
        {
          /* Update active display width */
          bitmap.viewport.w = 256 + ((d & 1) << 6);

          /* Redraw entire line */
          render_line(v_counter, framebufferPixmap());
        }
        else
        {
          /* Changes should be applied on next frame (Golden Axe III intro) */
          /* NB: This is not 100% accurate but is required by GCN/Wii port (GX texture direct mapping) */
          /* and isn't noticeable anyway since display is generally disabled when active width is modified */
          bitmap.viewport.changed |= 2;
        }
      }
      break;
    }

    case 13: /* HScroll Base Address */
    {
      reg[13] = d;
      hscb = (d << 10) & 0xFC00;
      break;
    }

    case 16: /* Playfield size */
    {
      reg[16] = d;
      playfield_shift = shift_table[(d & 3)];
      playfield_col_mask = col_mask_table[(d & 3)];
      playfield_row_mask = row_mask_table[(d >> 4) & 3];
      break;
    }

    case 17: /* Window/Plane A vertical clipping */
    {
      reg[17] = d;
      window_clip(d, reg[12] & 1);
      break;
    }

    default:
    {
      reg[r] = d;
      break;
    }
  }
}


/*--------------------------------------------------------------------------*/
/* FIFO update function                                                     */
/*--------------------------------------------------------------------------*/

static void vdp_fifo_update(unsigned int cycles)
{
  if (fifo_write_cnt > 0)
  {
    /* Get number of FIFO reads */
    int fifo_read = ((cycles - fifo_lastwrite) / fifo_latency);

    if (fifo_read > 0)
    {
      /* Process FIFO entries */
      fifo_write_cnt -= fifo_read;

      /* Clear FIFO full flag */
      status &= 0xFEFF;

      /* Check remaining FIFO entries */
      if (fifo_write_cnt <= 0)
      {
        /* Set FIFO empty flag */
        status |= 0x200;
        fifo_write_cnt = 0;
      }

      /* Update FIFO cycle count */
      fifo_lastwrite += (fifo_read * fifo_latency);
    }
  }
}


/*--------------------------------------------------------------------------*/
/* Internal 16-bit data bus access function (Mode 5 only)                   */
/*--------------------------------------------------------------------------*/

static void vdp_bus_w(unsigned int data)
{
  /* Check destination code */
  switch (code & 0x0F)
  {
    case 0x01:  /* VRAM */
    {
#ifdef LOGVDP
      error("[%d(%d)][%d(%d)] VRAM 0x%x write -> 0x%x (%x)\n", v_counter, mm68k.cycleCount/MCYCLES_PER_LINE, mm68k.cycleCount, mm68k.cycleCount%MCYCLES_PER_LINE, addr, data, m68k_get_reg (NULL, M68K_REG_PC));
#endif
      /* Byte-swap data if A0 is set */
      if (addr & 1)
      {
        data = ((data >> 8) | (data << 8)) & 0xFFFF;
      }

      /* VRAM address */
      int index = addr & 0xFFFE;

      /* Pointer to VRAM */
      uint16 *p = (uint16 *)&vram.b[index];

      /* Intercept writes to Sprite Attribute Table */
      if ((index & sat_base_mask) == satb)
      {
        /* Update internal SAT */
        sat.getS(index & sat_addr_mask) = data;
      }

      /* Only write unique data to VRAM */
      if (data != *p)
      {
        /* Write data to VRAM */
        *p = data;

        /* Update pattern cache */
        int name;
        MARK_BG_DIRTY (index);
      }
      break;
    }

    case 0x03:  /* CRAM */
    {
#ifdef LOGVDP
      error("[%d(%d)][%d(%d)] CRAM 0x%x write -> 0x%x (%x)\n", v_counter, mm68k.cycleCount/MCYCLES_PER_LINE, mm68k.cycleCount, mm68k.cycleCount%MCYCLES_PER_LINE, addr, data, m68k_get_reg (NULL, M68K_REG_PC));
#endif
      /* Pointer to CRAM 9-bit word */
      uint16 *p = &cram.getS(addr & 0x7E);

      /* Pack 16-bit bus data (BBB0GGG0RRR0) to 9-bit CRAM data (BBBGGGRRR) */
      data = ((data & 0xE00) >> 3) | ((data & 0x0E0) >> 2) | ((data & 0x00E) >> 1);

      /* Check if CRAM data is being modified */
      if (data != *p)
      {
        /* Write CRAM data */
        *p = data;

        /* CRAM index (64 words) */
        int index = (addr >> 1) & 0x3F;

        /* Color entry 0 of each palette is never displayed (transparent pixel) */
        if (index & 0x0F)
        {
          /* Update color palette */
          color_update_m5(index, data);
        }

        /* Update backdrop color */
        if (index == border)
        {
          color_update_m5(0x00, data);
        }

        /* CRAM modified during HBLANK (Striker, Zero the Kamikaze, etc) */
        if ((v_counter < bitmap.viewport.h) && (reg[1]& 0x40) && (mm68k.cycleCount <= int(mcycles_vdp + 860)))
        {
        	//logMsg("CRAM modified during HBLANK");
          /* Remap current line */
          remap_line(v_counter, framebufferPixmap());
        }
      }
      break;
    }

    case 0x05:  /* VSRAM */
    {
#ifdef LOGVDP
      error("[%d(%d)][%d(%d)] VSRAM 0x%x write -> 0x%x (%x)\n", v_counter, mm68k.cycleCount/MCYCLES_PER_LINE, mm68k.cycleCount, mm68k.cycleCount%MCYCLES_PER_LINE, addr, data, m68k_get_reg (NULL, M68K_REG_PC));
#endif
      vsram.getS(addr & 0x7E) = data;

      /* 2-cell Vscroll mode */
      if (reg[11] & 0x04)
      {
        /* VSRAM writes during HBLANK (Adventures of Batman & Robin) */
        if ((v_counter < bitmap.viewport.h) && (reg[1]& 0x40) && (mm68k.cycleCount <= int(mcycles_vdp + 860)))
        {
        	//logMsg("VSRAM modified during HBLANK");
          /* Remap current line */
          render_line(v_counter, framebufferPixmap());
        }
      }
      break;
    }

    default:
    {
    	logMsg("[%d(%d)][%d(%d)] Unknown (0x%X) 0x%x write -> 0x%x (%x)", v_counter, mm68k.cycleCount/MCYCLES_PER_LINE, mm68k.cycleCount, mm68k.cycleCount%MCYCLES_PER_LINE, code, addr, data, m68k_get_reg (mm68k, M68K_REG_PC));
      break;
    }
  }

  /* Increment address register (TODO: see how address is incremented in Mode 4) */
  addr += reg[15];
}


/*--------------------------------------------------------------------------*/
/* 68k data port access functions (Genesis mode)                            */
/*--------------------------------------------------------------------------*/

static void vdp_68k_data_w_m4(unsigned int data)
{
  /* Clear pending flag */
  pending = 0;

  /* Restricted VDP writes during active display */
  if (!(status & 8) && (reg[1] & 0x40))
  {
    /* Update VDP FIFO */
    vdp_fifo_update(mm68k.cycleCount);

    /* Clear FIFO empty flag */
    status &= 0xFDFF;

    /* 4 words can be stored */
    if (fifo_write_cnt < 4)
    {
      /* Increment FIFO counter */
      fifo_write_cnt++;

      /* Set FIFO full flag if 4 words are stored */
      status |= ((fifo_write_cnt & 4) << 6);
    }
    else
    {
      /* CPU is locked until last FIFO entry has been processed (Chaos Engine, Soldiers of Fortune, Double Clutch) */
      fifo_lastwrite += fifo_latency;
      mm68k.cycleCount = fifo_lastwrite;
    }
  }

  /* Check destination code */
  if (code & 0x02)
  {
    /* CRAM index (32 words) */
    int index = addr & 0x1F;

    /* Pointer to CRAM 9-bit word */
    uint16 *p = &cram.getS(index << 1);

    /* Pack 16-bit data (xxx000BBGGRR) to 9-bit CRAM data (xxxBBGGRR) */
    data = ((data & 0xE00) >> 3) | (data & 0x3F);

    /* Check if CRAM data is being modified */
    if (data != *p)
    {
      /* Write CRAM data */
      *p = data;

      /* Update color palette */
      color_update_m4(index, data);

      /* Update backdrop color */
      if (index == (0x10 | (border & 0x0F)))
      {
        color_update_m4(0x40, data);
      }
    }
  }
  else
  {
    /* Byte-swap data if A0 is set */
    if (addr & 1)
    {
      data = ((data >> 8) | (data << 8)) & 0xFFFF;
    }

    /* VRAM address (interleaved format) */
    int index = ((addr << 1) & 0x3FC) | ((addr & 0x200) >> 8) | (addr & 0x3C00);

    /* Pointer to VRAM */
    uint16 *p = (uint16 *)&vram.b[index];

    /* Only write unique data to VRAM */
    if (data != *p)
    {
      /* Write data to VRAM */
      *p = data;

      /* Update the pattern cache */
      int name;
      MARK_BG_DIRTY (index);
    }
  }

  /* Increment address register */
  addr += (reg[15] + 1);
}

static void vdp_68k_data_w_m5(unsigned int data)
{
  /* Clear pending flag */
  pending = 0;

  /* Restricted VDP writes during active display */
  if (!(status & 8) && (reg[1] & 0x40))
  {
    /* Update VDP FIFO */
    vdp_fifo_update(mm68k.cycleCount);

    /* Clear FIFO empty flag */
    status &= 0xFDFF;

    /* 4 words can be stored */
    if (fifo_write_cnt < 4)
    {
      /* Increment FIFO counter */
      fifo_write_cnt++;

      /* Set FIFO full flag if 4 words are stored */
      status |= ((fifo_write_cnt & 4) << 6);
    }
    else
    {
      /* CPU is locked until last FIFO entry has been processed (Chaos Engine, Soldiers of Fortune, Double Clutch) */
      fifo_lastwrite += fifo_latency;
      mm68k.cycleCount = fifo_lastwrite;
    }
  }

  /* Write data */
  vdp_bus_w(data);

  /* DMA Fill */
  if (dmafill & 0x100)
  {
    /* Fill data (DMA fill flag is cleared) */
    dmafill = data >> 8;

    /* DMA length */
    dma_length = (reg[20] << 8) | reg[19];

    /* Zero DMA length */
    if (!dma_length)
    {
      dma_length = 0x10000;
    }

    /* Perform DMA Fill*/
    dma_type = 2;
    vdp_dma_update(mm68k.cycleCount);
  }
}

static unsigned int vdp_68k_data_r_m4(void)
{
  /* Clear pending flag */
  pending = 0;

  /* VRAM address (interleaved format) */
  int index = ((addr << 1) & 0x3FC) | ((addr & 0x200) >> 8) | (addr & 0x3C00);

  /* Increment address register */
  addr += (reg[15] + 1);

  /* Read VRAM data */
  return vram.getS(index);
}

static unsigned int vdp_68k_data_r_m5(void)
{
  uint16 data = 0;

  /* Clear pending flag */
  pending = 0;

  switch (code & 0x0F)
  {
    case 0x00: /* VRAM */
    {
      /* Read data */
      data = vram.getS(addr & 0xFFFE);

#ifdef LOGVDP
      error("[%d(%d)][%d(%d)] VRAM 0x%x read -> 0x%x (%x)\n", v_counter, mm68k.cycleCount/MCYCLES_PER_LINE, mm68k.cycleCount, mm68k.cycleCount%MCYCLES_PER_LINE, addr, data, m68k_get_reg (NULL, M68K_REG_PC));
#endif
      break;
    }

    case 0x04: /* VSRAM */
    {
      /* Read data */
      data = vsram.getS(addr & 0x7E);

#ifdef LOGVDP
      error("[%d(%d)][%d(%d)] VSRAM 0x%x read -> 0x%x (%x)\n", v_counter, mm68k.cycleCount/MCYCLES_PER_LINE, mm68k.cycleCount, mm68k.cycleCount%MCYCLES_PER_LINE, addr, data, m68k_get_reg (NULL, M68K_REG_PC));
#endif
      break;
    }

    case 0x08: /* CRAM */
    {
      /* Read data */
      data = cram.getS(addr & 0x7E);

      /* Unpack 9-bit CRAM data (BBBGGGRRR) to 16-bit bus data (BBB0GGG0RRR0) */
      data = ((data & 0x1C0) << 3) | ((data & 0x038) << 2) | ((data & 0x007) << 1);

#ifdef LOGVDP
      error("[%d(%d)][%d(%d)] CRAM 0x%x read -> 0x%x (%x)\n", v_counter, mm68k.cycleCount/MCYCLES_PER_LINE, mm68k.cycleCount, mm68k.cycleCount%MCYCLES_PER_LINE, addr, data, m68k_get_reg (NULL, M68K_REG_PC));
#endif
      break;
    }

    default:
    {
      /* Invalid code value */
    	logMsg("[%d(%d)][%d(%d)] Invalid (%d) 0x%x read (%x)", v_counter, mm68k.cycleCount/MCYCLES_PER_LINE, mm68k.cycleCount, mm68k.cycleCount%MCYCLES_PER_LINE, code, addr, m68k_get_reg (mm68k, M68K_REG_PC));
      break;
    }
  }

  /* Increment address register */
  addr += reg[15];

  /* Return data */
  return data;
}


/*--------------------------------------------------------------------------*/
/* Z80 data port access functions (MS compatibilty mode)                    */
/*--------------------------------------------------------------------------*/

static void vdp_z80_data_w_m4(unsigned int data)
{
  /* Clear pending flag */
  pending = 0;

  /* Check destination code */
  if (code & 0x02)
  {
    /* CRAM index (32 words) */
    int index = addr & 0x1F;

    /* Pointer to CRAM word */
    uint16 *p = &cram.getS(index << 1);

    /* Check if CRAM data is being modified */
    if (data != *p)
    {
      /* Write CRAM data */
      *p = data;

      /* Update color palette */
      color_update_m4(index, data);

      /* Update backdrop color */
      if (index == (0x10 | (border & 0x0F)))
      {
        color_update_m4(0x40, data);
      }
    }
  }
  else
  {
    /* VRAM address */
    int index = addr & 0x3FFF;

    /* Only write unique data to VRAM */
    if (data != vram.b[index])
    {
      /* Write data */
      vram.b[index] = data;

      /* Update pattern cache */
      int name;
      MARK_BG_DIRTY(index);
    }
  }

  /* Increment address register  */
  addr += (reg[15] + 1);
}

static void vdp_z80_data_w_m5(unsigned int data)
{
  /* Clear pending flag */
  pending = 0;

  /* Check destination code */
  switch (code & 0x0F)
  {
    case 0x01:  /* VRAM */
    {
      /* VRAM address (write low byte to even address & high byte to odd address) */
      int index = addr ^ 1;

      /* Intercept writes to Sprite Attribute Table */
      if ((index & sat_base_mask) == satb)
      {
        /* Update internal SAT */
        WRITE_BYTE(sat.b, index & sat_addr_mask, data);
      }

      /* Only write unique data to VRAM */
      if (data != READ_BYTE(vram.b, index))
      {
        /* Write data */
        WRITE_BYTE(vram.b, index, data);

        /* Update pattern cache */
        int name;
        MARK_BG_DIRTY (index);
      }
      break;
    }

    case 0x03:  /* CRAM */
    {
      /* Pointer to CRAM word */
      uint16 *p = &cram.getS(addr & 0x7E);

      /* Pack 8-bit value into 9-bit CRAM data */
      if (addr & 1)
      {
        /* Write high byte (0000BBB0 -> BBBxxxxxx) */
        data = (*p & 0x3F) | ((data & 0x0E) << 5);
      }
      else
      {
        /* Write low byte (GGG0RRR0 -> xxxGGGRRR) */
        data = (*p & 0x1C0) | ((data & 0x0E) >> 1)| ((data & 0xE0) >> 2);
      }

      /* Check if CRAM data is being modified */
      if (data != *p)
      {
        /* Write CRAM data */
        *p = data;

        /* CRAM index (64 words) */
        int index = (addr >> 1) & 0x3F;

        /* Color entry 0 of each palette is never displayed (transparent pixel) */
        if (index & 0x0F)
        {
          /* Update color palette */
          color_update_m5(index, data);
        }

        /* Update backdrop color */
        if (index == border)
        {
          color_update_m5(0x00, data);
        }
      }
      break;
    }

    case 0x05: /* VSRAM */
    {
      /* Write low byte to even address & high byte to odd address */
      WRITE_BYTE(vsram.b, (addr & 0x7F) ^ 1, data);
      break;
    }
  }

  /* Increment address register  */
  addr += reg[15];

  /* DMA Fill */
  if (dmafill & 0x100)
  {
    /* Fill data (DMA fill flag is cleared) */
    dmafill = data;

    /* DMA length */
    dma_length = (reg[20] << 8) | reg[19];

    /* Zero DMA length */
    if (!dma_length)
    {
      dma_length = 0x10000;
    }

    /* Perform DMA Fill */
    dma_type = 2;
    vdp_dma_update(Z80.cycleCount);
  }
}

static unsigned int vdp_z80_data_r_m4(void)
{
  /* Clear pending flag */
  pending = 0;

  /* Read buffer */
  unsigned int data = fifo[0];

  /* Process next read */
  fifo[0] = vram.b[addr & 0x3FFF];

  /* Increment address register (register #15 can only be set in Mode 5) */
  addr += (reg[15] + 1);

  /* Return data */
  return data;
}

static unsigned int vdp_z80_data_r_m5(void)
{
  unsigned int data = 0;

  /* Clear pending flag */
  pending = 0;

  switch (code & 0x0F)
  {
    case 0x00: /* VRAM */
    {
      /* Return low byte from even address & high byte from odd address */
      data = READ_BYTE(vram.b, addr ^ 1);
      break;
    }

    case 0x04: /* VSRAM */
    {
      /* Return low byte from even address & high byte from odd address */
      data = READ_BYTE(vsram.b, (addr & 0x7F) ^ 1);
      break;
    }

    case 0x08: /* CRAM */
    {
      /* Read CRAM data */
      data = cram.getS(addr & 0x7E);

      /* Unpack 9-bit CRAM data (BBBGGGRRR) to 16-bit data (BBB0GGG0RRR0) */
      data = ((data & 0x1C0) << 3) | ((data & 0x038) << 2) | ((data & 0x007) << 1);

      /* Return low byte from even address & high byte from odd address */
      if (addr & 1)
      {
        data = data >> 8;
      }

      data &= 0xFF;
      break;
    }
  }

  /* Increment address register */
  addr += reg[15];

  /* Return data */
  return data;
}

/*-----------------------------------------------------------------------------*/
/* VDP specific data port access functions (Master System, Game Gear, SG-1000) */
/*-----------------------------------------------------------------------------*/

#if 0
static void vdp_z80_data_w_ms(unsigned int data)
{
  /* Clear pending flag */
  pending = 0;

  if (code < 3)
  {
    /* check if we are already on next line */
    int line = (lines_per_frame + (mcycles_z80 / MCYCLES_PER_LINE) - 1) % lines_per_frame;
    if ((line > v_counter) && (line < bitmap.viewport.h) && !(work_ram[0x1ffb] & cart.special))
    {
      v_counter = line;
      render_line(line);
    }

    /* VRAM address */
    int index = addr & 0x3FFF;

#ifdef LOGVDP
    error("[%d(%d)][%d(%d)] VRAM 0x%x write -> 0x%x (%x)\n", v_counter, mcycles_z80/MCYCLES_PER_LINE-1, mcycles_z80, mcycles_z80%MCYCLES_PER_LINE, index, data, Z80.pc.w.l);
#endif

    /* VRAM write */
    if(data != vram.b[index])
    {
      int name;
      vram.b[index] = data;
      MARK_BG_DIRTY(index);
    }
  }
  else
  {
#ifdef LOGVDP
      error("[%d(%d)][%d(%d)] CRAM 0x%x write -> 0x%x (%x)\n", v_counter, mcycles_z80/MCYCLES_PER_LINE-1, mcycles_z80, mcycles_z80%MCYCLES_PER_LINE, addr, data, Z80.pc.w.l);
#endif
    /* CRAM address */
    int index = addr & 0x1F;

    /* Pointer to CRAM word */
    uint16 *p = (uint16 *)&cram.b[index << 1];

    /* Check if CRAM data is being modified */
    if (data != *p)
    {
      /* Write CRAM data */
      *p = data;

      /* Update color palette */
      color_update_m4(index, data);

      /* Update backdrop color */
      if (index == (0x10 | (border & 0x0F)))
      {
        color_update_m4(0x40, data);
      }
    }
  }

  /* Update read buffer */
  fifo[0] = data;

  /* Update address register */
  addr++;
}

static void vdp_z80_data_w_gg(unsigned int data)
{
  /* Clear pending flag */
  pending = 0;

  if (code < 3)
  {
    /* check if we are already on next line*/
    int line = (lines_per_frame + (mcycles_z80 / MCYCLES_PER_LINE) - 1) % lines_per_frame;
    if ((line > v_counter) && (line < bitmap.viewport.h) && !(work_ram[0x1ffb] & cart.special))
    {
      v_counter = line;
      render_line(line);
    }

    /* VRAM address */
    int index = addr & 0x3FFF;

#ifdef LOGVDP
      error("[%d(%d)][%d(%d)] VRAM 0x%x write -> 0x%x (%x)\n", v_counter, mcycles_z80/MCYCLES_PER_LINE-1, mcycles_z80, mcycles_z80%MCYCLES_PER_LINE, index, data, Z80.pc.w.l);
#endif

    /* VRAM write */
    if(data != vram.b[index])
    {
      int name;
      vram.b[index] = data;
      MARK_BG_DIRTY(index);
    }
  }
  else
  {
    if(addr & 1)
    {
      /* 12-bit data word */
      data = (data << 8) | cached_write;

      /* Pointer to CRAM word */
      uint16 *p = (uint16 *)&cram.b[addr & 0x3E];

      /* Check if CRAM data is being modified */
      if (data != *p)
      {
        /* Write CRAM data */
        *p = data;

        /* Color index (0-31) */
        int index = (addr >> 1) & 0x1F;

        /* Update color palette */
        color_update_m4(index, data);

        /* Update backdrop color */
        if (index == (0x10 | (border & 0x0F)))
        {
          color_update_m4(0x40, data);
        }
      }
    }
    else
    {
      /* Latch LSB */
      cached_write = data;
    }
  }

  /* Update read buffer */
  fifo[0] = data;

  /* Update address register */
  addr++;
}

static void vdp_z80_data_w_sg(unsigned int data)
{
  /* Clear pending flag */
  pending = 0;

  /* VRAM address */
  int index = addr & 0x3FFF;

  /* 4K address decoding (cf. tms9918a.txt) */
  if (!(reg[1] & 0x80))
  {
    index = (index & 0x203F) | ((index >> 6) & 0x40) | ((index << 1) & 0x1F80);
  }

  /* VRAM write */
  vram.b[index] = data;

  /* Update address register */
  addr++;
}
#endif

/*--------------------------------------------------------------------------*/
/* DMA operations                                                           */
/*--------------------------------------------------------------------------*/

/* 68K bus to VRAM, VSRAM or CRAM */
static void vdp_dma_vbus(int length)
{
  unsigned int data;
  unsigned int source = (reg[23] << 17 | reg[22] << 9 | reg[21] << 1) & 0xFFFFFE;
  unsigned int base = source;

  //logMsg("VDP 68K bus DMA from 0x%X", source);
  /* DMA source */
  if ((source >> 17) == 0x50)
  {
    /* Z80 & I/O area ($A00000-$A1FFFF) */
    do
    {
      /* Return $FFFF only when the Z80 isn't hogging the Z-bus.
        (e.g. Z80 isn't reset and 68000 has the bus) */
      if (source <= 0xA0FFFF)
      {
        data = ((zstate ^ 3) ? *(uint16 *)(work_ram + (source & 0xFFFF)) : 0xFFFF);
      }

      /* The I/O chip and work RAM try to drive the data bus which results
          in both values being combined in random ways when read.
          We return the I/O chip values which seem to have precedence, */
      else if (source <= 0xA1001F)
      {
        data = io_68k_read((source >> 1) & 0x0F);
        data = (data << 8 | data);
      }

      /* All remaining locations access work RAM */
      else
      {
        data = *(uint16 *)(work_ram + (source & 0xFFFF));
      }

      /* Increment source address */
      source += 2;

      /* 128k DMA window (i.e reg #23 is not modified) */
      source = ((base & 0xFE0000) | (source & 0x1FFFF));

      /* Write data on internal bus */
      vdp_bus_w(data);
    }
    while (--length);
  }
  else
  {
		/*#ifndef NO_SCD
		if(sCD.isActive && (source>>16 >= 0x20) && (source>>16 <= 0x23))
		{
			//logMsg("DMA src %X", source);
			if(dmaStarting) //&& (code & 0x0F) == 1)
			{
				//logMsg("dma latency from 0x%X to 0x%X, length 0x%X", source, addr, length);
				addr += 2;
				length -= 1;
				if(length == 0)
				{
					logMsg("0 length DMA from Word RAM");
					//length = 1;
				  return;
				}
			}
		}
		dmaStarting = 0;
		#endif*/

    do
    {
      /* Read from mapped memory (ROM/RAM) */
			#ifndef NO_SCD
    	if(mm68k.memory_map[source>>16].read16)
    	{
    		logMsg("dma from 0x%X using read handler", source);
    		data = mm68k.memory_map[source>>16].read16(source);
    	}
    	else
			#endif
    		data = *(uint16 *)(mm68k.memory_map[source>>16].base + (source & 0xFFFF));

      /* Increment source address */
      source += 2;

      /* 128k DMA window (i.e reg #23 is not modified) */
      source = ((base & 0xFE0000) | (source & 0x1FFFF));

      /* Write data on internal bus */
      vdp_bus_w(data);
    }
    while (--length);
  }

  /* Update source address registers (reg #23 has not been modified) */
  reg[21] = (source >> 1) & 0xFF;
  reg[22] = (source >> 9) & 0xFF;
}

/*  VRAM Copy (TODO: check if CRAM or VSRAM copy is possible) */
static void vdp_dma_copy(int length)
{
  int name;
  unsigned int temp;
  unsigned int source = (reg[22] << 8) | reg[21];

  //logMsg("VDP VRAM DMA from 0x%X", source);
  do
  {
    /* Read byte from source address */
    temp = READ_BYTE(vram.b, source);

    /* Intercept writes to Sprite Attribute Table */
    if ((addr & sat_base_mask) == satb)
    {
      /* Update internal SAT */
      WRITE_BYTE(sat.b, addr & sat_addr_mask, temp);
    }

    /* Write byte to VRAM address */
    WRITE_BYTE(vram.b, addr, temp);

    /* Update pattern cache */
    MARK_BG_DIRTY(addr);

    /* Increment source address */
    source = (source + 1) & 0xFFFF;

    /* Increment VRAM address */
    addr += reg[15];
  }
  while (--length);

  /* Update source address registers */
  reg[21] = source & 0xFF;
  reg[22] = (source >> 8) & 0xFF;
}

/* VRAM Fill (TODO: check if CRAM or VSRAM fill is possible) */
static void vdp_dma_fill(unsigned int data, int length)
{
  int name;

  //logMsg("VDP DMA fill from addr 0x%X with 0x%X len %d", addr, data, length);
  do
  {
    /* Intercept writes to Sprite Attribute Table */
    if ((addr & sat_base_mask) == satb)
    {
      /* Update internal SAT */
      WRITE_BYTE(sat.b, (addr & sat_addr_mask) ^ 1, data);
    }

    /* Write byte to adjacent VRAM address */
    WRITE_BYTE(vram.b, addr ^ 1, data);

    /* Update pattern cache */
    MARK_BG_DIRTY (addr);

    /* Increment VRAM address */
    addr += reg[15];
  }
  while (--length);
}

static IG::MutablePixmapView framebufferPixmap(IG::PixelFormat fmt)
{
	return {{{bitmap.viewport.w, bitmap.viewport.h}, fmt}, frameBufferData};
}

IG::MutablePixmapView framebufferPixmap()
{
	return framebufferPixmap(fbPixelFormat);
}

IG::MutablePixmapView framebufferRenderFormatPixmap()
{
	return framebufferPixmap(framebufferRenderFormat());
}
