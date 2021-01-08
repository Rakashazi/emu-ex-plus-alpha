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
#include <imagine/time/Time.hh>
#include <utility>
#include <compare>

namespace Base
{

class XScreen
{
public:
	constexpr XScreen() {}
	XScreen(void *xScreen);
	std::pair<float, float> mmSize() const;
	void *nativeObject() const;
	bool operator ==(XScreen const &rhs) const;
	explicit operator bool() const;

protected:
	void *xScreen{};
	float xMM = 0, yMM = 0;
	IG::FloatSeconds frameTime_{};
	bool reliableFrameTime = true;
};

using ScreenImpl = XScreen;

}
