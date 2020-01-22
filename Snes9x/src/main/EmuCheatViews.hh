#pragma once
#include <emuframework/Cheats.hh>
#include <cheats.h>

namespace EmuCheats
{
#ifdef MAX_CHEATS
static const uint MAX = MAX_CHEATS;
#else
static const uint MAX = 150;
#endif
}

class EmuCheatsView : public BaseCheatsView
{
private:
	void loadCheatItems() final;

public:
	EmuCheatsView(ViewAttachParams attach);
};

class EmuEditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addCode{};

	void loadCheatItems() final;

public:
	EmuEditCheatListView(ViewAttachParams attach);
};

class EmuEditCheatView : public BaseEditCheatView
{
private:
	DualTextMenuItem addr{}, value{}, saved{};
	uint idx = 0;
	char addrStr[7]{}, valueStr[3]{}, savedStr[3]{};

	void renamed(const char *str) final;

public:
	EmuEditCheatView(ViewAttachParams attach, uint cheatIdx);
};
