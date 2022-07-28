/****************************************************************************
 *  Genesis Plus
 *  MegaSD flashcart CD hardware interface overlay & enhanced ROM mappers
 *
 *  Copyright (C) 2020-2021 Eke-Eke (Genesis Plus GX)
 *
 *  Redistribution and use of this code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *   - Redistributions may not be sold, nor may they be used in a commercial
 *     product or activity.
 *
 *   - Redistributions that are modified from the original source must include the
 *     complete source code, including the source code for all components used by a
 *     binary built from the modified sources. However, as a special exception, the
 *     source code distributed need not include anything that is normally distributed
 *     (in either source or binary form) with the major components (compiler, kernel,
 *     and so on) of the operating system on which the executable runs, unless that
 *     component itself accompanies the executable.
 *
 *   - Redistributions must reproduce the above copyright notice, this list of
 *     conditions and the following disclaimer in the documentation and/or other
 *     materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************************/
#include "shared.h"

typedef struct
{
  uint8 unlock;
  uint8 bank0;
  uint8 special;
  uint8 writeEnable;
  uint8 overlayEnable;
  uint8 playbackLoop;
  uint8 playbackLoopTrack;
  uint8 playbackEndTrack;
  uint16 result;
  uint16 fadeoutStartVolume;
  int fadeoutSamplesTotal;
  int fadeoutSamplesCount;
  int playbackSamplesCount;
  int playbackLoopSector;
  int playbackEndSector;
  uint8 buffer[0x800];
} T_MEGASD_HW;

/* MegaSD mapper hardware */
static T_MEGASD_HW megasd_hw;

/* Internal function prototypes */
static void megasd_ctrl_write_byte(unsigned int address, unsigned int data);
static void megasd_ctrl_write_word(unsigned int address, unsigned int data);
static unsigned int megasd_ctrl_read_byte(unsigned int address);
static unsigned int megasd_ctrl_read_word(unsigned int address);
static void megasd_pcm_write_byte(unsigned int address, unsigned int data);
static void megasd_pcm_write_word(unsigned int address, unsigned int data);
static unsigned int megasd_pcm_read_byte(unsigned int address);
static unsigned int megasd_pcm_read_word(unsigned int address);

/* default MegaSD version & serial number */
static const unsigned char megasd_version[16] = {'M','E','G','A','S','D',0x01,0x04,0x07,0x00,0xFF,0xFF,0x12,0x34,0x56,0x78};

/* MegaSD ID string (NB: there seems to be an error in MegaSD Dev manual as given hexadecimal values correspond to ASCII word "BATE", not "RATE") */
static const unsigned char megasd_id[4] = {0x42,0x41,0x54,0x45};

void megasd_reset(void)
{
  /* reset MegaSD mapper */
  memset(&megasd_hw, 0x00, sizeof (megasd_hw));

  /* cartridge area bank #7 default mapping */
  megasd_hw.special = 0x07;

  /* CD hardware access overlay is mapped in 0x030000-0x03FFFF */
  m68k.memory_map[0x03].read8   = megasd_ctrl_read_byte;
  m68k.memory_map[0x03].read16  = megasd_ctrl_read_word;
  m68k.memory_map[0x03].write8  = megasd_ctrl_write_byte;
  m68k.memory_map[0x03].write16 = megasd_ctrl_write_word;
  zbank_memory_map[0x03].read   = megasd_ctrl_read_byte;

  /* reset CD hardware */
  pcm_reset();
  cdd_reset();
  scd.regs[0x36>>1].byte.h = 0x01;
}

int megasd_context_save(uint8 *state)
{
  int bufferptr = 0;

  save_param(&megasd_hw, sizeof(megasd_hw));
  bufferptr += cdd_context_save(&state[bufferptr]);
  bufferptr += pcm_context_save(&state[bufferptr]);
  save_param(&scd.regs[0x36>>1].byte.h, 1);

  return bufferptr;
}

int megasd_context_load(uint8 *state)
{
  int bufferptr = 0;

  load_param(&megasd_hw, sizeof(megasd_hw));
  bufferptr += cdd_context_load(&state[bufferptr], STATE_VERSION);
  bufferptr += pcm_context_load(&state[bufferptr]);
  load_param(&scd.regs[0x36>>1].byte.h, 1);

  return bufferptr;
}

