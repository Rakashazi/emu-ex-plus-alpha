#pragma once
#include "OptionView.hh"
#include <libgen.h>

class SystemOptionView : public OptionView
{
private:

	BiosSelectMenu biosSelectMenu {&::biosPath, ssBiosFsFilter};
	char biosPathStr[256] {0};
	TextMenuItem biosPath {""};

	template <size_t S>
	static void printBiosMenuEntryStr(char (&str)[S])
	{
		char basenameStr[S];
		strcpy(basenameStr, ::biosPath);
		string_printf(str, "BIOS: %s", strlen(::biosPath) ? basename(basenameStr) : "None set");
	}

	void biosPathUpdated()
	{
		logMsg("set bios %s", ::biosPath);
		printBiosMenuEntryStr(biosPathStr);
		biosPath.compile();
	}

	void biosPathHandler(TextMenuItem &, const Input::Event &e)
	{
		biosSelectMenu.init(!e.isPointer());
		biosSelectMenu.placeRect(Gfx::viewportRect());
		biosSelectMenu.biosChangeDel.bind<SystemOptionView, &SystemOptionView::biosPathUpdated>(this);
		modalView = &biosSelectMenu;
		Base::displayNeedsUpdate();
	}

	MultiChoiceSelectMenuItem sh2Core {"SH2"};

	void sh2CoreInit()
	{
		static const char *str[6];

		int setting = 0, cores = 0;
		iterateTimes(sizeofArray(SH2CoreList)-1, i)
		{
			if(i == sizeofArray(str))
				break;
			str[i] = SH2CoreList[i]->Name;
			if(SH2CoreList[i]->id == yinit.sh2coretype)
				setting = i;
			cores++;
		}

		sh2Core.init(str, setting, cores);
		sh2Core.valueDelegate().bind<&sh2CoreSet>();
	}

	static void sh2CoreSet(MultiChoiceMenuItem &, int val)
	{
		assert(val < (int)sizeofArray(SH2CoreList)-1);
		yinit.sh2coretype = SH2CoreList[val]->id;
	}

public:
	constexpr SystemOptionView() { }

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		if(sizeofArray(SH2CoreList) > 2)
		{
			sh2CoreInit(); item[items++] = &sh2Core;
		}
		printBiosMenuEntryStr(biosPathStr);
		biosPath.init(biosPathStr); item[items++] = &biosPath;
		biosPath.selectDelegate().bind<SystemOptionView, &SystemOptionView::biosPathHandler>(this);
	}
};

#include "MenuView.hh"

class SystemMenuView : public MenuView
{
public:
	constexpr SystemMenuView() { }
};
