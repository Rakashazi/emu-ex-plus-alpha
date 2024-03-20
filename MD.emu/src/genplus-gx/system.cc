/***************************************************************************************
 *  Genesis Plus
 *  Virtual System emulation
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

#include <imagine/util/algorithm.h>
#include <emuframework/EmuApp.hh>
#include "shared.h"
#include "vdp_render.h"
#include "Fir_Resampler.h"
#include "eq.h"
#include "assert.h"

#ifndef NO_SCD
#include <scd/scd.h>
#include <scd/pcm.h>
#endif

/* Global variables */
//t_bitmap bitmap;
t_snd snd{44100, 1};
uint32 mcycles_vdp;
//uint32 Z80.cycleCount;
//uint32 mcycles_68k;
uint8 system_hw;
void (*system_frame)(EmuEx::EmuSystemTaskContext, EmuEx::EmuVideo *);
int (*audioUpdateFunc)(int16 *sb);
Z80CPU<z80Desc> Z80;

template <bool hasSegaCD = 0>
static void system_frame_md(EmuEx::EmuSystemTaskContext, EmuEx::EmuVideo *);
static void system_frame_sms(EmuEx::EmuSystemTaskContext, EmuEx::EmuVideo *);
static int pause_b;
static EQSTATE eq;
static int32 llp,rrp;

/****************************************************************
 * Audio subsystem
 ****************************************************************/

int audio_init (int samplerate, float framerate)
{
  /* Shutdown first */
  audio_shutdown();

  /* Clear the sound data context */
  memset(&snd, 0, sizeof (snd));

  /* Default settings */
  snd.sample_rate = samplerate;
  snd.frame_rate  = framerate;
  snd.cddaRatio = 44100./snd.sample_rate;

  /* Calculate the sound buffer size (for one frame) */
  snd.buffer_size = (int)(samplerate / framerate) + 32;

  /* SN76489 stream buffers */
  snd.psg.buffer = (int16 *) malloc(snd.buffer_size * sizeof(int16));
  if (!snd.psg.buffer) return (-1);

  /* YM2612 stream buffers */
  snd.fm.buffer = (FMSampleType*) malloc(snd.buffer_size * sizeof(FMSampleType) * 2);
  if (!snd.fm.buffer) return (-1);

	#ifndef NO_SCD
		scd_pcm_setRate(samplerate);
	#endif

  /* Resampling buffer */
  if (config_hq_fm && !Fir_Resampler_initialize(4096)) return (-1);

  /* Set audio enable flag */
  snd.enabled = 1;

  /* Reset audio */
  audio_reset();

  return (0);
}

void audio_reset(void)
{
  /* Low-Pass filter */
  llp = 0;
  rrp = 0;

  /* 3 band EQ */
  audio_set_equalizer();

  /* Resampling buffer */
  Fir_Resampler_clear();

  /* Audio buffers */
  snd.psg.pos = snd.psg.buffer;
  snd.fm.pos  = snd.fm.buffer;
  if (snd.psg.buffer) memset (snd.psg.buffer, 0, snd.buffer_size * sizeof(int16));
  if (snd.fm.buffer) memset (snd.fm.buffer, 0, snd.buffer_size * sizeof(FMSampleType) * 2);
}

void audio_set_equalizer(void)
{
  //init_3band_state(&eq,config_low_freq,config_high_freq,snd.sample_rate);
  eq.lg = (double)(config_lg) / 100.0;
  eq.mg = (double)(config_mg) / 100.0;
  eq.hg = (double)(config_hg) / 100.0;
}

void audio_shutdown(void)
{
  /* Sound buffers */
  if (snd.fm.buffer) free(snd.fm.buffer);
  if (snd.psg.buffer) free(snd.psg.buffer);

  /* Resampling buffer */
  Fir_Resampler_shutdown();
}

