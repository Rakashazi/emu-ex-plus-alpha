#pragma once

/*  This file is part of GBC.emu.

	GBC.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	GBC.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with GBC.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/util/bit.hh>
#include <imagine/util/container/ArrayList.hh>
#include <imagine/util/string.h>
#include <emuframework/EmuSystem.hh>

namespace EmuEx
{

struct GbcCheat
{
	constexpr GbcCheat() {}
	uint8_t flags = 0;
	IG::StaticString<64> name{};
	IG::StaticString<12> code{};

	static const uint8_t ON = IG::bit(0);

	bool isOn()
	{
		return flags & ON;
	}

	void toggleOn()
	{
		flags ^= ON;
		// if(game running) refresh cheats
	}

	bool operator ==(GbcCheat const& rhs) const
	{
		return code == rhs.code;
	}
};

static constexpr size_t maxCheats = 255;
extern StaticArrayList<GbcCheat, maxCheats> cheatList;
void readCheatFile(EmuSystem &);

}
