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

struct xcb_connection_t;

namespace IG
{

using NativeWindowFormat = uint32_t;
using NativeWindow = uint32_t;

class XWindow : public BaseWindow
{
public:
	using BaseWindow::BaseWindow;
	~XWindow();
	std::pair<uint32_t, uint32_t> xdndData() const;
	explicit operator bool() const;

protected:
	xcb_connection_t* xConn{};
	uint32_t xWin{};
	uint32_t draggerXWin{};
	uint32_t dragAction{};
	uint32_t colormap{};
public:
	bool shouldBypassCompositorState{};
};

using WindowImpl = XWindow;

}