template <bool hasSegaCD>
int audioUpdateAll(int16 *sb)
{
  int32 i, l, r;
  int32 ll = llp;
  int32 rr = rrp;

  int psg_preamp  = config_psg_preamp;
  int fm_preamp   = config_fm_preamp;
  int filter      = config_filter;
  uint32 factora  = (config_lp_range << 16) / 100;
  uint32 factorb  = 0x10000 - factora;

  FMSampleType *fm       = snd.fm.buffer;
  int16 *psg      = snd.psg.buffer;

  /* get number of available samples */
  int size = sound_update(mcycles_vdp);

  /* return an aligned number of samples */
  size &= ~7;

	#ifndef NO_SCD
	int16 cdPCMBuff[size*2];
	int16 *cdPCM = cdPCMBuff;
	bool doPCM = hasSegaCD && (sCD.pcm.control & 0x80) && sCD.pcm.enabled;
	if(doPCM)
	{
		scd_pcm_update(cdPCMBuff, size, 1);
	}
	auto cddaRatio = snd.cddaRatio;
	unsigned cddaFrames = round((float)size*cddaRatio);
	int16 cddaBuff[cddaFrames*2];
	int16 *cdda = cddaBuff;
	int16 cddaRemsampledBuff[size*2];
	extern int readCDDA(void *dest, unsigned size);
	bool doCDDA = hasSegaCD && readCDDA(cddaBuff, cddaFrames);
	if(doCDDA && snd.sample_rate != 44100)
	{
		auto cddaPtr = (int32*)cddaBuff;
		auto cddaResampledPtr = (int32*)cddaRemsampledBuff;
		for(auto i : IG::iotaCount(size))
		{
			unsigned samplePos = round(i * cddaRatio);
			if(samplePos > cddaFrames)
			{
				logMsg("resample pos %u too high", samplePos);
				samplePos = cddaFrames-1;
			}
			cddaResampledPtr[i] = cddaPtr[samplePos];
		}
		cdda = cddaRemsampledBuff;
	}
	#endif

  if (config_hq_fm)
  {
    /* resample into FM output buffer */
    Fir_Resampler_read(fm, size);

#ifdef LOGSOUND
    error("%d FM samples remaining\n",Fir_Resampler_written() >> 1);
#endif
  }
  else
  {  
    /* adjust remaining samples in FM output buffer*/
    snd.fm.pos -= (size * 2);

#ifdef LOGSOUND
    error("%d FM samples remaining\n",(snd.fm.pos - snd.fm.buffer)>>1);
#endif
  }

  /* adjust remaining samples in PSG output buffer*/
  snd.psg.pos -= size;

#ifdef LOGSOUND
  error("%d PSG samples remaining\n",snd.psg.pos - snd.psg.buffer);
#endif

  assert(size < snd.buffer_size);
  /* mix samples */
  for (i = 0; i < size; i ++)
  {
    /* PSG samples (mono) */
    l = r = (((*psg++) * psg_preamp) / 100);

    /* FM samples (stereo) */
    l += ((*fm++ * fm_preamp) / 100);
    r += ((*fm++ * fm_preamp) / 100);

		#ifndef NO_SCD
    if(doPCM)
		{
			l += *cdPCM++;
			r += *cdPCM++;
		}
    if(doCDDA)
    {
    	l += *cdda++;
    	r += *cdda++;
    }
		#endif

    /* filtering */
    if (filter & 1)
    {
      /* single-pole low-pass filter (6 dB/octave) */
      ll = (ll>>16)*factora + l*factorb;
      rr = (rr>>16)*factora + r*factorb;
      l = ll >> 16;
      r = rr >> 16;
    }
    else if (filter & 2)
    {
      /* 3 Band EQ */
      l = do_3band(&eq,l);
      r = do_3band(&eq,r);
    }

    /* clipping (16-bit samples) */
    if(config_clipSound)
    {
		if (l > 32767) l = 32767;
		else if (l < -32768) l = -32768;
		if (r > 32767) r = 32767;
		else if (r < -32768) r = -32768;
    }

    /* update sound buffer */
#ifndef NGC
    *sb++ = l;
    *sb++ = r;
#else
    *sb++ = r;
    *sb++ = l;
#endif
  }

  /* save filtered samples for next frame */
  llp = ll;
  rrp = rr;

  /* keep remaining samples for next frame */
  memcpy(snd.fm.buffer, fm, (snd.fm.pos - snd.fm.buffer) * sizeof(FMSampleType));
  memcpy(snd.psg.buffer, psg, (snd.psg.pos - snd.psg.buffer) * sizeof(int16));

#ifdef LOGSOUND
  error("%d samples returned\n\n",size);
#endif

  return size;
}

