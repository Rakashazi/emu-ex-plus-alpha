#pragma once
#include <emuframework/Cheats.hh>

namespace EmuCheats
{

static const unsigned MAX = 254;

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
	TextMenuItem addGG{}, addRAM{};

	void loadCheatItems() final;

public:
	EmuEditCheatListView(ViewAttachParams attach);
};

class EmuEditCheatView : public BaseEditCheatView
{
private:
	DualTextMenuItem addr{}, value{}, comp{}, ggCode{};
	unsigned idx = 0;
	int type = 0;
	char addrStr[5]{}, valueStr[3]{}, compStr[3]{}, ggCodeStr[9]{};

	void syncCheat(const char *newName = {});
	const char *cheatNameString() const final;
	void renamed(const char *str) final;

public:
	EmuEditCheatView(ViewAttachParams attach, unsigned cheatIdx, RefreshCheatsDelegate onCheatListChanged);
};
