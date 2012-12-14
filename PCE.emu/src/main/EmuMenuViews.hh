#pragma once
#include "OptionView.hh"
#include <libgen.h>

static int pceHuFsFilter(const char *name, int type);

class SystemOptionView : public OptionView
{
private:

	BiosSelectMenu biosSelectMenu {&::sysCardPath, pceHuFsFilter};
	char sysCardPathStr[256] {0};
	TextMenuItem sysCardPath {""};

	template <size_t S>
	static void printBiosMenuEntryStr(char (&str)[S])
	{
		char basenameStr[S];
		strcpy(basenameStr, ::sysCardPath);
		string_printf(str, "System Card: %s", strlen(::sysCardPath) ? basename(basenameStr) : "None set");
	}

	void biosPathUpdated()
	{
		logMsg("set bios %s", ::sysCardPath);
		printBiosMenuEntryStr(sysCardPathStr);
		sysCardPath.compile();
	}

	void sysCardPathHandler(TextMenuItem &, const InputEvent &e)
	{
		biosSelectMenu.init(!e.isPointer());
		biosSelectMenu.placeRect(Gfx::viewportRect());
		biosSelectMenu.biosChangeDel.bind<SystemOptionView, &SystemOptionView::biosPathUpdated>(this);
		modalView = &biosSelectMenu;
		Base::displayNeedsUpdate();
	}

	BoolMenuItem arcadeCard {"Arcade Card"}, sixButtonPad {"6-button support"};

	static void arcadeCardHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionArcadeCard = item.on;
	}

	static void sixButtonPadHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		PCE_Fast::AVPad6Enabled[0] = item.on;
		PCE_Fast::AVPad6Enabled[1] = item.on;
		#ifdef INPUT_SUPPORTS_POINTER
		vController.gp.activeFaceBtns = item.on ? 6 : 2;
		vController.place();
		#endif
	}

public:
	constexpr SystemOptionView() { }

	void loadInputItems(MenuItem *item[], uint &items)
	{
		OptionView::loadInputItems(item, items);
		sixButtonPad.init(PCE_Fast::AVPad6Enabled[0]); item[items++] = &sixButtonPad;
		sixButtonPad.selectDelegate().bind<&sixButtonPadHandler>();
	}

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		arcadeCard.init(optionArcadeCard); item[items++] = &arcadeCard;
		arcadeCard.selectDelegate().bind<&arcadeCardHandler>();
		printBiosMenuEntryStr(sysCardPathStr);
		sysCardPath.init(sysCardPathStr); item[items++] = &sysCardPath;
		sysCardPath.selectDelegate().bind<SystemOptionView, &SystemOptionView::sysCardPathHandler>(this);
	}
};

#include "MenuView.hh"

class SystemMenuView : public MenuView
{
public:
	constexpr SystemMenuView() { }
};
