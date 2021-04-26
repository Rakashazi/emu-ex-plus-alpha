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
#include <utility>
#include <compare>

struct _XDisplay;

namespace Base
{

using NativeWindowFormat = void*;
using NativeWindow = unsigned long;

class XWindow : public BaseWindow
{
public:
	using BaseWindow::BaseWindow;
	~XWindow();
	std::pair<unsigned long, unsigned long> xdndData() const;
	explicit operator bool() const;
	void toggleFullScreen();

protected:
	_XDisplay *dpy{};
	unsigned long xWin{};
	unsigned long draggerXWin{};
	unsigned long dragAction{};
	IG_enableMemberIf(!Config::MACHINE_IS_PANDORA, unsigned long, colormap){};
};

using WindowImpl = XWindow;

}