/*
  Enhanced "SSF2" mapper
*/
void megasd_enhanced_ssf2_mapper_w(unsigned int address, unsigned int data)
{
  int i, updateLastBank = 0;

  switch (address & 0xf)
  {
    case 0x0:
    {
      /* check protect bit */
      if (data & 0x80)
      {
        /* access to bank #0 register and ROM write enable bit is unlocked */
        megasd_hw.unlock = 1;

        /* ROM write enable bit */
        megasd_hw.writeEnable = data & 0x20;

        /* map bank #0 selected ROM page to $000000-$07ffff */
        for (i=0x00; i<0x08; i++)
        {
          m68k.memory_map[i].base = cart.rom + (((megasd_hw.bank0 & 0x0f) << 19) & cart.mask) + (i<<16);
        }
      }
      else
      {
        /* access to bank #0 register and ROM write enable bit is locked */
        megasd_hw.unlock = 0;

        /* disable ROM write enable access */
        megasd_hw.writeEnable = 0;

        /* reset default ROM mapping in $000000-$07ffff */
        for (i=0x00; i<0x08; i++)
        {
          m68k.memory_map[i].base = cart.rom + (i<<16);
        }
      }

      /* check ROM write enable status */
      if (megasd_hw.writeEnable)
      {
        /* enable write access to cartridge ROM area ($000000-$37ffff) */
        for (i=0x00; i<0x38; i++)
        {
          m68k.memory_map[i].write8   = NULL;
          m68k.memory_map[i].write16  = NULL;
          zbank_memory_map[i].write   = NULL;
        }
      }
      else
      {
        /* disable ROM write access to cartridge ROM area ($000000-$37ffff) */
        for (i=0x00; i<0x38; i++)
        {
          m68k.memory_map[i].write8   = m68k_unused_8_w;
          m68k.memory_map[i].write16  = m68k_unused_16_w;
          zbank_memory_map[i].write   = zbank_unused_w;
        }

        /* enable CD hardware overlay area access (it is assumed that overlay area is disabled when ROM write access is enabled) */
        m68k.memory_map[0x03].write8 = megasd_ctrl_write_byte;
        m68k.memory_map[0x03].write16 = megasd_ctrl_write_word;
      }

      /* force $380000-$3fffff mapping update */
      updateLastBank = 1;

      break;
    }

    case 0xf:
    {
      /* special bank register */
      megasd_hw.special = data;

      /* force $380000-$3fffff mapping update */
      updateLastBank = 1;

      break;
    }

    default:
    {
      /* LWR only */
      if (address & 1)
      {
        /* 512K ROM paging (max. 8MB)*/
        uint8 *src = cart.rom + (((data  & 0x0f) << 19) & cart.mask);

        /* cartridge area ($000000-$3FFFFF) is divided into 8 x 512K banks */
        uint8 bank = (address << 2) & 0x38;

        /* check selected bank is not locked */
        if ((bank != 0x00) || megasd_hw.unlock)
        {
          /* selected ROM page is mapped to selected bank */
          for (i=0; i<8; i++)
          {
            m68k.memory_map[bank + i].base = src + (i<<16);
          }
        }
      }
      else
      {
        m68k_unused_8_w(address, data);
      }
      break;
    }
  }

  /*  check if $380000-$3fffff mapping has to be updated */
  if (updateLastBank)
  {
    /* check special bank register value */
    if (megasd_hw.special == 0x80)
    {
      /* SRAM mapped in $380000-$3fffff (max 16KB) */
      for (i=0x38; i<0x40; i++)
      {
        m68k.memory_map[i].base    = sram.sram;
        m68k.memory_map[i].read8   = sram_read_byte;
        m68k.memory_map[i].read16  = sram_read_word;
        m68k.memory_map[i].write8  = megasd_hw.writeEnable ? sram_write_byte : m68k_unused_8_w;
        m68k.memory_map[i].write16 = megasd_hw.writeEnable ? sram_write_word : m68k_unused_16_w;
        zbank_memory_map[i].read   = sram_read_byte;
        zbank_memory_map[i].write  = megasd_hw.writeEnable ? sram_write_byte : zbank_unused_w;
      }
    }
    else if (megasd_hw.special == 0x81)
    {
      /* PCM hardware mapped in $380000-$3fffff */
      for (i=0x38; i<0x40; i++)
      {
        m68k.memory_map[i].base    = NULL;
        m68k.memory_map[i].read8   = megasd_pcm_read_byte;
        m68k.memory_map[i].read16  = megasd_pcm_read_word;
        m68k.memory_map[i].write8  = megasd_hw.writeEnable ? megasd_pcm_write_byte : m68k_unused_8_w;
        m68k.memory_map[i].write16 = megasd_hw.writeEnable ? megasd_pcm_write_word : m68k_unused_16_w;
        zbank_memory_map[i].read   = megasd_pcm_read_byte;
        zbank_memory_map[i].write  = megasd_hw.writeEnable ? megasd_pcm_write_byte : zbank_unused_w;
      }
    }
    else
    {
      /* 512K ROM paging (max. 8MB)*/
      uint8 *src = cart.rom + (((megasd_hw.special & 0x0f) << 19) & cart.mask);

      /* selected ROM page mapped in $380000-$3fffff */
      for (i=0x38; i<0x40; i++)
      {
        m68k.memory_map[i].base    = src + (i << 16);;
        m68k.memory_map[i].read8   = NULL;
        m68k.memory_map[i].read16  = NULL;
        m68k.memory_map[i].write8  = megasd_hw.writeEnable ? NULL : m68k_unused_8_w;
        m68k.memory_map[i].write16 = megasd_hw.writeEnable ? NULL : m68k_unused_16_w;
        zbank_memory_map[i].read   = NULL;
        zbank_memory_map[i].write  = megasd_hw.writeEnable ? NULL : zbank_unused_w;;
      }
    }
  }
}

