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
#include <imagine/gfx/Quads.hh>
#include <cstdio>
#include <array>

namespace IG
{

class ToastView : public View
{
public:
	ToastView(ViewAttachParams attach);
	void setFace(Gfx::GlyphTextureSet &face);
	void clear();
	void place() final;

	void post(UTF16Convertible auto &&msg, int secs, bool error)
	{
		text.resetString(IG_forward(msg));
		place();
		this->error = error;
		postContent(secs);
	}

	void postError(UTF16Convertible auto &&msg, int secs) { post(IG_forward(msg), secs, true); }
	void unpost();
	void prepareDraw() final;
	void draw(Gfx::RendererCommands &__restrict__, ViewDrawParams p = {}) const final;

private:
	Gfx::Text text;
	Timer unpostTimer;
	Gfx::IQuads msgFrameQuads;
	WRect msgFrame{};
	bool error = false;

	void postContent(int secs);
};

}
