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
#include <imagine/util/operators.hh>

namespace Base
{

class XScreen : public NotEquals<XScreen>
{
public:
	void *xScreen{};
	float xMM = 0, yMM = 0;
	double frameTime_ = 0;
	bool reliableFrameTime = true;

	constexpr XScreen() {}

	void init(void *xScreen);

	bool operator ==(XScreen const &rhs) const
	{
		return xScreen == rhs.xScreen;
	}

	explicit operator bool() const
	{
		return xScreen;
	}
};

using ScreenImpl = XScreen;

}
