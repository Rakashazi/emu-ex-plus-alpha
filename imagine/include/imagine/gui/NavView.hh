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

#include <cstddef>
#include <memory>
#include <imagine/config/defs.hh>
#include <imagine/gfx/GfxText.hh>
#include <imagine/gfx/GeomRect.hh>
#include <imagine/gfx/GfxLGradient.hh>
#include <imagine/gfx/Texture.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/input/Input.hh>

class NavView
{
public:
	using OnPushDelegate = DelegateFunc<void (Input::Event e)>;

	NavView(Gfx::GlyphTextureSet *face);
	virtual ~NavView() {}
	void setOnPushLeftBtn(OnPushDelegate del);
	void setOnPushRightBtn(OnPushDelegate del);
	void setOnPushMiddleBtn(OnPushDelegate del);
	void setTitle(const char *title) { text.setString(title); }
	virtual void place(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP);
	bool inputEvent(Input::Event e);
	virtual void draw(Gfx::Renderer &r, const Base::Window &win, const Gfx::ProjectionPlane &projP) = 0;
	virtual void showLeftBtn(bool show) = 0;
	virtual void showRightBtn(bool show) = 0;
	IG::WindowRect &viewRect();
	Gfx::GlyphTextureSet *titleFace();

protected:
	IG::WindowRect leftBtn{}, rightBtn{}, textRect{};
	Gfx::Text text{};
	IG::WindowRect viewRect_{};
	OnPushDelegate onPushLeftBtn_{};
	OnPushDelegate onPushRightBtn_{};
	OnPushDelegate onPushMiddleBtn_{};
	bool hasBackBtn = false;
	bool hasCloseBtn = false;
};

class BasicNavView : public NavView
{
public:
	bool centerTitle = true;
	bool rotateLeftBtn = false;

	BasicNavView(Gfx::Renderer &r, Gfx::GlyphTextureSet *face, Gfx::PixmapTexture *leftRes, Gfx::PixmapTexture *rightRes);
	void setBackImage(Gfx::Renderer &r, Gfx::PixmapTexture *img);
	void setBackgroundGradient(const Gfx::LGradientStopDesc *gradStop, uint gradStops);

	template <size_t S>
	void setBackgroundGradient(const Gfx::LGradientStopDesc (&gradStop)[S])
	{
		setBackgroundGradient(gradStop, S);
	}

	void draw(Gfx::Renderer &r, const Base::Window &win, const Gfx::ProjectionPlane &projP) override;
	void place(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP) override;
	void showLeftBtn(bool show) override;
	void showRightBtn(bool show) override;

protected:
	Gfx::Sprite leftSpr{}, rightSpr{};
	Gfx::LGradient bg{};
	std::unique_ptr<Gfx::LGradientStopDesc[]> gradientStops{};
};
