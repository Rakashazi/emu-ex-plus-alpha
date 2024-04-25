#pragma once

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

#include <emuframework/config.hh>
#include <emuframework/EmuAppHelper.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/util/container/ArrayList.hh>

namespace EmuEx
{

using namespace IG;
class EmuVideoLayer;
class EmuVideo;
enum class VideoSystem: uint8_t;

class FrameTimingView : public TableView, public EmuAppHelper
{
public:
	FrameTimingView(ViewAttachParams attach);
	void loadStockItems();

protected:
	static constexpr int MAX_ASPECT_RATIO_ITEMS = 5;
	TextMenuItem frameIntervalItem[5];
	MultiChoiceMenuItem frameInterval;
	TextMenuItem frameRateItems[4];
	VideoSystem activeVideoSystem{};
	MultiChoiceMenuItem frameRate;
	MultiChoiceMenuItem frameRatePAL;
	ConditionalMember<enableFrameTimeStats, BoolMenuItem> frameTimeStats;
	TextMenuItem frameClockItems[4];
	MultiChoiceMenuItem frameClock;
	ConditionalMember<Gfx::supportsPresentModes, TextMenuItem> presentModeItems[3];
	ConditionalMember<Gfx::supportsPresentModes, MultiChoiceMenuItem> presentMode;
	ConditionalMember<Config::multipleScreenFrameRates, std::vector<TextMenuItem>> screenFrameRateItems;
	ConditionalMember<Config::multipleScreenFrameRates, MultiChoiceMenuItem> screenFrameRate;
	ConditionalMember<Gfx::supportsPresentationTime, TextMenuItem> presentationTimeItems[3];
	ConditionalMember<Gfx::supportsPresentationTime, MultiChoiceMenuItem> presentationTime;
	BoolMenuItem blankFrameInsertion;
	TextHeadingMenuItem advancedHeading;
	StaticArrayList<MenuItem*, 10> item;

	bool onFrameTimeChange(VideoSystem vidSys, SteadyClockTime time);
};

}
