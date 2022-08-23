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
#include <imagine/util/variant.hh>
#include <imagine/logger/logger.h>

namespace IG
{

NavView::NavView(ViewAttachParams attach, Gfx::GlyphTextureSet *face):
	View{attach},
	text{"", face}
{}

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
	for(auto i : iotaCount(controls))
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
	for(auto i : iotaCount(controls))
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

bool NavView::inputEvent(const Input::Event &e)
{
	return visit(overloaded
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
					logMsg("nav focus moved");
					selected = -1;
					return true;
				}
				else
				{
					logMsg("nav focus not moved");
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
	}, e);
}

void NavView::prepareDraw()
{
	text.makeGlyphs(renderer());
}

void NavView::place()
{
	text.compile(renderer(), projP);
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

Gfx::VertexColor NavView::separatorColor() const
{
	return Gfx::VertexColorPixelFormat.build(.25, .25, .25, 1.);
}

// BasicNavView

BasicNavView::BasicNavView(ViewAttachParams attach, Gfx::GlyphTextureSet *face, Gfx::TextureSpan backRes, Gfx::TextureSpan closeRes):
	NavView{attach, face}
{
	leftSpr = {{{-.5, -.5}, {.5, .5}}};
	rightSpr = {{{-.5, -.5}, {.5, .5}}};
	bool compiled = false;
	if(backRes)
	{
		leftSpr.set(backRes);
		control[0].isActive = true;
	}
	if(closeRes)
	{
		rightSpr.set(closeRes);
		control[2].isActive = true;
	}
}

void BasicNavView::setBackImage(Gfx::TextureSpan img)
{
	leftSpr.set(img);
	control[0].isActive = leftSpr.hasTexture();
}

void BasicNavView::setBackgroundGradient(std::span<const Gfx::LGradientStopDesc> gradStops)
{
	gradientStops = std::make_unique<Gfx::LGradientStopDesc[]>(gradStops.size());
	std::copy(gradStops.begin(), gradStops.end(), gradientStops.get());
	bg.setPos({gradientStops.get(), gradStops.size()}, {});
}

void BasicNavView::draw(Gfx::RendererCommands &__restrict__ cmds)
{
	using namespace IG::Gfx;
	auto const &textRect = control[1].rect;
	auto &basicEffect = cmds.basicEffect();
	if(bg)
	{
		cmds.set(BlendMode::OFF);
		basicEffect.disableTexture(cmds);
		if(viewRect().y > displayRect().y)
		{
			cmds.setColor(VertexColorPixelFormat.rgbaNorm(bg.mesh().v().data()->color));
			topBg.draw(cmds);
		}
		cmds.set(ColorName::WHITE);
		bg.draw(cmds);
	}
	if(selected != -1 && control[selected].isActive)
	{
		cmds.set(BlendMode::ALPHA);
		cmds.setColor(.2, .71, .9, 1./3.);
		basicEffect.disableTexture(cmds);
		GeomRect::draw(cmds, control[selected].rect, projP);
	}
	cmds.set(ColorName::WHITE);
	basicEffect.enableAlphaTexture(cmds);
	if(centerTitle)
	{
		text.draw(cmds, projP.alignToPixel(projP.unProjectRect(viewRect()).pos(C2DO)), C2DO, projP);
	}
	else
	{
		auto xIndent = manager().tableXIndent();
		if(text.width() > projP.unprojectXSize(textRect) - xIndent*2)
		{
			cmds.setClipRect(renderer().makeClipRect(window(), textRect));
			cmds.setClipTest(true);
			text.draw(cmds, projP.alignToPixel(projP.unProjectRect(textRect).pos(RC2DO) - FP{xIndent, 0}), RC2DO, projP);
			cmds.setClipTest(false);
		}
		else
		{
			text.draw(cmds, projP.alignToPixel(projP.unProjectRect(textRect).pos(LC2DO) + FP{xIndent, 0}), LC2DO, projP);
		}
	}
	if(control[0].isActive)
	{
		assumeExpr(leftSpr.hasTexture());
		cmds.set(BlendMode::ALPHA);
		cmds.set(ColorName::WHITE);
		cmds.set(imageCommonTextureSampler);
		auto trans = projP.makeTranslate(projP.unProjectRect(control[0].rect).pos(C2DO));
		if(rotateLeftBtn)
			trans = trans.rollRotate(radians(90.f));
		basicEffect.setModelView(cmds, trans);
		leftSpr.draw(cmds, basicEffect);
	}
	if(control[2].isActive)
	{
		assumeExpr(rightSpr.hasTexture());
		cmds.set(BlendMode::ALPHA);
		cmds.set(ColorName::WHITE);
		cmds.set(imageCommonTextureSampler);
		basicEffect.setModelView(cmds, projP.makeTranslate(projP.unProjectRect(control[2].rect).pos(C2DO)));
		rightSpr.draw(cmds, basicEffect);
	}
	basicEffect.setModelView(cmds, projP.makeTranslate());
}

void BasicNavView::place()
{
	using namespace IG::Gfx;
	auto &r = renderer();
	NavView::place();
	if(leftSpr.hasTexture())
	{
		auto rect = projP.unProjectRect(control[0].rect);
		Gfx::GCRect scaledRect{-rect.size() / 3.f, rect.size() / 3.f};
		leftSpr.setPos(scaledRect);
	}
	if(rightSpr.hasTexture())
	{
		auto rect = projP.unProjectRect(control[2].rect);
		Gfx::GCRect scaledRect{-rect.size() / 3.f, rect.size() / 3.f};
		rightSpr.setPos(scaledRect);
	}
	bg.setPos({gradientStops.get(), (size_t)bg.stops()}, projP.unProjectRect(displayRect().xRect() + viewRect().yRect()));
	if(viewRect().y > displayRect().y)
	{
		topBg.setPos(displayInsetRect(Direction::TOP), projP);
	}
}

void BasicNavView::showLeftBtn(bool show)
{
	control[0].isActive = show && leftSpr.hasTexture();
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
	control[2].isActive = show && rightSpr.hasTexture();
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