template int audioUpdateAll<0>(int16 *sb);
template int audioUpdateAll<1>(int16 *sb);

int audio_update(int16 *sb)
{
	return audioUpdateFunc(sb);
}

/****************************************************************
 * Virtual Genesis initialization
 ****************************************************************/
void system_init(void)
{
  gen_init();
  io_init();
  vdp_init();
  render_init();
  sound_init();
  system_frame =
  #ifndef NO_SYSTEM_PBC
	(system_hw == SYSTEM_PBC) ? system_frame_sms :
  #endif
	#ifndef NO_SCD
	sCD.isActive ? system_frame_md<1> :
	#endif
	system_frame_md<0>;
  audioUpdateFunc =
	#ifndef NO_SCD
	sCD.isActive ? audioUpdateAll<1> :
	#endif
	audioUpdateAll<0>;
}

/****************************************************************
 * Virtual System emulation
 ****************************************************************/
void system_reset(void)
{
  gen_reset(1);
  io_reset();
  render_reset();
  vdp_reset();
  sound_reset();
  audio_reset();
}

void system_shutdown (void)
{
  gen_shutdown();
  SN76489_Shutdown();
}

template <bool hasSegaCD>
static void runM68k(unsigned cycles)
{
	m68k_run(mm68k, cycles);
	#ifndef NO_SCD
		if(hasSegaCD)
		{
			scd_runSubCpu(cycles);
		}
	#endif
}

template void system_frame_md<0>(EmuEx::EmuSystemTaskContext, EmuEx::EmuVideo *);
template void system_frame_md<1>(EmuEx::EmuSystemTaskContext, EmuEx::EmuVideo *);

