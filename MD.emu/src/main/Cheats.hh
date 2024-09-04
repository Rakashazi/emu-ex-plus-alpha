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
#include <vector>

namespace EmuEx
{

// Needs to be a #define because it get strigified in Cheats.cc
#define MAX_CHEAT_NAME_CHARS 127

using CheatCodeString = StaticString<11>;

struct CheatCode
{
	CheatCodeString text;
	uint32_t address{};
	uint16_t data{};
	uint16_t origData{};
	uint8_t *prev{};

	constexpr bool operator==(const CheatCode& rhs) const { return text == rhs.text;  }
};

struct Cheat
{
	StaticString<MAX_CHEAT_NAME_CHARS> name;
	std::vector<CheatCode> codes;
	union
	{
		struct{uint8_t on:1, applied:1;};
		uint8_t flags{};
	};

	bool operator==(Cheat const& rhs) const
	{
		return codes == rhs.codes;
	}
};

}
