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

#include <emuframework/EmuSystem.hh>
#include <imagine/pixmap/PixelFormat.hh>
#include <memory>

namespace Base
{
class Window;
}

namespace Input
{
class Event;
}

namespace Gfx
{
class Viewport;
class Projection;
}

class GenericIO;
class ViewAttachParams;
class EmuSystemTask;
class EmuViewController;
class EmuVideo;
class EmuAudio;

const char *appViewTitle();
bool hasGooglePlayStoreFeatures();
void runBenchmarkOneShot(EmuApp &, EmuVideo &);
void onSelectFileFromPicker(EmuApp &, GenericIO, IG::CStringView path, bool pathIsUri,
	Input::Event, EmuSystemCreateParams, ViewAttachParams);
void launchSystem(EmuApp &, bool tryAutoState);
Gfx::Viewport makeViewport(const Base::Window &win);
Gfx::Projection updateProjection(Gfx::Viewport viewport);
uint8_t currentFrameInterval();
IG::PixelFormatID optionImageEffectPixelFormatValue();
