#pragma once
#include <OptionView.hh>

class SystemOptionView : public OptionView
{
	MultiChoiceSelectMenuItem rtc {"RTC Emulation", MultiChoiceMenuItem::ValueDelegate::create<&rtcSet>()};

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

	static void rtcSet(MultiChoiceMenuItem &, int val)
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

public:
	constexpr SystemOptionView() { }

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		rtcInit(); item[items++] = &rtc;
	}
};

class SystemMenuView : public MenuView
{
public:
	constexpr SystemMenuView() { }
};