/*
  ROM write access mapper
*/
void megasd_rom_mapper_w(unsigned int address, unsigned int data)
{
  int i;

  if ((address & 0xff) == 0xff)
  {
    if (data == 'W')
    {
      /* enable write access to cartridge ROM area ($000000-$3fffff) */
      for (i=0; i<0x40; i++)
      {
        m68k.memory_map[i].write8   = NULL;
        m68k.memory_map[i].write16  = NULL;
        zbank_memory_map[i].write   = NULL;
      }
    }
    else
    {
      /* disable write access to cartridge ROM area ($000000-$3fffff) */
      for (i=0; i<0x40; i++)
      {
        m68k.memory_map[i].write8   = m68k_unused_8_w;
        m68k.memory_map[i].write16  = m68k_unused_16_w;
        zbank_memory_map[i].write   = zbank_unused_w;
      }

      /* enable CD hardware overlay access */
      m68k.memory_map[0x03].write8 = megasd_ctrl_write_byte;
      m68k.memory_map[0x03].write16 = megasd_ctrl_write_word;
    }
  }
  else
  {
    m68k_unused_8_w(address, data);
  }
}

/*
  CDDA samples playback handler
*/
void megasd_update_cdda(unsigned int samples)
{
  unsigned int count;

  while (samples > 0)
  {
    /* attempt to read remaing needed samples by default */
    count = samples;

    /* check against fade out remaining samples */
    if ((megasd_hw.fadeoutSamplesCount > 0) && (count > megasd_hw.fadeoutSamplesCount))
    {
      count = megasd_hw.fadeoutSamplesCount;
    }

    /* check against playback remaining samples */
    if ((megasd_hw.playbackSamplesCount > 0) && (count > megasd_hw.playbackSamplesCount))
    {
      count = megasd_hw.playbackSamplesCount;
    }

    /* read required CD-DA samples */
    cdd_read_audio(count);

    /* adjust remaing needed samples count */
    samples -= count;

    /* check if fade out is still in progress */
    if (megasd_hw.fadeoutSamplesCount > 0)
    {
      /* update remaining fade out samples count */
      megasd_hw.fadeoutSamplesCount -= count;

      /* check end of fade out */
      if (megasd_hw.fadeoutSamplesCount <= 0)
      {
        /* pause audio playback */
        scd.regs[0x36>>1].byte.h = 0x01;
        cdd.status = CD_PAUSE;

        /* restore initial volume */
        cdd.fader[0] = cdd.fader[1] = megasd_hw.fadeoutStartVolume;
      }
      else
      {
        /* force volume for next frame */
        cdd.fader[0] = cdd.fader[1] = (megasd_hw.fadeoutSamplesCount * megasd_hw.fadeoutStartVolume) / megasd_hw.fadeoutSamplesTotal;
      }
    }

    /* Playback in progress ? */
    if (megasd_hw.playbackSamplesCount > 0)
    {
      /* update remaining samples count */
      megasd_hw.playbackSamplesCount -= count;

      /* check end of current track */
      if (megasd_hw.playbackSamplesCount <= 0)
      {
        /* check playback end track */
        if (cdd.index < megasd_hw.playbackEndTrack)
        {
          /* seek start of next track */
          cdd_seek_audio(cdd.index + 1, cdd.toc.tracks[cdd.index + 1].start);

          /* increment current track index */
          cdd.index++;

          /* check if last track is being played */
          if (cdd.index == megasd_hw.playbackEndTrack)
          {
            /* reinitialize remaining samples count using current track start sector and playback end sectors */
            megasd_hw.playbackSamplesCount = (megasd_hw.playbackEndSector - cdd.toc.tracks[cdd.index].start) * 588;
          }
          else
          {
            /* reinitialize remaining samples count using current track start and end sectors */
            megasd_hw.playbackSamplesCount = (cdd.toc.tracks[cdd.index].end - cdd.toc.tracks[cdd.index].start) * 588;
          }
        }

        /* check track loop */
        else if (megasd_hw.playbackLoop)
        {
          /* seek back to start track loop sector */
          cdd_seek_audio(megasd_hw.playbackLoopTrack, megasd_hw.playbackLoopSector);

          /* update current track index */
          cdd.index = megasd_hw.playbackLoopTrack;

          /* check if single track or successive tracks are being played */
          if (cdd.index == megasd_hw.playbackEndTrack)
          {
            /* reinitialize remaining samples count using playback loop and end sectors */
            megasd_hw.playbackSamplesCount = (megasd_hw.playbackEndSector - megasd_hw.playbackLoopSector) * 588;
          }
          else
          {
            /* reinitialize remaining samples count using playback loop sector and track end sector */
            megasd_hw.playbackSamplesCount = (cdd.toc.tracks[cdd.index].end - megasd_hw.playbackLoopSector) * 588;
          }
        }
        else
        {
          /* stop audio playback */
          cdd.status = CD_STOP;
          scd.regs[0x36>>1].byte.h = 0x01;
        }
      }
    }
  }
}

