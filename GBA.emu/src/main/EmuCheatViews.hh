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
	EmuCheatsView(ViewAttachParams attach);
};

class EmuEditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addGS12CBCode{}, addGS3Code{};

	void loadCheatItems() override;
	void addNewCheat(int isGSv3);

public:
	EmuEditCheatListView(ViewAttachParams attach);
};

class EmuEditCheatView : public BaseEditCheatView
{
private:
	DualTextMenuItem code{};
	uint idx = 0;

	void renamed(const char *str) override;

public:
	EmuEditCheatView(ViewAttachParams attach, uint cheatIdx);
};
