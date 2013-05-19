#pragma once
#include <OptionView.hh>

class SystemOptionView : public OptionView
{
	MultiChoiceSelectMenuItem rtc
	{
		"RTC Emulation",
		[](MultiChoiceMenuItem &, int val)
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
	SystemOptionView() { }

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		rtcInit(); item[items++] = &rtc;
	}
};

#include "EmuCheatViews.hh"
#include "MenuView.hh"

class SystemMenuView : public MenuView
{
	TextMenuItem cheats
	{
		"Cheats",
		[](TextMenuItem &item, const Input::Event &e)
		{
			if(EmuSystem::gameIsRunning())
			{
				cheatsMenu.init(!e.isPointer());
				viewStack.pushAndShow(&cheatsMenu);
			}
		}
	};

public:
	SystemMenuView() {}

	void onShow()
	{
		MenuView::onShow();
		cheats.active = EmuSystem::gameIsRunning();
	}

	void init(bool highlightFirst)
	{
		uint items = 0;
		loadFileBrowserItems(item, items);
		cheats.init(); item[items++] = &cheats;
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		BaseMenuView::init(item, items, highlightFirst);
	}
};
