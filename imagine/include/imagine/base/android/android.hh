#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/config/defs.hh>
#include <imagine/fs/FSDefs.hh>
#include <android/api-level.h>
#include <array>

namespace Base
{

using AndroidPropString = std::array<char, 92>;

uint32_t androidSDK();
void setProcessPriority(int nice);
int processPriority();
bool apkSignatureIsConsistent();
AndroidPropString androidBuildDevice();
bool packageIsInstalled(const char *name);
FS::PathString mainSOPath();
void setSustainedPerformanceMode(bool on);

}

namespace AudioManager
{

uint32_t nativeOutputFramesPerBuffer();
bool hasLowLatency();

}

namespace Input
{

void initMOGA(bool notify);
void deinitMOGA();
bool mogaSystemIsActive();
void enumDevices();
bool hasTrackball();

}
