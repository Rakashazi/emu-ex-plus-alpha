#pragma once
#include <imagine/util/bits.h>
#include <imagine/util/container/ArrayList.hh>
#include <emuframework/EmuSystem.hh>

namespace EmuCheats
{

static const uint MAX = 255;

}

struct GbcCheat
{
	constexpr GbcCheat() {}
	uint8_t flags = 0;
	char name[64]{};
	char code[12]{};

	static const uint8_t ON = IG::bit(0);

	bool isOn()
	{
		return flags & ON;
	}

	void toggleOn()
	{
		flags = IG::flipBits(flags, ON);
		// if(game running) refresh cheats
	}

	bool operator ==(GbcCheat const& rhs) const
	{
		return string_equal(code, rhs.code);
	}
};

extern bool cheatsModified;
extern StaticArrayList<GbcCheat, EmuCheats::MAX> cheatList;
void applyCheats();
void readCheatFile();
void writeCheatFile();
