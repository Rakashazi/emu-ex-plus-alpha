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
#include <emuframework/OptionView.hh>
#include <emuframework/AudioOptionView.hh>
#include <emuframework/VideoOptionView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#include "EmuCheatViews.hh"
#include "Palette.hh"
#include "MainApp.hh"
#include <resample/resamplerinfo.h>

namespace EmuEx
{

template <class T>
using MainAppHelper = EmuAppHelper<T, MainApp>;

static constexpr size_t MAX_RESAMPLERS = 4;

class CustomAudioOptionView : public AudioOptionView, public MainAppHelper<CustomAudioOptionView>
{
	using MainAppHelper<CustomAudioOptionView>::app;
	using MainAppHelper<CustomAudioOptionView>::system;

	StaticArrayList<TextMenuItem, MAX_RESAMPLERS> resamplerItem;

	MultiChoiceMenuItem resampler
	{
		"Resampler", &defaultFace(),
		system().optionAudioResampler.val,
		resamplerItem
	};

public:
	CustomAudioOptionView(ViewAttachParams attach): AudioOptionView{attach, true}
	{
		loadStockItems();
		logMsg("%d resamplers", (int)ResamplerInfo::num());
		auto resamplers = std::min(ResamplerInfo::num(), MAX_RESAMPLERS);
		for(auto i : iotaCount(resamplers))
		{
			ResamplerInfo r = ResamplerInfo::get(i);
			logMsg("%zu %s", i, r.desc);
			resamplerItem.emplace_back(r.desc, &defaultFace(),
				[this, i]()
				{
					system().optionAudioResampler = i;
					app().configFrameTime();
				});
		}
		item.emplace_back(&resampler);
	}
};

class CustomVideoOptionView : public VideoOptionView, public MainAppHelper<CustomVideoOptionView>
{
	using MainAppHelper<CustomVideoOptionView>::system;

	TextMenuItem::SelectDelegate setGbPaletteDel()
	{
		return [this](TextMenuItem &item)
		{
			system().optionGBPal = item.id();
			system().applyGBPalette();
		};
	}

	TextMenuItem gbPaletteItem[13]
	{
		{"Original",   &defaultFace(), setGbPaletteDel(), 0},
		{"Brown",      &defaultFace(), setGbPaletteDel(), 1},
		{"Red",        &defaultFace(), setGbPaletteDel(), 2},
		{"Dark Brown", &defaultFace(), setGbPaletteDel(), 3},
		{"Pastel",     &defaultFace(), setGbPaletteDel(), 4},
		{"Orange",     &defaultFace(), setGbPaletteDel(), 5},
		{"Yellow",     &defaultFace(), setGbPaletteDel(), 6},
		{"Blue",       &defaultFace(), setGbPaletteDel(), 7},
		{"Dark Blue",  &defaultFace(), setGbPaletteDel(), 8},
		{"Gray",       &defaultFace(), setGbPaletteDel(), 9},
		{"Green",      &defaultFace(), setGbPaletteDel(), 10},
		{"Dark Green", &defaultFace(), setGbPaletteDel(), 11},
		{"Reverse",    &defaultFace(), setGbPaletteDel(), 12},
	};

	MultiChoiceMenuItem gbPalette
	{
		"GB Palette", &defaultFace(),
		system().optionGBPal.val,
		gbPaletteItem
	};

	BoolMenuItem fullSaturation
	{
		"Saturated GBC Colors", &defaultFace(),
		(bool)system().optionFullGbcSaturation,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().optionFullGbcSaturation = item.flipBoolValue(*this);
			if(system().hasContent())
			{
				system().refreshPalettes();
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

class ConsoleOptionView : public TableView, public MainAppHelper<ConsoleOptionView>
{
	BoolMenuItem useBuiltinGBPalette
	{
		"Use Built-in GB Palettes", &defaultFace(),
		(bool)system().optionUseBuiltinGBPalette,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().sessionOptionSet();
			system().optionUseBuiltinGBPalette = item.flipBoolValue(*this);
			system().applyGBPalette();
		}
	};

	BoolMenuItem reportAsGba
	{
		"Report Hardware as GBA", &defaultFace(),
		(bool)system().optionReportAsGba,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().sessionOptionSet();
			system().optionReportAsGba = item.flipBoolValue(*this);
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
			if(system().hasContent())
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

}
