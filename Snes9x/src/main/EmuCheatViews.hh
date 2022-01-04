#pragma once
#include <emuframework/Cheats.hh>
#include <cheats.h>

namespace EmuCheats
{
#ifdef MAX_CHEATS
static const unsigned MAX = MAX_CHEATS;
#else
static const unsigned MAX = 150;
#endif
}

namespace EmuEx
{

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
	EmuEditCheatView(ViewAttachParams attach, unsigned cheatIdx, RefreshCheatsDelegate onCheatListChanged_);

private:
	DualTextMenuItem addr{}, value{}, saved{};
	unsigned idx = 0;
	IG::StaticString<6> addrStr{};
	IG::StaticString<2> valueStr{};
	IG::StaticString<2> savedStr{};

	const char *cheatNameString() const final;
	void renamed(const char *str) final;
};

}
