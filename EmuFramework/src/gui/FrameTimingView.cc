/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include "FrameTimingView.hh"
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppHelper.hh>
#include <emuframework/EmuViewController.hh>
#include <emuframework/viewUtils.hh>
#include <imagine/base/Screen.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <format>

namespace EmuEx
{

static std::string makeFrameRateStr(VideoSystem vidSys, const OutputTimingManager& mgr)
{
	auto opt = mgr.frameRateOption(vidSys);
	if(opt == OutputTimingManager::autoOption)
		return "Auto";
	else if(opt == OutputTimingManager::originalOption)
		return "Original";
	else
		return std::format("{:g}Hz", toHz(opt));
}

FrameTimingView::FrameTimingView(ViewAttachParams attach):
	TableView{"Frame Timing Options", attach, item},
	frameIntervalItem
	{
		{"Full (No Skip)", attach, {.id = 0}},
		{"Full",           attach, {.id = 1}},
		{"1/2",            attach, {.id = 2}},
		{"1/3",            attach, {.id = 3}},
		{"1/4",            attach, {.id = 4}},
	},
	frameInterval
	{
		"Frame Rate Target", attach,
		MenuId{app().frameInterval},
		frameIntervalItem,
		MultiChoiceMenuItem::Config
		{
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().frameInterval.setUnchecked(item.id); }
		},
	},
	frameRateItems
	{
		{"Auto (Match screen when rates are similar)", attach,
			[this]
			{
				if(!app().viewController().emuWindowScreen()->frameRateIsReliable())
				{
					app().postErrorMessage("Reported rate potentially unreliable, "
						"using the detected rate may give better results");
				}
				onFrameRateChange(activeVideoSystem, OutputTimingManager::autoOption);
			}, {.id = OutputTimingManager::autoOption.count()}
		},
		{"Original (Use emulated system's rate)", attach,
			[this]
			{
				onFrameRateChange(activeVideoSystem, OutputTimingManager::originalOption);
			}, {.id = OutputTimingManager::originalOption.count()}
		},
		{"Custom Rate", attach,
			[this](const Input::Event &e)
			{
				pushAndShowNewCollectValueInputView<std::pair<double, double>>(attachParams(), e,
					"Input decimal or fraction", "",
					[this](CollectTextInputView&, auto val)
					{
						if(onFrameRateChange(activeVideoSystem, fromSeconds<SteadyClockDuration>(val.second / val.first)))
						{
							if(activeVideoSystem == VideoSystem::NATIVE_NTSC)
								frameRate.setSelected(defaultMenuId, *this);
							else
								frameRatePAL.setSelected(defaultMenuId, *this);
							dismissPrevious();
							return true;
						}
						else
							return false;
					});
				return false;
			}, {.id = defaultMenuId}
		},
	},
	frameRate
	{
		"Frame Rate", attach,
		app().outputTimingManager.frameRateOptionAsMenuId(VideoSystem::NATIVE_NTSC),
		frameRateItems,
		{
			.onSetDisplayString = [this](auto, Gfx::Text& t)
			{
				t.resetString(makeFrameRateStr(VideoSystem::NATIVE_NTSC, app().outputTimingManager));
				return true;
			},
			.onSelect = [this](MultiChoiceMenuItem &item, View &view, const Input::Event &e)
			{
				activeVideoSystem = VideoSystem::NATIVE_NTSC;
				item.defaultOnSelect(view, e);
			},
		},
	},
	frameRatePAL
	{
		"Frame Rate (PAL)", attach,
		app().outputTimingManager.frameRateOptionAsMenuId(VideoSystem::PAL),
		frameRateItems,
		{
			.onSetDisplayString = [this](auto, Gfx::Text& t)
			{
				t.resetString(makeFrameRateStr(VideoSystem::PAL, app().outputTimingManager));
				return true;
			},
			.onSelect = [this](MultiChoiceMenuItem &item, View &view, const Input::Event &e)
			{
				activeVideoSystem = VideoSystem::PAL;
				item.defaultOnSelect(view, e);
			},
		},
	},
	frameTimingStats
	{
		"Show Frame Timing Stats", attach,
		app().showFrameTimingStats,
		[this](BoolMenuItem &item) { app().showFrameTimingStats = item.flipBoolValue(*this); }
	},
	frameClockItems
	{
		{"Auto",                                  attach, MenuItem::Config{.id = FrameClockSource::Unset}},
		{"Screen (Less latency & power use)",     attach, MenuItem::Config{.id = FrameClockSource::Screen}},
		{"Timer (Best for VRR displays)",         attach, MenuItem::Config{.id = FrameClockSource::Timer}},
		{"Renderer (May buffer multiple frames)", attach, MenuItem::Config{.id = FrameClockSource::Renderer}},
	},
	frameClock
	{
		"Frame Clock", attach,
		MenuId{FrameClockSource(app().frameClockSource)},
		frameClockItems,
		MultiChoiceMenuItem::Config
		{
			.onSetDisplayString = [this](auto, Gfx::Text& t)
			{
				t.resetString(wise_enum::to_string(app().effectiveFrameClockSource()));
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item)
			{
				app().frameClockSource = FrameClockSource(item.id.val);
				app().video.resetImage(); // texture can switch between single/double buffered
			}
		},
	},
	presentModeItems
	{
		{"Auto",                                                 attach, MenuItem::Config{.id = Gfx::PresentMode::Auto}},
		{"Immediate (Less compositor latency, may drop frames)", attach, MenuItem::Config{.id = Gfx::PresentMode::Immediate}},
		{"Queued (Better frame rate stability)",                 attach, MenuItem::Config{.id = Gfx::PresentMode::FIFO}},
	},
	presentMode
	{
		"Present Mode", attach,
		MenuId{Gfx::PresentMode(app().presentMode)},
		presentModeItems,
		MultiChoiceMenuItem::Config
		{
			.onSetDisplayString = [this](auto, Gfx::Text& t)
			{
				t.resetString(renderer().evalPresentMode(app().emuWindow(), app().presentMode) == Gfx::PresentMode::FIFO ? "Queued" : "Immediate");
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item)
			{
				app().presentMode = Gfx::PresentMode(item.id.val);
			}
		},
	},
	screenFrameRateItems
	{
		[&]
		{
			std::vector<TextMenuItem> items;
			auto setRateDel = [this](TextMenuItem& item) { app().overrideScreenFrameRate = std::bit_cast<float>(item.id); };
			items.emplace_back("Off", attach, setRateDel, MenuItem::Config{.id = 0});
			for(auto rate : app().emuScreen().supportedFrameRates())
				items.emplace_back(std::format("{:g}Hz", rate.hz()), attach, setRateDel, MenuItem::Config{.id = std::bit_cast<MenuId>(rate.hz())});
			return items;
		}()
	},
	screenFrameRate
	{
		"Override Screen Frame Rate", attach,
		std::bit_cast<MenuId>(float(app().overrideScreenFrameRate)),
		screenFrameRateItems
	},
	presentationTimeItems
	{
		{"Full (Apply to all frame rate targets)",         attach, MenuItem::Config{.id = PresentationTimeMode::full}},
		{"Basic (Only apply to lower frame rate targets)", attach, MenuItem::Config{.id = PresentationTimeMode::basic}},
		{"Off",                                            attach, MenuItem::Config{.id = PresentationTimeMode::off}},
	},
	presentationTime
	{
		"Precise Frame Pacing", attach,
		MenuId{PresentationTimeMode(app().presentationTimeMode)},
		presentationTimeItems,
		MultiChoiceMenuItem::Config
		{
			.onSetDisplayString = [this](auto, Gfx::Text& t)
			{
				if(app().presentationTimeMode == PresentationTimeMode::off)
					return false;
				t.resetString(app().presentationTimeMode == PresentationTimeMode::full ? "Full" : "Basic");
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item)
			{
				app().presentationTimeMode = PresentationTimeMode(item.id.val);
			}
		},
	},
	blankFrameInsertion
	{
		"Allow Blank Frame Insertion", attach,
		app().allowBlankFrameInsertion,
		[this](BoolMenuItem &item) { app().allowBlankFrameInsertion = item.flipBoolValue(*this); }
	},
	advancedHeading{"Advanced", attach}
{
	loadStockItems();
}

void FrameTimingView::loadStockItems()
{
	item.emplace_back(&frameInterval);
	item.emplace_back(&frameRate);
	if(EmuSystem::hasPALVideoSystem)
	{
		item.emplace_back(&frameRatePAL);
	}
	item.emplace_back(&frameTimingStats);
	item.emplace_back(&advancedHeading);
	item.emplace_back(&frameClock);
	if(used(presentMode))
		item.emplace_back(&presentMode);
	if(used(presentationTime) && renderer().supportsPresentationTime())
		item.emplace_back(&presentationTime);
	item.emplace_back(&blankFrameInsertion);
	if(used(screenFrameRate) && app().emuScreen().supportedFrameRates().size() > 1)
		item.emplace_back(&screenFrameRate);
}

bool FrameTimingView::onFrameRateChange(VideoSystem vidSys, SteadyClockDuration d)
{
	if(!app().outputTimingManager.setFrameRateOption(vidSys, d))
	{
		app().postMessage(4, true, std::format("{:g}Hz not in valid range", toHz(d)));
		return false;
	}
	return true;
}

}
