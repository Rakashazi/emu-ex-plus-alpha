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

#include <imagine/gfx/AnimatedViewport.hh>
#include <imagine/gfx/ProjectionPlane.hh>
#include <emuframework/EmuSystem.hh>
#include "Recent.hh"
#include <memory>

namespace Base
{
class Window;
}

namespace Input
{
class Event;
}

class ViewAttachParams;
class EmuSystemTask;
class EmuViewController;
class EmuVideo;
class EmuAudio;

struct WindowData
{
	Gfx::Viewport viewport() const { return projection.plane().viewport(); }
	Gfx::Projection projection{};
	Gfx::AnimatedViewport animatedViewport{};
	bool hasEmuView = false;
	bool hasPopup = false;
	bool focused = true;
};

extern RecentGameList recentGameList;
static constexpr const char *strftimeFormat = "%x  %r";

void addRecentGame(const char *fullPath, const char *name);
const char *appViewTitle();
bool hasGooglePlayStoreFeatures();
void runBenchmarkOneShot(EmuApp &, EmuVideo &);
void onSelectFileFromPicker(EmuApp &, const char* name, Input::Event e, EmuSystemCreateParams params, ViewAttachParams attachParams);
void launchSystem(EmuApp &, bool tryAutoState, bool addToRecent);
Gfx::Viewport makeViewport(const Base::Window &win);
Gfx::Projection updateProjection(Gfx::Viewport viewport);
WindowData &windowData(const Base::Window &win);
uint8_t currentFrameInterval();
IG::PixelFormatID optionImageEffectPixelFormatValue();

static void addRecentGame()
{
	addRecentGame(EmuSystem::fullGamePath(), EmuSystem::fullGameName().data());
}
