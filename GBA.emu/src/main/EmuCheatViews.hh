#pragma once
#include <emuframework/Cheats.hh>
#include <vector>

namespace EmuCheats
{

static const uint MAX = 100;

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
	TextMenuItem addGS12CBCode{}, addGS3Code{};

	void loadCheatItems() override;
	void addNewCheat(int isGSv3);

public:
	EmuEditCheatListView(Base::Window &win);
};

class EmuEditCheatView : public BaseEditCheatView
{
private:
	DualTextMenuItem code{};
	uint idx = 0;

	void renamed(const char *str) override;

public:
	EmuEditCheatView(Base::Window &win, uint cheatIdx);
};
