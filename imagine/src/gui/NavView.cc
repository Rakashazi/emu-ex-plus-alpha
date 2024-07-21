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

#include <imagine/gui/NavView.hh>
#include <imagine/gui/ViewManager.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/Mat4.hh>
#include <imagine/util/variant.hh>
#include <imagine/logger/logger.h>

namespace IG
{

constexpr SystemLogger log;

NavView::NavView(ViewAttachParams attach, Gfx::GlyphTextureSet *face):
	View{attach},
	text{attach.rendererTask, u"", face} {}

void NavView::setOnPushLeftBtn(OnPushDelegate del)
{
	control[0].onPush = del;
}

void NavView::setOnPushMiddleBtn(OnPushDelegate del)
{
	control[1].onPush = del;
	control[1].isActive = (bool)del;
}

void NavView::setOnPushRightBtn(OnPushDelegate del)
{
	control[2].onPush = del;
}

bool NavView::selectNextLeftButton()
{
	if(selected == -1)
		selected = 1;
	int elem = IG::wrapMinMax(selected - 1, 0, controls);
	for([[maybe_unused]] auto i : iotaCount(controls))
	{
		if(control[elem].isActive)
		{
			selected = elem;
			postDraw();
			return true;
		}
		elem = IG::wrapMinMax(elem-1, 0, controls);
	}
	return false;
}

bool NavView::selectNextRightButton()
{
	if(selected == -1)
		selected = controls - 2;
	int elem = IG::wrapMinMax(selected + 1, 0, controls);
	for([[maybe_unused]] auto i : iotaCount(controls))
	{
		if(control[elem].isActive)
		{
			selected = elem;
			postDraw();
			return true;
		}
		elem = IG::wrapMinMax(elem+1, 0, controls);
	}
	return false;
}

bool NavView::inputEvent(const Input::Event &e, ViewInputEventParams)
{
	return e.visit(overloaded
	{
		[&](const Input::KeyEvent &keyEv)
		{
			if(!keyEv.pushed())
				return false;
			if(keyEv.isDefaultUpButton() || keyEv.isDefaultDownButton())
			{
				if(keyEv.repeated())
					return false;
				if(selected == -1)
				{
					return selectNextLeftButton();
				}
				else if(moveFocusToNextView(keyEv, keyEv.isDefaultDownButton() ? CB2DO : CT2DO))
				{
					log.info("nav focus moved");
					selected = -1;
					return true;
				}
				else
				{
					log.info("nav focus not moved");
				}
			}
			else if(keyEv.isDefaultLeftButton())
			{
				return selectNextLeftButton();
			}
			else if(keyEv.isDefaultRightButton())
			{
				return selectNextRightButton();
			}
			else if(keyEv.isDefaultConfirmButton() && selected != -1 && control[selected].isActive)
			{
				control[selected].onPush.callCopySafe(e);
				return true;
			}
			return false;
		},
		[&](const Input::MotionEvent &motionEv)
		{
			if(motionEv.pushed())
			{
				for(auto &c : control)
				{
					if(c.isActive && c.rect.overlaps(motionEv.pos()))
					{
						selected = -1;
						c.onPush.callCopySafe(e);
						return true;
					}
				}
			}
			return false;
		}
	});
}

void NavView::prepareDraw()
{
	text.makeGlyphs();
}

void NavView::place()
{
	text.compile();
	auto &textRect = control[1].rect;
	textRect.setPosRel(viewRect().pos(LT2DO), viewRect().size(), LT2DO);
	control[0].rect.setPosRel(viewRect().pos(LT2DO), viewRect().ySize(), LT2DO);
	if(control[0].isActive)
		textRect.x += control[0].rect.xSize();
	control[2].rect.setPosRel(viewRect().pos(RT2DO), viewRect().ySize(), RT2DO);
	if(control[2].isActive)
		textRect.x2 -= control[2].rect.xSize();
}

void NavView::clearSelection()
{
	selected = -1;
}

Gfx::GlyphTextureSet *NavView::titleFace()
{
	return text.face();
}

bool NavView::hasButtons() const
{
	return control[0].isActive || control[2].isActive;
}

Gfx::PackedColor NavView::separatorColor() const
{
	return Gfx::PackedColor::format.build(.25, .25, .25, 1.);
}

// BasicNavView

BasicNavView::BasicNavView(ViewAttachParams attach, Gfx::GlyphTextureSet *face, Gfx::TextureSpan backRes, Gfx::TextureSpan closeRes):
	NavView{attach, face},
	selectQuad{attach.rendererTask, {.size = 1}},
	bgVerts{attach.rendererTask, {.size = 0}},
	buttonQuads{attach.rendererTask, {.size = 2}}
{
	selectQuad.write(0, {.bounds = {{}, {1, 1}}});
	if(backRes)
	{
		leftTex = backRes;
		control[0].isActive = true;
	}
	if(closeRes)
	{
		rightTex = closeRes;
		control[2].isActive = true;
	}
}

void BasicNavView::setBackImage(Gfx::TextureSpan img)
{
	leftTex = img;
	control[0].isActive = bool(img);
	place();
}

void BasicNavView::setBackgroundGradient(std::span<const Gfx::LGradientStopDesc> gradStops)
{
	if(!gradStops.size())
	{
		gradientStops = {};
		bgVerts.reset({.size = 0});
		return;
	}
	gradientStops.resetForOverwrite(gradStops.size());
	std::ranges::copy(gradStops, gradientStops.begin());
}

void BasicNavView::draw(Gfx::RendererCommands &__restrict__ cmds, ViewDrawParams) const
{
	using namespace IG::Gfx;
	auto const &textRect = control[1].rect;
	auto &basicEffect = cmds.basicEffect();
	basicEffect.setModelView(cmds, Mat4::ident());
	if(bgVerts.size())
	{
		cmds.set(BlendMode::OFF);
		basicEffect.disableTexture(cmds);
		cmds.drawPrimitives(Primitive::TRIANGLE_STRIP, bgVerts, 0, bgVerts.size());
	}
	if(selected != -1 && control[selected].isActive)
	{
		cmds.set(BlendMode::ALPHA);
		cmds.setColor({.2, .71, .9, 1./3.});
		basicEffect.disableTexture(cmds);
		basicEffect.setModelView(cmds, Mat4::makeTranslateScale(control[selected].rect));
		cmds.drawQuad(selectQuad, 0);
	}
	basicEffect.enableAlphaTexture(cmds);
	if(centerTitle)
	{
		text.draw(cmds, viewRect().pos(C2DO), C2DO, ColorName::WHITE);
	}
	else
	{
		auto xIndent = manager().tableXIndentPx;
		if(text.width() > textRect.xSize() - xIndent * 2)
		{
			cmds.setClipRect(renderer().makeClipRect(window(), textRect));
			cmds.setClipTest(true);
			text.draw(cmds, textRect.pos(RC2DO) - WPt{xIndent, 0}, RC2DO, ColorName::WHITE);
			cmds.setClipTest(false);
		}
		else
		{
			text.draw(cmds, textRect.pos(LC2DO) + WPt{xIndent, 0}, LC2DO, ColorName::WHITE);
		}
	}
	if(control[0].isActive || control[2].isActive)
	{
		cmds.set(BlendMode::PREMULT_ALPHA);
		cmds.setColor(ColorName::WHITE);
		cmds.setVertexArray(buttonQuads);
	}
	if(control[0].isActive)
	{
		auto trans = Mat4::makeTranslate(control[0].rect.pos(C2DO));
		if(rotateLeftBtn)
			trans = trans.rollRotate(radians(-90.f));
		basicEffect.setModelView(cmds, trans);
		basicEffect.drawSprite(cmds, 0, leftTex);
	}
	if(control[2].isActive)
	{
		basicEffect.setModelView(cmds, Mat4::makeTranslate(control[2].rect.pos(C2DO)));
		basicEffect.drawSprite(cmds, 1, rightTex);
	}
	basicEffect.setModelView(cmds, Mat4::ident());
}

void BasicNavView::place()
{
	using namespace IG::Gfx;
	NavView::place();
	if(leftTex)
	{
		auto rect = control[0].rect;
		WRect scaledRect{-rect.size() / 3, rect.size() / 3};
		buttonQuads.write(0, {.bounds = scaledRect.as<int16_t>(), .textureSpan = leftTex});
	}
	if(rightTex)
	{
		auto rect = control[2].rect;
		WRect scaledRect{-rect.size() / 3, rect.size() / 3};
		buttonQuads.write(1, {.bounds = scaledRect.as<int16_t>(), .textureSpan = rightTex});
	}
	bool needsTopPadding = viewRect().y > displayRect().y;
	auto bgVertsSize = LGradient::vertexSize(gradientStops.size(), needsTopPadding ? LGradientPadMode::top : LGradientPadMode::none);
	bgVerts.reset({.size = bgVertsSize});
	auto bgVertsMap = bgVerts.map();
	auto rect = displayRect().xRect() + viewRect().yRect();
	std::optional<int> topPadding = needsTopPadding ? displayInsetRect(Direction::TOP).y : std::optional<int>{};
	LGradient::write(bgVertsMap, 0, gradientStops, rect, topPadding);
}

void BasicNavView::showLeftBtn(bool show)
{
	control[0].isActive = show && leftTex;
	if(!show && selected == 0)
	{
		if(control[2].isActive)
			selected = 2;
		else
			selected = -1;
	}
}

void BasicNavView::showRightBtn(bool show)
{
	control[2].isActive = show && rightTex;
	if(!show && selected == 1)
	{
		if(control[0].isActive)
			selected = 0;
		else
			selected = -1;
	}
}

void BasicNavView::setCenterTitle(bool on)
{
	centerTitle = on;
}

void BasicNavView::setRotateLeftButton(bool on)
{
	rotateLeftBtn = on;
}

}
