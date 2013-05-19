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
#include <pixmap/Pixmap.hh>
#include <EmuSystem.hh>

bool writeScreenshot(const Pixmap &vidPix, const char *fname);

template <size_t S>
static int sprintScreenshotFilename(char (&str)[S])
{
	const uint maxNum = 999;
	int num = -1;
	iterateTimes(maxNum, i)
	{
		snprintf(str, S, "%s/%s.%.3d.png", EmuSystem::savePath(), EmuSystem::gameName, i);
		if(!FsSys::fileExists(str))
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
