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
#include <system_error>
#include <imagine/gfx/GfxText.hh>
#include <imagine/gfx/GeomRect.hh>
#include <imagine/base/Timer.hh>



class MsgPopup
{
private:
	Gfx::Renderer &r;
	Gfx::Text text{};
	Gfx::ProjectionPlane projP{};
	Base::Timer unpostTimer{};
	bool error = false;
	std::array<char, 1024> str{};

	void postContent(int secs, bool error);

public:
	MsgPopup(Gfx::Renderer &r);
	void setFace(Gfx::GlyphTextureSet &face);
	void clear();
	void place(const Gfx::ProjectionPlane &projP);
	void unpost();
	void post(const char *msg, int secs = 3, bool error = false);
	void postError(const char *msg, int secs = 3);
	void post(const char *prefix, const std::system_error &err, int secs = 3);
	void post(const char *prefix, std::error_code ec, int secs = 3);
	void prepareDraw();
	void draw(Gfx::RendererCommands &cmds);

	[[gnu::format(printf, 4, 5)]]
	void printf(uint secs, bool error, const char *format, ...);
	void vprintf(uint secs, bool error, const char *format, va_list args);
};
