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

#import <QuartzCore/CVDisplayLink.h>

namespace IG
{

class CocoaScreen
{
public:
	CVDisplayLinkRef displayLink{}; // CADisplayLink in ObjC
	uint64_t hostFrameTime = 0;

	constexpr CocoaScreen() {}

	bool operator ==(CocoaScreen const &rhs) const
	{
		//TODO
		return displayLink == rhs.displayLink;
	}

	explicit operator bool() const
	{
		//TODO
		return displayLink;
	}

	void init();
};

using ScreenImpl = CocoaScreen;

}
