#pragma once
#include <emuframework/Cheats.hh>

namespace EmuEx
{

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

class EmuEditCheatView : public BaseEditCheatView<EmuEditCheatView>
{
public:
	EmuEditCheatView(ViewAttachParams attach, unsigned cheatIdx, RefreshCheatsDelegate onCheatListChanged);
	std::string cheatNameString() const;
	void renamed(std::string_view);

private:
	DualTextMenuItem addr{}, value{}, comp{}, ggCode{};
	unsigned idx = 0;
	int type = 0;
	IG::StaticString<4> addrStr{};
	IG::StaticString<2> valueStr{};
	IG::StaticString<2> compStr{};
	IG::StaticString<8> ggCodeStr{};

	void syncCheat(std::string_view newName = {});
};

}
