/*  This file is part of GBC.emu.

	GBC.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	GBC.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with GBC.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppHelper.hh>
#include <emuframework/OptionView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#include "EmuCheatViews.hh"
#include "internal.hh"
#include <resample/resamplerinfo.h>

static constexpr unsigned MAX_RESAMPLERS = 4;

class CustomAudioOptionView : public AudioOptionView
{
	StaticArrayList<TextMenuItem, MAX_RESAMPLERS> resamplerItem{};

	MultiChoiceMenuItem resampler
	{
		"Resampler", &defaultFace(),
		optionAudioResampler,
		resamplerItem
	};

public:
	CustomAudioOptionView(ViewAttachParams attach): AudioOptionView{attach, true}
	{
		loadStockItems();
		logMsg("%d resamplers", (int)ResamplerInfo::num());
		auto resamplers = std::min((unsigned)ResamplerInfo::num(), MAX_RESAMPLERS);
		iterateTimes(resamplers, i)
		{
			ResamplerInfo r = ResamplerInfo::get(i);
			logMsg("%d %s", i, r.desc);
			resamplerItem.emplace_back(r.desc, &defaultFace(),
				[this, i]()
				{
					optionAudioResampler = i;
					app().configFrameTime();
				});
		}
		item.emplace_back(&resampler);
	}
};

class CustomVideoOptionView : public VideoOptionView
{
	TextMenuItem gbPaletteItem[13]
	{
		{"Original", &defaultFace(), [](){ optionGBPal = 0; applyGBPalette(); }},
		{"Brown", &defaultFace(), [](){ optionGBPal = 1; applyGBPalette(); }},
		{"Red", &defaultFace(), [](){ optionGBPal = 2; applyGBPalette(); }},
		{"Dark Brown", &defaultFace(), [](){ optionGBPal = 3; applyGBPalette(); }},
		{"Pastel", &defaultFace(), [](){ optionGBPal = 4; applyGBPalette(); }},
		{"Orange", &defaultFace(), [](){ optionGBPal = 5; applyGBPalette(); }},
		{"Yellow", &defaultFace(), [](){ optionGBPal = 6; applyGBPalette(); }},
		{"Blue", &defaultFace(), [](){ optionGBPal = 7; applyGBPalette(); }},
		{"Dark Blue", &defaultFace(), [](){ optionGBPal = 8; applyGBPalette(); }},
		{"Gray", &defaultFace(), [](){ optionGBPal = 9; applyGBPalette(); }},
		{"Green", &defaultFace(), [](){ optionGBPal = 10; applyGBPalette(); }},
		{"Dark Green", &defaultFace(), [](){ optionGBPal = 11; applyGBPalette(); }},
		{"Reverse", &defaultFace(), [](){ optionGBPal = 12; applyGBPalette(); }},
	};

	MultiChoiceMenuItem gbPalette
	{
		"GB Palette", &defaultFace(),
		optionGBPal,
		gbPaletteItem
	};

	BoolMenuItem fullSaturation
	{
		"Saturated GBC Colors", &defaultFace(),
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
	CustomVideoOptionView(ViewAttachParams attach): VideoOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&systemSpecificHeading);
		item.emplace_back(&gbPalette);
		item.emplace_back(&fullSaturation);
	}
};

class ConsoleOptionView : public TableView, public EmuAppHelper<ConsoleOptionView>
{
	BoolMenuItem useBuiltinGBPalette
	{
		"Use Built-in GB Palettes", &defaultFace(),
		(bool)optionUseBuiltinGBPalette,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			EmuSystem::sessionOptionSet();
			optionUseBuiltinGBPalette = item.flipBoolValue(*this);
			applyGBPalette();
		}
	};

	BoolMenuItem reportAsGba
	{
		"Report Hardware as GBA", &defaultFace(),
		(bool)optionReportAsGba,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			EmuSystem::sessionOptionSet();
			optionReportAsGba = item.flipBoolValue(*this);
			app().promptSystemReloadDueToSetOption(attachParams(), e);
		}
	};

	std::array<MenuItem*, 2> menuItem
	{
		&useBuiltinGBPalette,
		&reportAsGba
	};

public:
	ConsoleOptionView(ViewAttachParams attach):
		TableView
		{
			"Console Options",
			attach,
			menuItem
		}
	{}
};

class CustomSystemActionsView : public EmuSystemActionsView
{
	TextMenuItem options
	{
		"Console Options", &defaultFace(),
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
			{
				pushAndShow(makeView<ConsoleOptionView>(), e);
			}
		}
	};

public:
	CustomSystemActionsView(ViewAttachParams attach): EmuSystemActionsView{attach, true}
	{
		item.emplace_back(&options);
		loadStandardItems();
	}
};

std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::VIDEO_OPTIONS: return std::make_unique<CustomVideoOptionView>(attach);
		case ViewID::AUDIO_OPTIONS: return std::make_unique<CustomAudioOptionView>(attach);
		case ViewID::SYSTEM_ACTIONS: return std::make_unique<CustomSystemActionsView>(attach);
		case ViewID::EDIT_CHEATS: return std::make_unique<EmuEditCheatListView>(attach);
		case ViewID::LIST_CHEATS: return std::make_unique<EmuCheatsView>(attach);
		default: return nullptr;
	}
}