/*
   CD hardware overlay interface
*/
static void megasd_ctrl_write_byte(unsigned int address, unsigned int data)
{
  /* check if overlay area access is enabled */
  if (megasd_hw.overlayEnable) 
  {
    /* 2KB buffer area */
    if (address >= 0x03f800)
    {
      megasd_hw.buffer[address & 0x7ff] = data;
      return;
    }
  }

  /* cartridge area write access is disabled by default */
  m68k_unused_8_w(address, data);
}

static void megasd_ctrl_write_word(unsigned int address, unsigned int data)
{
  /* overlay port (word write only) */
  if (address == 0x03f7fa)
  {
    /* enable /disable CD hardware overlay access */
    megasd_hw.overlayEnable = (data == 0xcd54) ? 1 : 0;
    return;
  }

  /* check if overlay area access is enabled */
  if (megasd_hw.overlayEnable) 
  {
    /* command port (word write only) */
    if (address == 0x03f7fe)
    {
      switch ((data >> 8) & 0xff)
      {
        case 0x10:  /* Get MegaSD version & serial number */
        {
          memcpy(megasd_hw.buffer, megasd_version, sizeof(megasd_version));
          return;
        }

        case 0x11:  /* Play specified CDDA track and stop */
        case 0x12:  /* Play specified CDDA track and loop from start */
        case 0x1a:  /* Play specified CDDA track and loop from offset */
        {
          /* check a valid disc is loaded */
          if (cdd.loaded)
          {
            /* get track index from command parameter */
            int index = (data & 0xff) - 1;

            /* check track index corresponds to a valid audio track */
            if ((index >= 0) && (index < cdd.toc.last) && !cdd.toc.tracks[index].type)
            {
              /* initialize default playback info */
              megasd_hw.playbackEndTrack = index;
              megasd_hw.playbackEndSector = cdd.toc.tracks[index].end;

              /* seek audio track start */
              cdd_seek_audio(index, cdd.toc.tracks[index].start);

              /* update current track index */
              cdd.index = index;

              /* indicate audio track is playing */
              cdd.status = CD_PLAY;
              scd.regs[0x36>>1].byte.h = 0x00;

              /* check if fade out is still in progress */
              if (megasd_hw.fadeoutSamplesCount > 0)
              {
                /* reset fade out */
                megasd_hw.fadeoutSamplesCount = 0;

                /* restore initial volume */
                cdd.fader[0] = cdd.fader[1] = megasd_hw.fadeoutStartVolume;
              }

              /* initialize remaining samples count */
              megasd_hw.playbackSamplesCount = (cdd.toc.tracks[index].end - cdd.toc.tracks[index].start) * 588;

              /* check if there are any loop commands found in cue file for this track (see cdd_load function in cdd.c) */
              if (cdd.toc.tracks[index].loopEnabled != 0)
              {
                /* cue file loop commands override programmed command loop settings */
                megasd_hw.playbackLoop = cdd.toc.tracks[index].loopEnabled + 1;
                megasd_hw.playbackLoopSector = cdd.toc.tracks[index].start + cdd.toc.tracks[index].loopOffset;
              }
              else
              {
                /* track loop is enabled for commands 12h and 1Ah / disabled for command 11h */
                megasd_hw.playbackLoop = (data >> 8) & 0x02;

                /* command 1Ah specifies track loop offset in data buffer (32-bit value stored in big-endian format) */
                if ((data >> 8) == 0x1a)
                {
#ifndef LSB_FIRST 
                  megasd_hw.playbackLoopSector = cdd.toc.tracks[index].start + *(unsigned int *)(megasd_hw.buffer);
#else
                  megasd_hw.playbackLoopSector = cdd.toc.tracks[index].start + (megasd_hw.buffer[0] << 24) + (megasd_hw.buffer[1] << 16) + (megasd_hw.buffer[2] << 8) + megasd_hw.buffer[3];
#endif
                }
                else
                {
                  /* default track loop offset */
                  megasd_hw.playbackLoopSector = cdd.toc.tracks[index].start;
                }
              }

              /* check loop sector is within track limit */
              if ((megasd_hw.playbackLoopSector < cdd.toc.tracks[index].start) || (megasd_hw.playbackLoopSector >= cdd.toc.tracks[index].end))
              {
                /* force default track loop offset */
                megasd_hw.playbackLoopSector = cdd.toc.tracks[index].start;
              }

              /* initialize loop track index */
              megasd_hw.playbackLoopTrack = index;
            }
          }
          return;
        }

        case 0x13:  /* Pause CDDA track */
        {
          /* check audio track is currently playing */
          if (scd.regs[0x36>>1].byte.h == 0x00)
          {
            /* check if fade out is already in progress */
            if (megasd_hw.fadeoutSamplesCount > 0)
            {
              /* restore initial volume */
              cdd.fader[0] = cdd.fader[1] = megasd_hw.fadeoutStartVolume;
            }

            /* get fade out samples count from command parameter */
            megasd_hw.fadeoutSamplesCount = (data & 0xff) * 588;

            /* fade out enabled ? */
            if (megasd_hw.fadeoutSamplesCount > 0)
            {
              /* save fade out sample count */
              megasd_hw.fadeoutSamplesTotal = megasd_hw.fadeoutSamplesCount;

              /* save current volume */
              megasd_hw.fadeoutStartVolume = cdd.fader[0];
            }
            else
            {
              /* pause audio track playback immediately when fade out is disabled */
              scd.regs[0x36>>1].byte.h = 0x01;
              cdd.status = CD_PAUSE;
            }
          }
          return;
        }

        case 0x14:  /* Resume CDDA track */
        {
          /* check audio playback is currently paused */
          if (cdd.status == CD_PAUSE)
          {
            scd.regs[0x36>>1].byte.h = 0x00;
            cdd.status = CD_PLAY;
          }
          return;
        }

        case 0x15:  /* Set CDDA volume (0-255) */
        {
          /* check if fade out is in progress */
          if (megasd_hw.fadeoutSamplesCount > 0)
          {
            /* update default volume to be restored once fade out is finished */
            megasd_hw.fadeoutStartVolume = ((data & 0xff) * 0x400) / 255;
          }
          else
          {
            /* update current volume */
            cdd.fader[0] = cdd.fader[1] = ((data & 0xff) * 0x400) / 255;
          }
          return;
        }

        case 0x16:  /* Get CDDA playback status */
        {
          megasd_hw.result = (scd.regs[0x36>>1].byte.h == 0x00) ? 0x01 : 0x00;
          return;
        }

        case 0x17:  /* Request CD sector read */
        {
          /* check a disc with valid data track is loaded */
          if (cdd.loaded && cdd.toc.tracks[0].type)
          {
            /* get LBA from command buffer (32-bit value stored in big-endian format) */
#ifndef LSB_FIRST 
            int lba = *(unsigned int *)(megasd_hw.buffer) - 150;
#else
            int lba = (megasd_hw.buffer[0] << 24) + (megasd_hw.buffer[1] << 16) + (megasd_hw.buffer[2] << 8) + megasd_hw.buffer[3] - 150;
#endif
            /* only allow reading within first data track */
            if (lba < cdd.toc.tracks[0].end)
            {
              /* set current LBA position */
              cdd.lba = lba;

              /* set current track index */
              cdd.index = 0;

              /* update CDD status to allow reading data track */
              cdd.status = CD_PLAY;

              /* no audio track playing */
              scd.regs[0x36>>1].byte.h = 0x01;
            }
          }
          return;
        }

        case 0x18:  /* Transfer last read sector */
        {
          /* check a disc is loaded with a valid data track currently being read */
          if (cdd.loaded && (cdd.status == CD_PLAY) && cdd.toc.tracks[cdd.index].type)
          {
            /* read sector data to 2K buffer */
            cdd_read_data(megasd_hw.buffer, NULL);
          }
          return;
        }

        case 0x19:  /* Request read of next sector */
        {
          /* check a disc is loaded with a valid data track currently being read */
          if (cdd.loaded && (cdd.status == CD_PLAY) && cdd.toc.tracks[cdd.index].type)
          {
             /* only allow reading within first data track */
             if (cdd.lba < (cdd.toc.tracks[0].end - 1))
             {
               /* increment current LBA position */
               cdd.lba++;
             }
          }
          return;
        }

        case 0x1b:  /* Play CDDA from specific sector */
        {
          /* check a valid disc is loaded */
          if (cdd.loaded)
          {
            /* get playback start sector from command buffer (32-bit value in big-endian format) */
#ifndef LSB_FIRST
            int lba = *(unsigned int *)(megasd_hw.buffer) - 150;
#else
            int lba = (megasd_hw.buffer[0] << 24) + (megasd_hw.buffer[1] << 16) + (megasd_hw.buffer[2] << 8) + megasd_hw.buffer[3] - 150;
#endif

            /* get playback start track index */
            int index  = 0;
            while ((cdd.toc.tracks[index].end <= lba) && (index < cdd.toc.last))
            {
              index++;
            }

            /* check start track index corresponds to a valid audio track */
            if ((index < cdd.toc.last) && (cdd.toc.tracks[index].type == 0x00))
            {
              /* stay within track limits when seeking files */
              if (lba < cdd.toc.tracks[index].start) 
              {
                lba = cdd.toc.tracks[index].start;
              }

              /* seek audio track start offset */
              cdd_seek_audio(index, lba);

              /* update current track index */
              cdd.index = index;

              /* indicate audio track is playing */
              cdd.status = CD_PLAY;
              scd.regs[0x36>>1].byte.h = 0x00;

              /* check if fade out is still in progress */
              if (megasd_hw.fadeoutSamplesCount > 0)
              {
                /* reset fade out */
                megasd_hw.fadeoutSamplesCount = 0;

                /* restore initial volume */
                cdd.fader[0] = cdd.fader[1] = megasd_hw.fadeoutStartVolume;
              }

              /* get playback end sector from command buffer (32-bit value in big-endian format) */
#ifndef LSB_FIRST 
              megasd_hw.playbackEndSector = *(unsigned int *)(megasd_hw.buffer + 4) - 150;
#else
              megasd_hw.playbackEndSector = (megasd_hw.buffer[4] << 24) + (megasd_hw.buffer[5] << 16) + (megasd_hw.buffer[6] << 8) + megasd_hw.buffer[7] - 150;
#endif

              /* check playback end sector is greater than start sector */
              if (megasd_hw.playbackEndSector > lba) 
              {
                /* get playback end track index */
                megasd_hw.playbackEndTrack = index;
                while ((cdd.toc.tracks[megasd_hw.playbackEndTrack].end <= megasd_hw.playbackEndSector) && (megasd_hw.playbackEndTrack < cdd.toc.last))
                {
                  megasd_hw.playbackEndTrack++;
                }

                /* force playback end sector to end of last track when greater than last disc sector */
                if (megasd_hw.playbackEndTrack == cdd.toc.last) 
                {
                  megasd_hw.playbackEndSector = cdd.toc.tracks[cdd.toc.last - 1].end;
                  megasd_hw.playbackEndTrack  = cdd.toc.last - 1;
                }
              }
              else
              {
                /* force playback end sector to end of playback start track otherwise */
                megasd_hw.playbackEndSector = cdd.toc.tracks[index].end;
                megasd_hw.playbackEndTrack  = index;
              }

              /* check if a single track or successive tracks are played */
              if (megasd_hw.playbackEndTrack == index)
              {
                /* initialize remaining samples count using playback start and end sectors */
                megasd_hw.playbackSamplesCount = (megasd_hw.playbackEndSector - lba) * 588;
              }
              else
              {
                /* initialize remaining samples count using playback start sector and start track last sector */
                megasd_hw.playbackSamplesCount = (cdd.toc.tracks[index].end - lba) * 588;
              }

              /* check if there are any loop commands found in cue file for this track (see cdd_load function in cdd.c) */
              if (cdd.toc.tracks[index].loopEnabled != 0)
              {
                /* cue file loop commands override programmed command loop settings */
                megasd_hw.playbackLoop = cdd.toc.tracks[index].loopEnabled + 1;
                megasd_hw.playbackLoopSector = cdd.toc.tracks[index].start + cdd.toc.tracks[index].loopOffset;
              }
              else
              {
                /* get track loop setting from command parameter */
                megasd_hw.playbackLoop = data & 0x01;

                /* track loop enabled ? */
                if (megasd_hw.playbackLoop)
                {
                  /* get playback loop sector from data buffer (32-bit value in big-endian format) */
#ifndef LSB_FIRST 
                  megasd_hw.playbackLoopSector = *(unsigned int *)(megasd_hw.buffer + 8) - 150;
#else
                  megasd_hw.playbackLoopSector = (megasd_hw.buffer[8] << 24) + (megasd_hw.buffer[9] << 16) + (megasd_hw.buffer[10] << 8) + megasd_hw.buffer[11] - 150;
#endif
                }
                else
                {
                  /* set default track loop offset */
                  megasd_hw.playbackLoopSector = cdd.toc.tracks[index].start;
                }
              }

              /* check loop sector is within start track limits */
              if ((megasd_hw.playbackLoopSector < cdd.toc.tracks[index].start) || (megasd_hw.playbackLoopSector >= cdd.toc.tracks[index].end) || (megasd_hw.playbackLoopSector >= megasd_hw.playbackEndSector))
              {
                /* force default track loop offset */
                megasd_hw.playbackLoopSector = cdd.toc.tracks[index].start;
              }

              /* initialize loop track index */
              megasd_hw.playbackLoopTrack = index;
            }
          }
          return;
        }

        case 0x1C: /* Start reading from a file */
        case 0x1D: /* Read data from file to ROM */
        case 0x1E: /* Read directory files to ROM */
        case 0x1F: /* Play WAV file */
        case 0x20: /* Read 2K data block from file to internal buffer */
        case 0x21: /* Read next 2K data block from file to internal buffer */
        {
          /* unsupported commands */
           megasd_hw.result = 0;
           return;
        }

        default:
        {
          /* invalid command */
          m68k_unused_16_w(address, data);
          return;
        }
      }
    }

    /* 2KB buffer area */
    if (address >= 0x03f800)
    {
      WRITE_WORD(megasd_hw.buffer, address & 0x7fe, data);
      return;
    }
  }

  /* cartridge area write access is disabled by default */
  m68k_unused_16_w(address, data);
}