template <bool hasSegaCD>
static void system_frame_md(EmuEx::EmuSystemTaskContext taskCtx, EmuEx::EmuVideo *emuVideo)
{
	int do_skip = !emuVideo;

	//logMsg("start frame");
  /* line counter */
  int line = 0;

  /* Z80 interrupt flag */
  int zirq = 1;

  /* reload H Counter */
  int h_counter = reg[10];

  /* reset line master cycle count */
  mcycles_vdp = 0;

  /* reload V Counter */
  v_counter = lines_per_frame - 1;

  /* reset VDP FIFO */
  fifo_write_cnt = 0;
  fifo_lastwrite = 0;

  /* update 6-Buttons & Lightguns */
  input_refresh();
	#ifndef NO_SCD
	if(hasSegaCD) scd_checkDma();
	#endif

  /* display changed during VBLANK */
  if (bitmap.viewport.changed & 2)
  {
    bitmap.viewport.changed &= ~2;

    /* interlaced mode */
    int old_interlaced  = interlaced;
    interlaced = (reg[12] & 0x02) >> 1;
    if (old_interlaced != interlaced)
    {
      im2_flag = ((reg[12] & 0x06) == 0x06);
      odd_frame = 1;
      bitmap.viewport.changed = 5;

      /* update rendering mode */
      if (reg[1] & 0x04)
      {
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
      }
    }

    /* active screen height */
    if (reg[1] & 0x04)
    {
      bitmap.viewport.h = 224 + ((reg[1] & 0x08) << 1);
      bitmap.viewport.y = (config_overscan & 1) * ((240 + 48*vdp_pal - bitmap.viewport.h) >> 1);
    }
    else
    {
      bitmap.viewport.h = 192;
      bitmap.viewport.y = (config_overscan & 1) * 24 * (vdp_pal + 1);
    }

    /* active screen width */
    bitmap.viewport.w = 256 + ((reg[12] & 0x01) << 6);
  }

  auto pixmap = framebufferRenderFormatPixmap();

  /* clear VBLANK, DMA, FIFO FULL & field flags */
  status &= 0xFEE5;

  /* set FIFO EMPTY flag */
  status |= 0x0200;

  /* even/odd field flag (interlaced modes only) */
  odd_frame ^= 1;
  if (interlaced)
  {
    status |= (odd_frame << 4);
  }

  /* update VDP DMA */
  if (dma_length)
  {
    vdp_dma_update(0);
  }

  /* render last line of overscan */
  if (bitmap.viewport.y > 0)
  {
    blank_line(v_counter, -bitmap.viewport.x, bitmap.viewport.w + 2*bitmap.viewport.x);
  }

  /* parse first line of sprites */
  if (reg[1] & 0x40)
  {
    parse_satb(-1);
  }

  /* run 68k & Z80 */
  //m68k_run(mm68k, MCYCLES_PER_LINE);
  runM68k<hasSegaCD>(MCYCLES_PER_LINE);
  if (zstate == 1)
  {
    z80_run(MCYCLES_PER_LINE);
  }
  else
  {
    Z80.cycleCount = MCYCLES_PER_LINE;
  }

	#ifndef NO_SCD
  if(hasSegaCD) scd_update();
	#endif

  /* run SVP chip */
  #ifndef NO_SVP
  if (!hasSegaCD && svp)
  {
    ssp1601_run(SVP_cycles);
  }
  #endif

  /* update line cycle count */
  mcycles_vdp += MCYCLES_PER_LINE;

  /* Active Display */
  do
  {
    /* update V Counter */
    v_counter = line;

    /* update 6-Buttons & Lightguns */
    input_refresh();
		#ifndef NO_SCD
		if(hasSegaCD) scd_checkDma();
		#endif

    /* H Interrupt */
    if(--h_counter < 0)
    {
      /* reload H Counter */
      h_counter = reg[10];
      
      /* interrupt level 4 */
      hint_pending = 0x10;
      if (reg[0] & 0x10)
      {
    	  //mm68k.irq_state |= 0x14;
    	  mm68k.updateIRQ(4);
      }
    }

    /* update VDP DMA */
    if (dma_length)
    {
      vdp_dma_update(mcycles_vdp);
    }

    /* render scanline */
    if (!do_skip)
    {
      render_line(line, pixmap);
    }

    /* run 68k & Z80 */
    //m68k_run(mm68k, mcycles_vdp + MCYCLES_PER_LINE);
    runM68k<hasSegaCD>(mcycles_vdp + MCYCLES_PER_LINE);
    if (zstate == 1)
    {
      z80_run(mcycles_vdp + MCYCLES_PER_LINE);
    }
    else
    {
      Z80.cycleCount = mcycles_vdp + MCYCLES_PER_LINE;
    }

		#ifndef NO_SCD
		if(hasSegaCD) scd_update();
		#endif

    /* run SVP chip */
	#ifndef NO_SVP
    if (!hasSegaCD && svp)
    {
      ssp1601_run(SVP_cycles);
    }
	#endif

    /* update line cycle count */
    mcycles_vdp += MCYCLES_PER_LINE;
  }
  while (++line < bitmap.viewport.h);

  if(emuVideo)
  {
  	emuVideo->startFrameWithAltFormat(taskCtx, pixmap);
  }

  /* end of active display */
  v_counter = line;

  /* set VBLANK flag */
  status |= 0x08;

  /* overscan area */
  int start = lines_per_frame - bitmap.viewport.y;
  int end   = bitmap.viewport.h + bitmap.viewport.y;

  /* check viewport changes */
  if ((bitmap.viewport.w != bitmap.viewport.ow) || (bitmap.viewport.h != bitmap.viewport.oh))
  {
    bitmap.viewport.ow = bitmap.viewport.w;
    bitmap.viewport.oh = bitmap.viewport.h;
    bitmap.viewport.changed |= 1;
  }

  /* update 6-Buttons & Lightguns */
  input_refresh();
	#ifndef NO_SCD
	if(hasSegaCD) scd_checkDma();
	#endif

  /* H Interrupt */
  if(--h_counter < 0)
  {
    /* reload H Counter */
    h_counter = reg[10];

    /* interrupt level 4 */
    hint_pending = 0x10;
    if (reg[0] & 0x10)
    {
    	//mm68k.irq_state |= 0x14;
    	mm68k.updateIRQ(4);
    }
  }

  /* update VDP DMA */
  if (dma_length)
  {
    vdp_dma_update(mcycles_vdp);
  }

  /* render overscan */
  if (line < end)
  {
    blank_line(line, -bitmap.viewport.x, bitmap.viewport.w + 2*bitmap.viewport.x);
  }

  /* update inputs before VINT (Warriors of Eternal Sun) */
  osd_input_Update();

  /* delay between VINT flag & V Interrupt (Ex-Mutants, Tyrant) */
  //m68k_run(mm68k, mcycles_vdp + 588);
  runM68k<hasSegaCD>(mcycles_vdp + 588);
  status |= 0x80;

  /* delay between VBLANK flag & V Interrupt (Dracula, OutRunners, VR Troopers) */
  //m68k_run(mm68k, mcycles_vdp + 788);
  runM68k<hasSegaCD>(mcycles_vdp + 788);
  if (zstate == 1)
  {
    z80_run(mcycles_vdp + 788);
  }
  else
  {
    Z80.cycleCount = mcycles_vdp + 788;
  }

  /* V Interrupt */
  vint_pending = 0x20;
  if (reg[1] & 0x20)
  {
	  //mm68k.irq_state = 0x16;
	  mm68k.setIRQ(6);
  }

  /* assert Z80 interrupt */
  Z80.irq_state = ASSERT_LINE;

  /* run 68k & Z80 until end of line */
  //m68k_run(mm68k, mcycles_vdp + MCYCLES_PER_LINE);
  runM68k<hasSegaCD>(mcycles_vdp + MCYCLES_PER_LINE);
  if (zstate == 1)
  {
    z80_run(mcycles_vdp + MCYCLES_PER_LINE);
  }
  else
  {
    Z80.cycleCount = mcycles_vdp + MCYCLES_PER_LINE;
  }

	#ifndef NO_SCD
	if(hasSegaCD) scd_update();
	#endif

  /* run SVP chip */
  #ifndef NO_SVP
  if (!hasSegaCD && svp)
  {
    ssp1601_run(SVP_cycles);
  }
  #endif

  /* update line cycle count */
  mcycles_vdp += MCYCLES_PER_LINE;

  /* increment line count */
  line++;

  /* Vertical Blanking */
  do
  {
    /* update V Counter */
    v_counter = line;

    /* update 6-Buttons & Lightguns */
    input_refresh();
		#ifndef NO_SCD
		if(hasSegaCD) scd_checkDma();
		#endif

    /* render overscan */
    if ((line < end) || (line >= start))
    {
      blank_line(line, -bitmap.viewport.x, bitmap.viewport.w + 2*bitmap.viewport.x);
    }

    if (zirq)
    {
      /* Z80 interrupt is asserted exactly for one line */
      //m68k_run(mm68k, mcycles_vdp + 788);
    	runM68k<hasSegaCD>(mcycles_vdp + 788);
      if (zstate == 1)
      {
        z80_run(mcycles_vdp + 788);
      }
      else
      {
        Z80.cycleCount = mcycles_vdp + 788;
      }

      /* clear Z80 interrupt */
      Z80.irq_state = CLEAR_LINE;
      zirq = 0;
    }

    /* run 68k & Z80 */
    runM68k<hasSegaCD>(mcycles_vdp + MCYCLES_PER_LINE);
    if (zstate == 1)
    {
      z80_run(mcycles_vdp + MCYCLES_PER_LINE);
    }
    else
    {
      Z80.cycleCount = mcycles_vdp + MCYCLES_PER_LINE;
    }

		#ifndef NO_SCD
		if(hasSegaCD) scd_update();
		#endif

    /* run SVP chip */
	#ifndef NO_SVP
    if (!hasSegaCD && svp)
    {
      ssp1601_run(SVP_cycles);
    }
	#endif

    /* update line cycle count */
    mcycles_vdp += MCYCLES_PER_LINE;
  }
  while (++line < (lines_per_frame - 1));

  /* adjust 68k & Z80 cycle count for next frame */
  mm68k.cycleCount -= mcycles_vdp;
  Z80.cycleCount -= mcycles_vdp;
	#ifndef NO_SCD
	if(hasSegaCD) sCD.cpu.cycleCount -= mcycles_vdp;
	#endif
  //logMsg("end frame");
}


