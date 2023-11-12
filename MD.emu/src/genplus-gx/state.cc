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
#include <system_error>
#include <memory>
#include <format>

static unsigned oldStateSizeAfterZ80Regs()
{
	unsigned size = 0;
  #ifndef NO_SYSTEM_PBC
  if (system_hw == SYSTEM_PBC)
  {
    size += 4;
  }
  else
  #endif
  {
    size += 4 + 0x40;
    if(svp)
  	{
    	auto ssp1601Size = 1280;
  		size += 0x800 + 0x20000 + ssp1601Size;
  	}
  }
	#ifndef NO_SCD
	if (sCD.isActive)
	{
		auto m68kSize = 78;
		size += m68kSize + 920658;
	}
	#endif
	return size;
}

static unsigned oldStateSizeAfterVDP(int exVersion, bool is64Bit)
{
	unsigned size = 0;

	// Sound state
	#ifndef NO_SYSTEM_PBC
  if (system_hw == SYSTEM_PBC)
  {
   size += 5976;
  }
  else
  #endif
  {
	 size += is64Bit ? 19992 : 19748;
	 // DT table indices
	 size += 4 * 6 * 2;
  }

  // SN76489 state
  size += 112;
  // fm_cycles_count & psg_cycles_count
  size += 8;

  // M68K state
	#ifndef NO_SYSTEM_PBC
  if (system_hw != SYSTEM_PBC)
  #endif
  {
    size += (18 * 4) + 2;
    if(exVersion >= 1)
    {
    	size += 4;
    }
  }

  // Z80 state
  size += is64Bit ? 80 : 72;

  size += oldStateSizeAfterZ80Regs();

  return size;
}

void state_load(const unsigned char *buffer)
{
	auto state = std::make_unique<unsigned char[]>(STATE_SIZE);

  /* buffer size */
  unsigned bufferptr = 0;

  /* uncompress savestate */
  int32 inbytes32;
  memcpy(&inbytes32, buffer, 4);
  unsigned long outbytes = STATE_SIZE;
  if(inbytes32 > 0)
  {
		unsigned long inbytes = inbytes32;
		outbytes = STATE_SIZE;
		logMsg("uncompressing %d bytes to buffer of %d size", (int)inbytes, (int)outbytes);
		{
			int result = uncompress((Bytef *)state.get(), &outbytes, (Bytef *)(buffer + 4), inbytes);
			if(result != Z_OK)
			{
				//logErr("error %d in uncompress loading state", result);
				throw std::runtime_error(std::format("Error {} during uncompress", result));
			}
		}
  }
  else // not compressed
  {
  	memcpy(state.get(), buffer + 4, -inbytes32);
  	outbytes = -inbytes32;
  }

  /* signature check (GENPLUS-GX x.x.x) */
  char version[17];
  load_param(version,16);
  version[16] = 0;
  if (strncmp(version,STATE_VERSION,11))
  {
    throw std::runtime_error("Missing header");
  }

  /* version check (1.5.0 and above) */
  if ((version[11] < 0x31) || ((version[11] == 0x31) && (version[13] < 0x35)))
  {
    throw std::runtime_error("Version too old");
  }

  unsigned exVersion = (version[15] >= 0x32) ? version[15] - 0x31 : 0;
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
  unsigned ptrSize = 0;
  if(exVersion < 2)
  {
  	// Old save states include pointer members and padding with different
  	// sizes on 32/64-bit platforms after this point. Use the remaining state
  	// bytes along with the expected remaining bytes to determine if the state
  	// was saved on a 32 or 64-bit machine and how much data to skip over.
  	int bytesLeft32 = oldStateSizeAfterVDP(exVersion, false);
  	int bytesLeft64 = oldStateSizeAfterVDP(exVersion, true);
  	int bytesLeft = (int)outbytes - bufferptr;
  	if(bytesLeft == bytesLeft32)
  	{
  		logMsg("state was made on 32-bit system");
  		ptrSize = 4;
  	}
  	else if(bytesLeft == bytesLeft64)
  	{
  		logMsg("state was made on 64-bit system");
  		ptrSize = 8;
  	}
  	else
  	{
  		logErr("unexpected amount of bytes remaining in state:%d, should be %d or %d",
  			bytesLeft, bytesLeft32, bytesLeft64);
  		system_reset();
  		throw std::runtime_error("Can't determine if created on 32 or 64-bit system");
  	}
  	bufferptr += sound_context_load(&state[bufferptr], version, true, ptrSize);
  }
  else
  {
    bufferptr += sound_context_load(&state[bufferptr], version, false, 0);
  }

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
  if(exVersion < 2)
  {
  	assumeExpr(ptrSize == 4 || ptrSize == 8);
  	logMsg("skipping extra Z80 regs data in state");
  	bufferptr += ptrSize * 2;
  }

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

	if(bufferptr != outbytes)
	{
		system_reset();
		throw std::runtime_error(std::format("Expected {} size state but got {}", bufferptr, (int)outbytes));
	}
}

int state_save(unsigned char *buffer, bool uncompressed)
{
	auto state = std::make_unique<unsigned char[]>(STATE_SIZE);

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

	int32 outbytes32;
  if(uncompressed)
  {
  	memcpy(buffer + 4, state.get(), bufferptr);
  	outbytes32 = -bufferptr; // use negative size to indicate uncompressed state
  	memcpy(buffer, &outbytes32, 4);
  	outbytes32 = bufferptr;
  	return bufferptr + 4;
  }
  else
  {
		unsigned long inbytes   = bufferptr;
		unsigned long outbytes  = STATE_SIZE;
		logMsg("compressing %d bytes to buffer of %d size", (int)inbytes, (int)outbytes);
		int ret = compress2 ((Bytef *)(buffer + 4), &outbytes, (Bytef *)state.get(), inbytes, 9);
		logMsg("compress2 returned %d, reduced to %d bytes", ret, (int)outbytes);
		outbytes32 = outbytes;
		memcpy(buffer, &outbytes32, 4);
		return outbytes + 4;
  }
}
