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

#include <stdlib.h>
#include <array>
#include <imagine/pixmap/Pixmap.hh>
#include <emuframework/EmuSystem.hh>

bool writeScreenshot(const IG::Pixmap &vidPix, const char *fname);

template <size_t S>
static int sprintScreenshotFilename(std::array<char, S> &str)
{
	const uint maxNum = 999;
	int num = -1;
	iterateTimes(maxNum, i)
	{
		string_printf(str, "%s/%s.%.3d.png", EmuSystem::savePath(), EmuSystem::gameName(), i);
		if(!FsSys::fileExists(str.data()))
		{
			num = i;
			break;
		}
	}
	if(num == -1)
	{
		logMsg("no screenshot filenames left");
		return -1;
	}
	logMsg("screenshot %d", num);
	return num;
}
