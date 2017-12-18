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
#include <imagine/gui/View.hh>

class NavView : public View
{
public:
	using OnPushDelegate = DelegateFunc<void (Input::Event e)>;

	NavView(ViewAttachParams attach, Gfx::GlyphTextureSet *face);
	void setOnPushLeftBtn(OnPushDelegate del);
	void setOnPushRightBtn(OnPushDelegate del);
	void setOnPushMiddleBtn(OnPushDelegate del);
	void setTitle(const char *title) { text.setString(title); }
	void place() override;
	bool inputEvent(Input::Event e) override;
	void clearSelection() override;
	void onAddedToController(Input::Event e) override;
	virtual void showLeftBtn(bool show) = 0;
	virtual void showRightBtn(bool show) = 0;
	IG::WindowRect &viewRect() override;
	Gfx::GlyphTextureSet *titleFace();
	bool hasButtons() const;

protected:
	IG::WindowRect leftBtn{}, rightBtn{}, textRect{};
	Gfx::Text text{};
	IG::WindowRect viewRect_{};
	OnPushDelegate onPushLeftBtn_{};
	OnPushDelegate onPushRightBtn_{};
	OnPushDelegate onPushMiddleBtn_{};
	int selected = -1;
	bool hasBackBtn = false;
	bool hasCloseBtn = false;

	bool selectNextLeftButton();
	bool selectNextRightButton();
};

class BasicNavView : public NavView
{
public:
	bool centerTitle = true;
	bool rotateLeftBtn = false;

	BasicNavView(ViewAttachParams attach, Gfx::GlyphTextureSet *face, Gfx::PixmapTexture *leftRes, Gfx::PixmapTexture *rightRes);
	void setBackImage(Gfx::PixmapTexture *img);
	void setBackgroundGradient(const Gfx::LGradientStopDesc *gradStop, uint gradStops);

	template <size_t S>
	void setBackgroundGradient(const Gfx::LGradientStopDesc (&gradStop)[S])
	{
		setBackgroundGradient(gradStop, S);
	}

	void draw() override;
	void place() override;
	void showLeftBtn(bool show) override;
	void showRightBtn(bool show) override;

protected:
	Gfx::Sprite leftSpr{}, rightSpr{};
	Gfx::LGradient bg{};
	std::unique_ptr<Gfx::LGradientStopDesc[]> gradientStops{};
};
