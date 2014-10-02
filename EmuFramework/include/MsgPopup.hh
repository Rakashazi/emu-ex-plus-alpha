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

#include <cstdio>
#include <imagine/gfx/GfxText.hh>
#include <imagine/gfx/GeomRect.hh>
#include <imagine/base/Timer.hh>

class MsgPopup
{
private:
	Gfx::Text text;
	Gfx::ProjectionPlane projP;
	Base::Timer unpostTimer;
	bool error = 0;
	std::array<char, 1024> str{};

public:
	MsgPopup() {}
	void init();
	void clear();
	void place(const Gfx::ProjectionPlane &projP);
	void unpost();
	void post(const char *msg, int secs = 3, bool error = false);
	void postError(const char *msg, int secs = 3);
	void draw();

	template <typename... ARGS>
	void printf(uint secs, bool error, const char *format, ARGS&&... args)
	{
		snprintf(str.data(), sizeof(str), format, std::forward<ARGS>(args)...);
		//logMsg("%s", str);
		post(str.data(), secs, error);
	}
};
