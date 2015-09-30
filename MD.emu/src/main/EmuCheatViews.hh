#pragma once
#include <emuframework/Cheats.hh>
#include <main/Cheats.hh>
#include <vector>
#include "system.h"
uint decodeCheat(const char *string, uint32 &address, uint16 &data, uint16 &originalData);

class SystemEditCheatView : public EditCheatView
{
private:
	DualTextMenuItem code{};
	MdCheat *cheat{};
	MenuItem *item[3]{};

	void renamed(const char *str) override;
	void removed() override;

public:
	SystemEditCheatView(Base::Window &win);
	void init(MdCheat &cheat);
};

extern SystemEditCheatView editCheatView;

class EditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addCode{};
	std::vector<TextMenuItem> cheat{};

	void loadAddCheatItems(std::vector<MenuItem*> &item) override;
	void loadCheatItems(std::vector<MenuItem*> &item) override;

public:
	EditCheatListView(Base::Window &win);
};

class CheatsView : public BaseCheatsView
{
private:
	std::vector<BoolMenuItem> cheat{};

	void loadCheatItems(std::vector<MenuItem*> &item) override;

public:
	CheatsView(Base::Window &win): BaseCheatsView(win) {}
};
