#pragma once
#include <emuframework/Cheats.hh>
#include <cheats.h>

namespace EmuCheats
{

static const uint MAX = MAX_CHEATS;

}

class EmuCheatsView : public BaseCheatsView
{
private:
	void loadCheatItems() override;

public:
	EmuCheatsView(ViewAttachParams attach);
};

class EmuEditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addCode{};

	void loadCheatItems() override;

public:
	EmuEditCheatListView(ViewAttachParams attach);
};

class EmuEditCheatView : public BaseEditCheatView
{
private:
	DualTextMenuItem addr{}, value{}, saved{};
	uint idx = 0;
	char addrStr[7]{}, valueStr[3]{}, savedStr[3]{};

	void renamed(const char *str) override;

public:
	EmuEditCheatView(ViewAttachParams attach, uint cheatIdx);
};
