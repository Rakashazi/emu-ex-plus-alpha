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
	uint8_t flags = 0;
	char name[MAX_CHEAT_NAME_CHARS+1]{};
	char code[12]{};
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
		flags = IG::flipBits(flags, ON);
	}

	void setApplied(bool applied)
	{
		IG::setOrClearBits(flags, APPLIED, applied);
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
