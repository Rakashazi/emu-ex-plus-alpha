#pragma once
#include <emuframework/Cheats.hh>
#include <main/Cheats.hh>

class SystemEditCheatView : public EditCheatView
{
private:
	DualTextMenuItem ggCode{};
	GbcCheat *cheat{};
	MenuItem *item[3]{};

	void renamed(const char *str) override;
	void removed() override;

public:
	SystemEditCheatView(Base::Window &win);
	void init(GbcCheat &cheat);
};

extern SystemEditCheatView editCheatView;

class EditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addGGGS{};
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
