/***************************************************************************************
 *  Genesis Plus
 *  Savestate support
 *
 *  Copyright (C) 2007-2011  Eke-Eke (GCN/Wii port)
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
#include <imagine/logger/logger.h>
//static unsigned char state[STATE_SIZE] __attribute__ ((aligned (4)));

int state_load(const unsigned char *buffer)
{
	unsigned char *state = (unsigned char*)malloc(STATE_SIZE);
	if(!state)
	{
		logErr("out of memory loading state");
		return -1;
	}

  /* buffer size */
  int bufferptr = 0;

  /* uncompress savestate */
  unsigned long inbytes, outbytes;
  uint32 inbytes32;
  memcpy(&inbytes32, buffer, 4);
  inbytes = inbytes32;
  outbytes = STATE_SIZE;
  logMsg("uncompressing %d bytes to buffer of %d size", (int)inbytes, (int)outbytes);
  {
  	int result = uncompress((Bytef *)state, &outbytes, (Bytef *)(buffer + 4), inbytes);
		if(result != Z_OK)
		{
			logErr("error %d in uncompress loading state", result);
			free(state);
			return -1;
		}
  }

  /* signature check (GENPLUS-GX x.x.x) */
  char version[17];
  load_param(version,16);
  version[16] = 0;
  if (strncmp(version,STATE_VERSION,11))
  {
  	logErr("bad signature loading state");
  	free(state);
    return -1;
  }

  /* version check (1.5.0 and above) */
  if ((version[11] < 0x31) || ((version[11] == 0x31) && (version[13] < 0x35)))
  {
  	logErr("version too old loading state");
  	free(state);
    return -1;
  }

  uint exVersion = (version[15] >= 0x32) ? version[15] - 0x31 : 0;
  if(exVersion)
  {
  	logMsg("state extra version: %d", exVersion);
  }

  /* reset system */
  system_reset();

  // GENESIS
  #ifndef NO_SYSTEM_PBC
  if (system_hw == SYSTEM_PBC)
  {
    load_param(work_ram, 0x2000);
  }
  else
  #endif
  {
    load_param(work_ram, sizeof(work_ram));
    load_param(zram, sizeof(zram));
    load_param(&zstate, sizeof(zstate));
    load_param(&zbank, sizeof(zbank));
    if (zstate == 3)
    {
      mm68k.memory_map[0xa0].read8   = z80_read_byte;
      mm68k.memory_map[0xa0].read16  = z80_read_word;
      mm68k.memory_map[0xa0].write8  = z80_write_byte;
      mm68k.memory_map[0xa0].write16 = z80_write_word;
    }
    else
    {
      mm68k.memory_map[0xa0].read8   = m68k_read_bus_8;
      mm68k.memory_map[0xa0].read16  = m68k_read_bus_16;
      mm68k.memory_map[0xa0].write8  = m68k_unused_8_w;
      mm68k.memory_map[0xa0].write16 = m68k_unused_16_w;
    }
  }

  /* extended state */
  load_param(&mm68k.cycleCount, sizeof(mm68k.cycleCount));
  load_param(&Z80.cycleCount, sizeof(Z80.cycleCount));

  // IO
  #ifndef NO_SYSTEM_PBC
  if (system_hw == SYSTEM_PBC)
  {
    load_param(&io_reg[0], 1);
  }
  else
  #endif
  {
    load_param(io_reg, sizeof(io_reg));
    io_reg[0] = region_code | 0x20 | (config.tmss & 1);
  }

  // VDP
  bufferptr += vdp_context_load(&state[bufferptr]);

  // SOUND 
  bufferptr += sound_context_load(&state[bufferptr], version);

  // 68000 
  #ifndef NO_SYSTEM_PBC
  if (system_hw != SYSTEM_PBC)
  #endif
  {
    uint16 tmp16;
    uint32 tmp32;
    load_param(&tmp32, 4); m68k_set_reg(mm68k, M68K_REG_D0, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(mm68k, M68K_REG_D1, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(mm68k, M68K_REG_D2, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(mm68k, M68K_REG_D3, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(mm68k, M68K_REG_D4, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(mm68k, M68K_REG_D5, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(mm68k, M68K_REG_D6, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(mm68k, M68K_REG_D7, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(mm68k, M68K_REG_A0, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(mm68k, M68K_REG_A1, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(mm68k, M68K_REG_A2, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(mm68k, M68K_REG_A3, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(mm68k, M68K_REG_A4, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(mm68k, M68K_REG_A5, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(mm68k, M68K_REG_A6, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(mm68k, M68K_REG_A7, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(mm68k, M68K_REG_PC, tmp32);
    load_param(&tmp16, 2); m68k_set_reg(mm68k, M68K_REG_SR, tmp16);
    load_param(&tmp32, 4); m68k_set_reg(mm68k, M68K_REG_USP,tmp32);
    if(exVersion >= 1)
    {
    	load_param(&tmp32, 4); m68k_set_reg(mm68k, M68K_REG_ISP,tmp32);
    }
  }

  // Z80 
  load_param(&Z80, sizeof(Z80_Regs));
  //Z80.irq_callback = z80_irq_callback;

  // Cartridge HW
  #ifndef NO_SYSTEM_PBC
  if (system_hw == SYSTEM_PBC)
  {
    bufferptr += sms_cart_context_load(&state[bufferptr]);
  }
  else
  #endif
  {  
    bufferptr += md_cart_context_load(&state[bufferptr]);
  }

	#ifndef NO_SCD
	if (sCD.isActive)
	{
		bufferptr += scd_loadState(&state[bufferptr], exVersion);
	}
	#endif

	free(state);
  return 1;
}

int state_save(unsigned char *buffer)
{
	unsigned char *state = (unsigned char*)malloc(STATE_SIZE);
	if(!state)
		return -1;

  /* buffer size */
  int bufferptr = 0;

  /* version string */
  char version[16] = { 0 };
  memcpy(version,STATE_VERSION,16);
  save_param(version, 16);

  // GENESIS
  #ifndef NO_SYSTEM_PBC
  if (system_hw == SYSTEM_PBC)
  {
    save_param(work_ram, 0x2000);
  }
  else
  #endif
  {
    save_param(work_ram, sizeof(work_ram));
    save_param(zram, sizeof(zram));
    save_param(&zstate, sizeof(zstate));
    save_param(&zbank, sizeof(zbank));
  }
  save_param(&mm68k.cycleCount, sizeof(mm68k.cycleCount));
  save_param(&Z80.cycleCount, sizeof(Z80.cycleCount));

  // IO
  #ifndef NO_SYSTEM_PBC
  if (system_hw == SYSTEM_PBC)
  {
    save_param(&io_reg[0], 1);
  }
  else
  #endif
  {
    save_param(io_reg, sizeof(io_reg));
  }

  // VDP
  bufferptr += vdp_context_save(&state[bufferptr]);

  // SOUND
  bufferptr += sound_context_save(&state[bufferptr]);

  // 68000
  #ifndef NO_SYSTEM_PBC
  if (system_hw != SYSTEM_PBC)
  #endif
  {
    uint16 tmp16;
    uint32 tmp32;
    tmp32 = m68k_get_reg(mm68k, M68K_REG_D0);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(mm68k, M68K_REG_D1);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(mm68k, M68K_REG_D2);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(mm68k, M68K_REG_D3);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(mm68k, M68K_REG_D4);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(mm68k, M68K_REG_D5);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(mm68k, M68K_REG_D6);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(mm68k, M68K_REG_D7);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(mm68k, M68K_REG_A0);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(mm68k, M68K_REG_A1);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(mm68k, M68K_REG_A2);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(mm68k, M68K_REG_A3);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(mm68k, M68K_REG_A4);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(mm68k, M68K_REG_A5);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(mm68k, M68K_REG_A6);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(mm68k, M68K_REG_A7);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(mm68k, M68K_REG_PC);  save_param(&tmp32, 4);
    tmp16 = m68k_get_reg(mm68k, M68K_REG_SR);  save_param(&tmp16, 2);
    tmp32 = m68k_get_reg(mm68k, M68K_REG_USP); save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(mm68k, M68K_REG_ISP); save_param(&tmp32, 4);
  }

  // Z80 
  save_param(&Z80, sizeof(Z80_Regs));

  // Cartridge HW
  #ifndef NO_SYSTEM_PBC
  if (system_hw == SYSTEM_PBC)
  {
    bufferptr += sms_cart_context_save(&state[bufferptr]);
  }
  else
  #endif
  {
    bufferptr += md_cart_context_save(&state[bufferptr]);
  }

	#ifndef NO_SCD
	if (sCD.isActive)
	{
		bufferptr += scd_saveState(&state[bufferptr]);
	}
	#endif

  /* compress state file */
  unsigned long inbytes   = bufferptr;
  unsigned long outbytes  = STATE_SIZE;
  logMsg("compressing %d bytes to buffer of %d size", (int)inbytes, (int)outbytes);
  int ret = compress2 ((Bytef *)(buffer + 4), &outbytes, (Bytef *)state, inbytes, 9);
  logMsg("compress2 returned %d, reduced to %d bytes", ret, (int)outbytes);
  free(state);
  uint32 outbytes32 = outbytes; // assumes no save states will ever be over 4GB
  memcpy(buffer, &outbytes32, 4);

  /* return total size */
  return (outbytes32 + 4);
}
