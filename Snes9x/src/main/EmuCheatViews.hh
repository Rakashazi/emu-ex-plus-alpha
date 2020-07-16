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
public:
	EmuCheatsView(ViewAttachParams attach);

private:
	void loadCheatItems() final;
};

class EmuEditCheatListView : public BaseEditCheatListView
{
public:
	EmuEditCheatListView(ViewAttachParams attach);

private:
	TextMenuItem addCode{};

	void loadCheatItems() final;
};

class EmuEditCheatView : public BaseEditCheatView
{
public:
	EmuEditCheatView(ViewAttachParams attach, uint cheatIdx, RefreshCheatsDelegate onCheatListChanged_);

private:
	DualTextMenuItem addr{}, value{}, saved{};
	uint idx = 0;
	char addrStr[7]{}, valueStr[3]{}, savedStr[3]{};

	const char *cheatNameString() const final;
	void renamed(const char *str) final;
};
