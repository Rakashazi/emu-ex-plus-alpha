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

#pragma once

#include <gfx/GfxText.hh>
#include <gfx/GeomRect.hh>
#include <gfx/GfxLGradient.hh>
#include <gfx/GfxBufferImage.hh>
#include <util/rectangle2.h>
#include <input/Input.hh>

class NavView
{
public:
	constexpr NavView() { }

	virtual void onLeftNavBtn(const Input::Event &e) {};
	virtual void onRightNavBtn(const Input::Event &e) {};

	Rect2<int> leftBtn, rightBtn, textRect;
	Gfx::Text text;
	Rect2<int> viewRect;
	bool hasBackBtn = 0, leftBtnActive = 0, hasCloseBtn = 0, rightBtnActive = 0;

	void setLeftBtnActive(bool on) { leftBtnActive = on; }
	void setRightBtnActive(bool on) { rightBtnActive = on; }
	void setTitle(const char *title) { text.setString(title); }

	void init(ResourceFace *face);
	virtual void deinit() = 0;
	void deinitText();
	virtual void place();
	void inputEvent(const Input::Event &e);
	virtual void draw() = 0;
};

class BasicNavView : public NavView
{
public:
	constexpr BasicNavView() { }
	Gfx::Sprite leftSpr, rightSpr;
	Gfx::LGradient bg;
	void init(ResourceFace *face, Gfx::BufferImage *leftRes, Gfx::BufferImage *rightRes,
			const Gfx::LGradientStopDesc *gradStop, uint gradStops);
	void setBackImage(Gfx::BufferImage *img);
	void draw() override;
	void place() override;
	void deinit() override;
};
