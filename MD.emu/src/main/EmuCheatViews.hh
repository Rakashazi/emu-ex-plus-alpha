#pragma once
#include <emuframework/Cheats.hh>
#include <main/Cheats.hh>
#include <vector>
#include "system.h"
uint decodeCheat(const char *string, uint32 &address, uint16 &data, uint16 &originalData);

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
	DualTextMenuItem code{};
	MdCheat *cheat{};

	void renamed(const char *str) final;

public:
	EmuEditCheatView(ViewAttachParams attach, MdCheat &cheat);
};
