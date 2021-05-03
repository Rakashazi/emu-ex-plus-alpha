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

#include <imagine/gfx/Projection.hh>
#include <imagine/gfx/AnimatedViewport.hh>

struct WindowData
{
	Gfx::Viewport viewport() const { return projection.plane().viewport(); }
	Gfx::Projection projection{};
	Gfx::AnimatedViewport animatedViewport{};
	bool hasEmuView = false;
	bool hasPopup = false;
	bool focused = true;
};

WindowData &windowData(const Base::Window &win);
