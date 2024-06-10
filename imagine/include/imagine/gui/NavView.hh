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

#include <imagine/config/defs.hh>
#include <imagine/gfx/GfxText.hh>
#include <imagine/gfx/GfxLGradient.hh>
#include <imagine/gfx/Quads.hh>
#include <imagine/gui/View.hh>
#include <imagine/util/memory/DynArray.hh>
#include <memory>
#include <array>
#include <span>

namespace IG
{

class NavView : public View
{
public:
	using OnPushDelegate = DelegateFunc<void (const Input::Event &)>;

	NavView(ViewAttachParams attach, Gfx::GlyphTextureSet *face);
	void setOnPushLeftBtn(OnPushDelegate del);
	void setOnPushRightBtn(OnPushDelegate del);
	void setOnPushMiddleBtn(OnPushDelegate del);
	void setTitle(UTF16Convertible auto &&title) { text.resetString(IG_forward(title)); }
	void prepareDraw() override;
	void place() override;
	bool inputEvent(const Input::Event&, ViewInputEventParams p = {}) override;
	void clearSelection() override;
	virtual void showLeftBtn(bool show) = 0;
	virtual void showRightBtn(bool show) = 0;
	Gfx::GlyphTextureSet *titleFace();
	bool hasButtons() const;
	Gfx::PackedColor separatorColor() const;

protected:
	struct Control
	{
		OnPushDelegate onPush{};
		IG::WindowRect rect{};
		bool isActive = false;
	};
	static constexpr int controls = 3;
	std::array<Control, controls> control{};
	int selected = -1;
	Gfx::Text text;

	bool selectNextLeftButton();
	bool selectNextRightButton();
};

class BasicNavView : public NavView
{
public:
	BasicNavView(ViewAttachParams attach, Gfx::GlyphTextureSet *face, Gfx::TextureSpan leftRes, Gfx::TextureSpan rightRes);
	void setBackImage(Gfx::TextureSpan img);
	void setBackgroundGradient(std::span<const Gfx::LGradientStopDesc> gradStops);
	void draw(Gfx::RendererCommands &__restrict__, ViewDrawParams p = {}) const override;
	void place() override;
	void showLeftBtn(bool show) override;
	void showRightBtn(bool show) override;
	void setCenterTitle(bool on);
	void setRotateLeftButton(bool on);

protected:
	DynArray<Gfx::LGradientStopDesc> gradientStops;
	Gfx::IQuads selectQuad;
	Gfx::ObjectVertexArray<Gfx::Vertex2IColI> bgVerts;
	Gfx::ITexQuads buttonQuads;
	Gfx::TextureSpan leftTex, rightTex;
	bool centerTitle = true;
	bool rotateLeftBtn{};
};

}
