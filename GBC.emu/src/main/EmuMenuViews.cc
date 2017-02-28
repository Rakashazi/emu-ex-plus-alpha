#include <emuframework/OptionView.hh>
#include <emuframework/MenuView.hh>
#include "EmuCheatViews.hh"
#include "internal.hh"
#include <resample/resamplerinfo.h>

static constexpr uint MAX_RESAMPLERS = 4;

class EmuAudioOptionView : public AudioOptionView
{
	StaticArrayList<TextMenuItem, MAX_RESAMPLERS> resamplerItem{};

	MultiChoiceMenuItem resampler
	{
		"Resampler",
		optionAudioResampler,
		resamplerItem
	};

public:
	EmuAudioOptionView(ViewAttachParams attach): AudioOptionView{attach, true}
	{
		loadStockItems();
		logMsg("%d resamplers", (int)ResamplerInfo::num());
		auto resamplers = std::min((uint)ResamplerInfo::num(), MAX_RESAMPLERS);
		iterateTimes(resamplers, i)
		{
			ResamplerInfo r = ResamplerInfo::get(i);
			logMsg("%d %s", i, r.desc);
			resamplerItem.emplace_back(r.desc,
				[i]()
				{
					optionAudioResampler = i;
					EmuSystem::configAudioPlayback();
				});
		}
		item.emplace_back(&resampler);
	}
};

class EmuVideoOptionView : public VideoOptionView
{
	TextMenuItem gbPaletteItem[13]
	{
		{"Original", [](){ optionGBPal = 0; applyGBPalette(); }},
		{"Brown", [](){ optionGBPal = 1; applyGBPalette(); }},
		{"Red", [](){ optionGBPal = 2; applyGBPalette(); }},
		{"Dark Brown", [](){ optionGBPal = 3; applyGBPalette(); }},
		{"Pastel", [](){ optionGBPal = 4; applyGBPalette(); }},
		{"Orange", [](){ optionGBPal = 5; applyGBPalette(); }},
		{"Yellow", [](){ optionGBPal = 6; applyGBPalette(); }},
		{"Blue", [](){ optionGBPal = 7; applyGBPalette(); }},
		{"Dark Blue", [](){ optionGBPal = 8; applyGBPalette(); }},
		{"Gray", [](){ optionGBPal = 9; applyGBPalette(); }},
		{"Green", [](){ optionGBPal = 10; applyGBPalette(); }},
		{"Dark Green", [](){ optionGBPal = 11; applyGBPalette(); }},
		{"Reverse", [](){ optionGBPal = 12; applyGBPalette(); }},
	};

	MultiChoiceMenuItem gbPalette
	{
		"GB Palette",
		optionGBPal,
		gbPaletteItem
	};

	BoolMenuItem useBuiltinGBPalette
	{
		"Use Built-in GB Palettes",
		(bool)optionUseBuiltinGBPalette,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionUseBuiltinGBPalette = item.flipBoolValue(*this);
			applyGBPalette();
		}
	};

	BoolMenuItem fullSaturation
	{
		"Saturated GBC Colors",
		(bool)optionFullGbcSaturation,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionFullGbcSaturation = item.flipBoolValue(*this);
			if(EmuSystem::gameIsRunning())
			{
				gbEmu.refreshPalettes();
			}
		}
	};

public:
	EmuVideoOptionView(ViewAttachParams attach): VideoOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&gbPalette);
		item.emplace_back(&useBuiltinGBPalette);
		item.emplace_back(&fullSaturation);
	}
};

class EmuSystemOptionView : public SystemOptionView
{
	BoolMenuItem reportAsGba
	{
		"Report Hardware as GBA",
		(bool)optionReportAsGba,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionReportAsGba = item.flipBoolValue(*this);
		}
	};

public:
	EmuSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&reportAsGba);
	}
};

View *EmuSystem::makeView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::MAIN_MENU: return new MenuView(attach);
		case ViewID::VIDEO_OPTIONS: return new EmuVideoOptionView(attach);
		case ViewID::AUDIO_OPTIONS: return new EmuAudioOptionView(attach);
		case ViewID::SYSTEM_OPTIONS: return new EmuSystemOptionView(attach);
		case ViewID::GUI_OPTIONS: return new GUIOptionView(attach);
		case ViewID::EDIT_CHEATS: return new EmuEditCheatListView(attach);
		case ViewID::LIST_CHEATS: return new EmuCheatsView(attach);
		default: return nullptr;
	}
}
