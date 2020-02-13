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
#include <imagine/base/BaseWindow.hh>
#include <imagine/util/operators.hh>
#include <utility>

namespace Base
{

struct NativeWindowFormat
{
	void *visual{};
	int depth{};
};

using NativeWindow = unsigned long;

class XWindow : public BaseWindow, public NotEquals<XWindow>
{
public:
	constexpr XWindow() {}
	std::pair<unsigned long, unsigned long> xdndData() const;
	bool operator ==(XWindow const &rhs) const;
	explicit operator bool() const;

protected:
	unsigned long xWin{};
	unsigned long draggerXWin{};
	unsigned long dragAction{};
	#ifndef CONFIG_MACHINE_PANDORA
	IG::Point2D<int> pos;
	unsigned long colormap{};
	#endif
};

void shutdownWindowSystem();

using WindowImpl = XWindow;

}
