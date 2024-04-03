/*  This file is part of NEO.emu.

	NEO.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NEO.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NEO.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/io/MapIO.hh>
#include <imagine/logger/logger.h>
#include <cstring>

extern "C"
{
	#include <gngeo/roms.h>
	#include <gngeo/timer.h>
	#include <gngeo/memory.h>
	#include <gngeo/video.h>

	void cpu_68k_mkstate(Stream *gzf,int mode);
	void cpu_z80_mkstate(Stream *gzf,int mode);
	void ym2610_mkstate(Stream *gzf,int mode);
	void timer_mkstate(Stream *gzf,int mode);
	void pd4990a_mkstate(Stream *gzf,int mode);
}

#ifdef USE_STARSCREAM
static int m68k_flag=0x1;
#elif USE_GENERATOR68K
static int m68k_flag=0x2;
#elif USE_CYCLONE
static int m68k_flag=0x3;
#elif USE_MUSASHI
static int m68k_flag=0x4;
#endif

#ifdef USE_RAZE
static int z80_flag=0x4;
#elif USE_MAMEZ80
static int z80_flag=0x8;
#elif USE_DRZ80
static int z80_flag=0xC;
#endif

#ifdef WORDS_BIGENDIAN
static int endian_flag=0x10;
#else
static int endian_flag=0x0;
#endif

using namespace IG;

namespace EmuEx
{

bool openState(MapIO &io, int mode)
{
	static const char *stateSig = "GNGST3";
	if(mode==STREAD)
	{
		char string[20]{};
		io.read(string, 6);

		if(strcmp(string, stateSig))
		{
			logErr("%s is not a valid gngeo state header", string);
			return false;
		}

		int flags = io.get<int>();

		if (flags != (m68k_flag | z80_flag | endian_flag))
		{
			logMsg("This save state comes from a different endian architecture.\n"
					"This is not currently supported :(");
			return false;
		}
	}
	else
	{
		int flags = m68k_flag | z80_flag | endian_flag;
		io.write(stateSig, 6);
		io.put(flags);
	}
	return true;
}

static int mkstate_data(MapIO &io, void *data, int size, int mode)
{
	if (mode==STREAD)
		return io.read(data, size);
	return io.write(data, size);
}

void makeState(MapIO &io, int mode)
{
	GAME_ROMS r;
	memcpy(&r,&memory.rom,sizeof(GAME_ROMS));
	mkstate_data(io, &memory, sizeof (memory), mode);

	/* Roms info are needed (at least) for z80 bankswitch, so we need to restore
	 * it asap */
	if (mode==STREAD) memcpy(&memory.rom,&r,sizeof(GAME_ROMS));


	mkstate_data(io, &bankaddress, sizeof (Uint32), mode);
	mkstate_data(io, &sram_lock, sizeof (Uint8), mode);
	mkstate_data(io, &sound_code, sizeof (Uint8), mode);
	mkstate_data(io, &pending_command, sizeof (Uint8), mode);
	mkstate_data(io, &result_code, sizeof (Uint8), mode);
	mkstate_data(io, &neogeo_frame_counter_speed, sizeof (Uint32), mode);
	mkstate_data(io, &neogeo_frame_counter, sizeof (Uint32), mode);
	cpu_68k_mkstate(&io, mode);
	mkstate_data(io, z80_bank,sizeof(Uint16)*4, mode);
	cpu_z80_mkstate(&io, mode);
	ym2610_mkstate(&io, mode);
	timer_mkstate(&io, mode);
	pd4990a_mkstate(&io, mode);
	if(hasPvc)
	{
		mkstate_data(io, pvcMem, sizeof(pvcMem), mode);
	}
}

}

CLINK int mkstate_data(Stream *ptr, void *data, int size, int mode)
{
	return EmuEx::mkstate_data(*static_cast<MapIO*>(ptr), data, size, mode);
}

