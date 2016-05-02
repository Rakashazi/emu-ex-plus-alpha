#pragma once
#include <emuframework/Cheats.hh>

namespace EmuCheats
{

static const uint MAX = 254;

}

class EmuCheatsView : public BaseCheatsView
{
private:
	void loadCheatItems() override;

public:
	EmuCheatsView(Base::Window &win);
};

class EmuEditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addGG{}, addRAM{};

	void loadCheatItems() override;

public:
	EmuEditCheatListView(Base::Window &win);
};

class EmuEditCheatView : public BaseEditCheatView
{
private:
	DualTextMenuItem addr{}, value{}, comp{}, ggCode{};
	uint idx = 0;
	int type = 0;
	char *nameStr{};
	char addrStr[5]{}, valueStr[3]{}, compStr[3]{}, ggCodeStr[9]{};

	void syncCheat(const char *newName = {});
	void renamed(const char *str) override;

public:
	EmuEditCheatView(Base::Window &win, uint cheatIdx);
};
