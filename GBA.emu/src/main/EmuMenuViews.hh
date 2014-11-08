#pragma once
#include <emuframework/OptionView.hh>
#include <emuframework/MenuView.hh>
#include "EmuCheatViews.hh"

class SystemOptionView : public OptionView
{
	MultiChoiceSelectMenuItem rtc
	{
		"RTC Emulation",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionRtcEmulation = val;

			if(detectedRtcGame && (uint)optionRtcEmulation == RTC_EMU_AUTO)
			{
				logMsg("automatically enabling RTC");
				rtcEnable(true);
			}
			else
			{
				logMsg("%s RTC", ((uint)optionRtcEmulation == RTC_EMU_ON) ? "enabled" : "disabled");
				rtcEnable((uint)optionRtcEmulation == RTC_EMU_ON);
			}
		}
	};

	void rtcInit()
	{
		static const char *str[] =
		{
			"Auto",
			"Off",
			"On"
		};
		rtc.init(str, optionRtcEmulation, sizeofArray(str));
	}

public:


public:
	SystemOptionView(Base::Window &win): OptionView(win) {}

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		rtcInit(); item[items++] = &rtc;
	}
};

class SystemMenuView : public MenuView
{
	TextMenuItem cheats
	{
		"Cheats",
		[this](TextMenuItem &item, View &, const Input::Event &e)
		{
			if(EmuSystem::gameIsRunning())
			{
				auto &cheatsMenu = *new CheatsView{window()};
				cheatsMenu.init(!e.isPointer());
				viewStack.pushAndShow(cheatsMenu);
			}
		}
	};

public:
	SystemMenuView(Base::Window &win): MenuView{win} {}

	void onShow()
	{
		MenuView::onShow();
		cheats.active = EmuSystem::gameIsRunning();
	}

	void init(bool highlightFirst)
	{
		name_ = appViewTitle();
		uint items = 0;
		loadFileBrowserItems(item, items);
		cheats.init(); item[items++] = &cheats;
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		TableView::init(item, items, highlightFirst);
	}
};
