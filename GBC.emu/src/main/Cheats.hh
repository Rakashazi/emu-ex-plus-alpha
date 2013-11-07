#pragma once
#include <util/bits.h>
#include <util/collection/ArrayList.hh>
#include <EmuSystem.hh>

struct GbcCheat
{
	constexpr GbcCheat() { }
	uchar flags = 0;
	char name[64] {0};
	char code[12] {0};

	static const uint ON = IG::bit(0);

	bool isOn()
	{
		return flags & ON;
	}

	void toggleOn()
	{
		toggleBits(flags, ON);
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
