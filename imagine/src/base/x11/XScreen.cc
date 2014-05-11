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

#define LOGTAG "XScreen"
#include <imagine/base/Screen.hh>
#include <imagine/util/time/sys.hh>
#include <imagine/logger/logger.h>
#include "x11.hh"
#include "GLContextHelper.hh"

namespace Base
{

void XScreen::init(::Screen *xScreen)
{
	assert(xScreen);
	var_selfs(xScreen);
	xMM = WidthMMOfScreen(xScreen);
	yMM = HeightMMOfScreen(xScreen);
	logMsg("X screen: 0x%p %dx%d (%dx%dmm)", xScreen,
		WidthOfScreen(xScreen), HeightOfScreen(xScreen), (int)xMM, (int)yMM);
}

void Screen::deinit() {}

int Screen::width()
{
	return WidthOfScreen(xScreen);
}

int Screen::height()
{
	return HeightOfScreen(xScreen);
}

uint Screen::refreshRate()
{
	auto conf = XRRGetScreenInfo(DisplayOfScreen(xScreen), RootWindowOfScreen(xScreen));
	auto rate = XRRConfigCurrentRate(conf);
	logMsg("refresh rate %d", (int)rate);
	return rate;
}

void Screen::setRefreshRate(uint rate)
{
	if(Config::MACHINE_IS_PANDORA)
	{
		if(rate == REFRESH_RATE_DEFAULT)
			rate = 60;
		if(rate != 50 && rate != 60)
		{
			logWarn("tried to set unsupported refresh rate: %u", rate);
		}
		char cmd[64];
		string_printf(cmd, "sudo /usr/pandora/scripts/op_lcdrate.sh %u", rate);
		int err = system(cmd);
		if(err)
		{
			logErr("error setting refresh rate, %d", err);
		}
	}
}

void Screen::swapsComplete()
{
	// update the frame time after a blocking double-buffered swap
	if(frameIsPosted())
		currFrameTime = TimeSys::now().toNs();
}

void Screen::postFrame()
{
	//logMsg("posting frame");
	framePosted = true;
}

void Screen::unpostFrame()
{
	framePosted = false;
	currFrameTime = 0;
}

int indexOfScreen(Screen &screen)
{
	iterateTimes(Screen::screens(), i)
	{
		if(*Screen::screen(i) == screen)
			return i;
	}
	bug_exit("screen wasn't in list");
	return 0;
}

}