static unsigned int megasd_ctrl_read_byte(unsigned int address)
{
  /* check if overlay area access is enabled */
  if (megasd_hw.overlayEnable) 
  {
    /* ID port */
    if ((address >= 0x3f7f6) && (address <= 0x3f7f9))
    {
      return megasd_id[address & 0x03];
    }

    /* Overlay port */
    if ((address >= 0x3f7fa) && (address <= 0x3f7fb))
    {
      return (address & 1) ? 0x54 :0xcd;
    }

    /* Result port */
    if ((address >= 0x3f7fc) && (address <= 0x3f7fd))
    {
      return (address & 1) ? (megasd_hw.result & 0xff) : (megasd_hw.result >> 8);
    }

    /* Command port */
    if ((address >= 0x3f7fe) && (address <= 0x3f7ff))
    {
      /* commands processing time is not emulated */
      return 0x00;
    }

    /* 2KB buffer area */
    if (address >= 0x03f800)
    {
      return megasd_hw.buffer[address & 0x7ff];
    }
  }

  /* default cartridge area */
  return READ_BYTE(m68k.memory_map[0x03].base, address & 0xffff);
}

static unsigned int megasd_ctrl_read_word(unsigned int address)
{
  /* check if overlay area access is enabled */
  if (megasd_hw.overlayEnable) 
  {
    /* ID port */
    if ((address == 0x3f7f6) || (address == 0x3f7f8))
    {
      return READ_WORD(megasd_id, address - 0x3f7f6);
    }

    /* Overlay port */
    if (address == 0x3f7fa)
    {
      return 0xcd54;
    }

    /* Result port */
    if (address == 0x3f7fc)
    {
      return megasd_hw.result;
    }

    /* Command port */
    if (address == 0x3f7fe)
    {
      /* commands processing time is not emulated */
      return 0x0000;
    }

    /* 2KB buffer area */
    if (address >= 0x03f800)
    {
      return READ_WORD(megasd_hw.buffer, address & 0x7fe);
    }
  }

  /* default cartridge area */
  return *(uint16 *)(m68k.memory_map[0x03].base + (address & 0xfffe));
}

/* 
  PCM sound chip interface
*/
static void megasd_pcm_write_byte(unsigned int address, unsigned int data)
{
  /* /LDS only */
  if (address & 1)
  {
    pcm_write((address >> 1) & 0x1fff, data, (m68k.cycles * SCYCLES_PER_LINE) / MCYCLES_PER_LINE);
    return;
  }

  m68k_unused_8_w(address, data);
  return;
}

static void megasd_pcm_write_word(unsigned int address, unsigned int data)
{
  /* /LDS only */
  pcm_write((address >> 1) & 0x1fff, data & 0xff, (m68k.cycles * SCYCLES_PER_LINE) / MCYCLES_PER_LINE);
}

static unsigned int megasd_pcm_read_byte(unsigned int address)
{
  /* /LDS only */
  if (address & 1)
  {
    return pcm_read((address >> 1) & 0x1fff, (m68k.cycles * SCYCLES_PER_LINE) / MCYCLES_PER_LINE);
  }

  return 0x00;
}

static unsigned int megasd_pcm_read_word(unsigned int address)
{
  /* /LDS only */
  return pcm_read((address >> 1) & 0x1fff, (m68k.cycles * SCYCLES_PER_LINE) / MCYCLES_PER_LINE);
}
