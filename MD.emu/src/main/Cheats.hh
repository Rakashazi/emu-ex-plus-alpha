#pragma once

/*  This file is part of MD.emu.

	MD.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MD.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MD.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/util/container/ArrayList.hh>
#include <imagine/util/bit.hh>
#include <imagine/util/string.h>
#include <emuframework/EmuSystem.hh>

namespace EmuEx
{

// Needs to be a #define because it get strigified in Cheats.cc
#define MAX_CHEAT_NAME_CHARS 127

struct MdCheat
{
	constexpr MdCheat() { }
	uint8_t flags = 0;
	IG::StaticString<MAX_CHEAT_NAME_CHARS> name{};
	IG::StaticString<11> code{};
	uint32_t address = 0;
	uint16_t data = 0;
	uint16_t origData = 0;
	uint8_t *prev{};

	static const uint8_t ON = IG::bit(0), APPLIED = IG::bit(1);

	bool isOn()
	{
		return flags & ON;
	}

	bool isApplied()
	{
		return flags & APPLIED;
	}

	void toggleOn()
	{
		flags ^= ON;
	}

	void setApplied(bool applied)
	{
		IG::setOrClearBits(flags, APPLIED, applied);
	}

	bool operator ==(MdCheat const& rhs) const
	{
		return code == rhs.code;
	}
};

void applyCheats();
void clearCheats();
void updateCheats(); // clears and applies cheats
void clearCheatList();
void writeCheatFile(EmuSystem &);
void readCheatFile(EmuSystem &);
void RAMCheatUpdate();

static constexpr size_t maxCheats = 100;
extern StaticArrayList<MdCheat, maxCheats> cheatList;
extern StaticArrayList<MdCheat*, maxCheats> romCheatList;
extern StaticArrayList<MdCheat*, maxCheats> ramCheatList;

}

void ROMCheatUpdate();
