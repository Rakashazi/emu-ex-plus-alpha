#pragma once

#include <imagine/util/container/ArrayList.hh>
#include <imagine/util/bits.h>
#include <emuframework/EmuSystem.hh>

namespace EmuCheats
{

static const uint MAX = 100;

}

// Needs to be a #define because it get strigified in Cheats.cc
#define MAX_CHEAT_NAME_CHARS 127

struct MdCheat
{
	constexpr MdCheat() { }
	uchar flags = 0;
	char name[MAX_CHEAT_NAME_CHARS+1] {0};
	char code[12] {0};
	uint32 address = 0;
	uint16 data = 0;
	uint16 origData = 0;
	uint8 *prev = nullptr;

	static const uint ON = IG::bit(0), APPLIED = IG::bit(1);

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
		toggleBits(flags, ON);
	}

	void setApplied(bool applied)
	{
		if(applied)
			setBits(flags, APPLIED);
		else
			unsetBits(flags, APPLIED);
	}

	bool operator ==(MdCheat const& rhs) const
	{
		return string_equal(code, rhs.code);
	}
};

void applyCheats();
void clearCheats();
void updateCheats(); // clears and applies cheats
void clearCheatList();
void writeCheatFile();
void readCheatFile();
void RAMCheatUpdate();
void ROMCheatUpdate();

extern StaticArrayList<MdCheat, EmuCheats::MAX> cheatList;
extern StaticArrayList<MdCheat*, EmuCheats::MAX> romCheatList;
extern StaticArrayList<MdCheat*, EmuCheats::MAX> ramCheatList;
extern bool cheatsModified;
