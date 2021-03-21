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
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/gfx/GfxText.hh>
#include <imagine/base/Timer.hh>
#include <imagine/gui/View.hh>
#include <cstdio>
#include <system_error>
#include <array>

class ToastView : public View
{
public:
	ToastView();
	ToastView(ViewAttachParams attach);
	void setFace(Gfx::GlyphTextureSet &face);
	void clear();
	void place() final;
	void unpost();
	void post(const char *msg, int secs = 3, bool error = false);
	void postError(const char *msg, int secs = 3);
	void post(const char *prefix, const std::system_error &err, int secs = 3);
	void post(const char *prefix, std::error_code ec, int secs = 3);
	void prepareDraw() final;
	void draw(Gfx::RendererCommands &cmds) final;
	bool inputEvent(Input::Event event) final;

	[[gnu::format(printf, 4, 5)]]
	void printf(uint32_t secs, bool error, const char *format, ...);
	void vprintf(uint32_t secs, bool error, const char *format, va_list args);

private:
	Gfx::Text text{};
	Base::Timer unpostTimer{Base::Timer::NullInit{}};
	Gfx::GCRect msgFrame{};
	bool error = false;

	void contentUpdated(bool error);
	void postContent(int secs);
};