static void system_frame_sms(EmuEx::EmuSystemTaskContext taskCtx, EmuEx::EmuVideo *emuVideo)
{
	int do_skip = !emuVideo;

  /* line counter */
  int line = 0;

  /* reload H Counter */
  int h_counter = reg[10];

  /* reset line master cycle count */
  mcycles_vdp = 0;

  /* reload V Counter */
  v_counter = lines_per_frame - 1;

  /* reset VDP FIFO */
  fifo_write_cnt = 0;
  fifo_lastwrite = 0;

  /* update 6-Buttons & Lightguns */
  input_refresh();

  /* display changed during VBLANK */
  if (bitmap.viewport.changed & 2)
  {
    bitmap.viewport.changed &= ~2;

    /* interlaced mode */
    int old_interlaced  = interlaced;
    interlaced = (reg[12] & 0x02) >> 1;
    if (old_interlaced != interlaced)
    {
      im2_flag = ((reg[12] & 0x06) == 0x06);
      odd_frame = 1;
      bitmap.viewport.changed = 5;

      /* update rendering mode */
      if (reg[1] & 0x04)
      {
        if (im2_flag)
        {
          render_bg = (reg[11] & 0x04) ? render_bg_m5_im2_vs : render_bg_m5_im2;
          render_obj = render_obj_m5_im2;

        }
        else
        {
          render_bg = (reg[11] & 0x04) ? render_bg_m5_vs : render_bg_m5;
          render_obj = render_obj_m5;
        }
      }
    }

    /* active screen height */
    if (reg[1] & 0x04)
    {
      bitmap.viewport.h = 224 + ((reg[1] & 0x08) << 1);
      bitmap.viewport.y = (config_overscan & 1) * ((240 + 48*vdp_pal - bitmap.viewport.h) >> 1);
    }
    else
    {
      bitmap.viewport.h = 192;
      bitmap.viewport.y = (config_overscan & 1) * 24 * (vdp_pal + 1);
    }

    /* active screen width */
    bitmap.viewport.w = 256 + ((reg[12] & 0x01) << 6);
  }

  auto pixmap = framebufferRenderFormatPixmap();

  /* Detect pause button input */
  if (input.pad[0] & INPUT_START)
  {
    /* NMI is edge-triggered */
    if (!pause_b)
    {
      pause_b = 1;
      z80_set_nmi_line(ASSERT_LINE);
      z80_set_nmi_line(CLEAR_LINE);
    }
  }
  else
  {
    pause_b = 0;
  }

  /* 3-D glasses faking: skip rendering of left lens frame */
  do_skip |= (work_ram[0x1ffb] & cart.special);

  /* clear VBLANK, DMA & field flags */
  status &= 0xE5;

  /* even/odd field flag (interlaced modes only) */
  odd_frame ^= 1;
  if (interlaced)
  {
    status |= (odd_frame << 4);
  }

  /* update VDP DMA */
  if (dma_length)
  {
    vdp_dma_update(0);
  }

  /* render last line of overscan */
  if (bitmap.viewport.y > 0)
  {
    blank_line(v_counter, -bitmap.viewport.x, bitmap.viewport.w + 2*bitmap.viewport.x);
  }

  /* parse first line of sprites */
  if (reg[1] & 0x40)
  {
    parse_satb(-1);
  }

  /* latch Horizontal Scroll register (if modified during VBLANK) */
  //hscroll = reg[0x08];

  /* run Z80 */
  z80_run(MCYCLES_PER_LINE);

  /* update line cycle count */
  mcycles_vdp += MCYCLES_PER_LINE;

  /* latch Vertical Scroll register */
  vscroll = reg[0x09];

  /* Active Display */
  do
  {
    /* update VDP DMA (Mega Drive VDP specific) */
    if (dma_length)
    {
      vdp_dma_update(mcycles_vdp);
    }

    /* make sure we didn't already render that line */
    if (v_counter != line)
    {
      /* update V Counter */
      v_counter = line;

      /* render scanline */
      if (!do_skip)
      {
        render_line(line, pixmap);
      }
    }

    /* update 6-Buttons & Lightguns */
    input_refresh();

    /* H Interrupt */
    if(--h_counter < 0)
    {
      /* reload H Counter */
      h_counter = reg[10];
      
      /* interrupt level 4 */
      hint_pending = 0x10;
      if (reg[0] & 0x10)
      {
        /* cycle-accurate HINT */
        /* IRQ line is latched between instructions, during instruction last cycle.       */
        /* This means that if Z80 cycle count is exactly a multiple of MCYCLES_PER_LINE,  */
        /* interrupt should be triggered AFTER the next instruction.                      */
        if ((Z80.cycleCount % MCYCLES_PER_LINE) == 0)
        {
          z80_run(Z80.cycleCount + 1);
        }

        Z80.irq_state = ASSERT_LINE;
      }
    }

    /* run Z80 */
    z80_run(mcycles_vdp + MCYCLES_PER_LINE);

    /* update line cycle count */
    mcycles_vdp += MCYCLES_PER_LINE;
  }
  while (++line < bitmap.viewport.h);

  if(emuVideo)
  {
  	emuVideo->startFrameWithAltFormat(taskCtx, pixmap);
  }

  /* end of active display */
  v_counter = line;

  /* set VBLANK flag */
  status |= 0x08;

  /* overscan area */
  int start = lines_per_frame - bitmap.viewport.y;
  int end   = bitmap.viewport.h + bitmap.viewport.y;

  /* check viewport changes */
  if ((bitmap.viewport.w != bitmap.viewport.ow) || (bitmap.viewport.h != bitmap.viewport.oh))
  {
    bitmap.viewport.ow = bitmap.viewport.w;
    bitmap.viewport.oh = bitmap.viewport.h;
    bitmap.viewport.changed |= 1;
  }

  /* update 6-Buttons & Lightguns */
  input_refresh();

  /* H Interrupt */
  if(--h_counter < 0)
  {
    /* reload H Counter */
    h_counter = reg[10];

    /* interrupt level 4 */
    hint_pending = 0x10;
    if (reg[0] & 0x10)
    {
      /* cycle-accurate HINT */
      if ((Z80.cycleCount % MCYCLES_PER_LINE) == 0)
      {
        z80_run(Z80.cycleCount + 1);
      }

      Z80.irq_state = ASSERT_LINE;
    }
  }

  /* update VDP DMA */
  if (dma_length)
  {
    vdp_dma_update(mcycles_vdp);
  }

  /* render overscan */
  if (line < end)
  {
    blank_line(line, -bitmap.viewport.x, bitmap.viewport.w + 2*bitmap.viewport.x);
  }

  /* update inputs before VINT (Warriors of Eternal Sun) */
  osd_input_Update();

  /* run Z80 until end of line */
  z80_run(mcycles_vdp + MCYCLES_PER_LINE);

  /* make sure VINT flag was not cleared by last instruction */
  if (v_counter == line)
  {
    /* Set VINT flag */
    status |= 0x80;

    /* V Interrupt */
    vint_pending = 0x20;
    if (reg[1] & 0x20)
    {
      Z80.irq_state = ASSERT_LINE;
    }
  }

  /* update line cycle count */
  mcycles_vdp += MCYCLES_PER_LINE;

  /* increment line count */
  line++;

  /* Vertical Blanking */
  do
  {
    /* update V Counter */
    v_counter = line;

    /* update 6-Buttons & Lightguns */
    input_refresh();

    /* render overscan */
    if ((line < end) || (line >= start))
    {
      blank_line(line, -bitmap.viewport.x, bitmap.viewport.w + 2*bitmap.viewport.x);
    }

    /* run Z80 */
    z80_run(mcycles_vdp + MCYCLES_PER_LINE);

    /* update line cycle count */
    mcycles_vdp += MCYCLES_PER_LINE;
  }
  while (++line < (lines_per_frame - 1));

  /* adjust Z80 cycle count for next frame */
  Z80.cycleCount -= mcycles_vdp;
}
