#include <emuframework/OptionView.hh>
#include <emuframework/EmuMainMenuView.hh>
#include "EmuCheatViews.hh"
#include "internal.hh"
#include <vbam/gba/GBA.h>
#include <vbam/gba/RTC.h>

class CustomSystemOptionView : public SystemOptionView
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
	CustomSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&rtc);
	}
};

View *EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::SYSTEM_OPTIONS: return new CustomSystemOptionView(attach);
		case ViewID::EDIT_CHEATS: return new EmuEditCheatListView(attach);
		case ViewID::LIST_CHEATS: return new EmuCheatsView(attach);
		default: return nullptr;
	}
}
