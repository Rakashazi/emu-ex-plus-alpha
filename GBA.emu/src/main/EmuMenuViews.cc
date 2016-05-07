#include <emuframework/OptionView.hh>
#include <emuframework/MenuView.hh>
#include "EmuCheatViews.hh"
#include "internal.hh"
#include <vbam/gba/GBA.h>
#include <vbam/gba/RTC.h>

class EmuSystemOptionView : public SystemOptionView
{
	TextMenuItem rtcItem[3]
	{
		{"Auto", [](){ setRTCEmulation(RTC_EMU_AUTO); }},
		{"Off", [](){ setRTCEmulation(0); }},
		{"On", [](){ setRTCEmulation(1); }},
	};

	MultiChoiceMenuItem rtc
	{
		"RTC Emulation",
		optionRtcEmulation,
		rtcItem
	};

	static void setRTCEmulation(uint val)
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
	EmuSystemOptionView(Base::Window &win): SystemOptionView{win, true}
	{
		loadStockItems();
		item.emplace_back(&rtc);
	}
};

View *EmuSystem::makeView(Base::Window &win, ViewID id)
{
	switch(id)
	{
		case ViewID::MAIN_MENU: return new MenuView(win);
		case ViewID::VIDEO_OPTIONS: return new VideoOptionView(win);
		case ViewID::AUDIO_OPTIONS: return new AudioOptionView(win);
		case ViewID::SYSTEM_OPTIONS: return new EmuSystemOptionView(win);
		case ViewID::GUI_OPTIONS: return new GUIOptionView(win);
		case ViewID::EDIT_CHEATS: return new EmuEditCheatListView(win);
		case ViewID::LIST_CHEATS: return new EmuCheatsView(win);
		default: return nullptr;
	}
}
