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
#include <imagine/gui/TableView.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/logger/logger.h>

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
	iterateTimes(controls, i)
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
	iterateTimes(controls, i)
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

bool NavView::inputEvent(Input::Event e)
{
	if(!e.isPointer())
	{
		if(!e.pushed())
			return false;
		if(e.isDefaultUpButton() || e.isDefaultDownButton())
		{
			if(e.repeated())
				return false;
			if(selected == -1)
			{
				return selectNextLeftButton();
			}
			else if(moveFocusToNextView(e, e.isDefaultDownButton() ? CB2DO : CT2DO))
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
		else if(e.isDefaultLeftButton())
		{
			return selectNextLeftButton();
		}
		else if(e.isDefaultRightButton())
		{
			return selectNextRightButton();
		}
		else if(e.isDefaultConfirmButton() && selected != -1 && control[selected].isActive)
		{
			control[selected].onPush.callCopySafe(e);
			return true;
		}
	}
	else if(e.pushed())
	{
		for(auto &c : control)
		{
			if(c.isActive && c.rect.overlaps(e.pos()))
			{
				selected = -1;
				c.onPush.callCopySafe(e);
				return true;
			}
		}
	}
	return false;
}

void NavView::setTitle(NameString title)
{
	text.setString(std::move(title));
}

void NavView::setTitle(NameStringView title)
{
	setTitle(NameString{title});
}

void NavView::prepareDraw()
{
	text.makeGlyphs(renderer());
}

void NavView::place()
{
	text.compile(renderer(), projP);
	auto &textRect = control[1].rect;
	textRect.setPosRel(viewRect_.pos(LT2DO), viewRect_.size(), LT2DO);
	control[0].rect.setPosRel(viewRect_.pos(LT2DO), viewRect_.ySize(), LT2DO);
	if(control[0].isActive)
		textRect.x += control[0].rect.xSize();
	control[2].rect.setPosRel(viewRect_.pos(RT2DO), viewRect_.ySize(), RT2DO);
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

// BasicNavView

BasicNavView::BasicNavView(ViewAttachParams attach, Gfx::GlyphTextureSet *face, Gfx::TextureSpan backRes, Gfx::TextureSpan closeRes):
	NavView{attach, face}
{
	leftSpr = {{{-.5, -.5}, {.5, .5}}};
	rightSpr = {{{-.5, -.5}, {.5, .5}}};
	bool compiled = false;
	if(backRes)
	{
		leftSpr.setImg(backRes);
		compiled |= leftSpr.compileDefaultProgram(Gfx::IMG_MODE_MODULATE);
		control[0].isActive = true;
	}
	if(closeRes)
	{
		rightSpr.setImg(closeRes);
		compiled |= rightSpr.compileDefaultProgram(Gfx::IMG_MODE_MODULATE);
		control[2].isActive = true;
	}
	if(compiled)
		renderer().autoReleaseShaderCompiler();
}

void BasicNavView::setBackImage(Gfx::TextureSpan img)
{
	leftSpr.setImg(img);
	if(leftSpr.compileDefaultProgram(Gfx::IMG_MODE_MODULATE))
		renderer().autoReleaseShaderCompiler();
	control[0].isActive = leftSpr.image();
}

void BasicNavView::setBackgroundGradient(const Gfx::LGradientStopDesc *gradStop, uint32_t gradStops)
{
	gradientStops = std::make_unique<Gfx::LGradientStopDesc[]>(gradStops);
	memcpy(gradientStops.get(), gradStop, sizeof(Gfx::LGradientStopDesc) * gradStops);
	bg.setPos(gradientStops.get(), gradStops, {});
}

void BasicNavView::draw(Gfx::RendererCommands &cmds)
{
	using namespace Gfx;
	auto const &textRect = control[1].rect;
	if(bg)
	{
		cmds.setBlendMode(0);
		cmds.setCommonProgram(CommonProgram::NO_TEX, projP.makeTranslate());
		bg.draw(cmds);
	}
	if(selected != -1 && control[selected].isActive)
	{
		cmds.setBlendMode(BLEND_MODE_ALPHA);
		cmds.setColor(.2, .71, .9, 1./3.);
		cmds.setCommonProgram(CommonProgram::NO_TEX, projP.makeTranslate());
		GeomRect::draw(cmds, control[selected].rect, projP);
	}
	cmds.set(ColorName::WHITE);
	cmds.setCommonProgram(CommonProgram::TEX_ALPHA);
	if(centerTitle)
	{
		text.draw(cmds, projP.alignToPixel(projP.unProjectRect(viewRect_).pos(C2DO)), C2DO, projP);
	}
	else
	{
		auto xIndent = manager().tableXIndent();
		if(text.width() > projP.unprojectXSize(textRect) - xIndent*2)
		{
			cmds.setClipRect(renderer().makeClipRect(window(), textRect));
			cmds.setClipTest(true);
			text.draw(cmds, projP.alignToPixel(projP.unProjectRect(textRect).pos(RC2DO) - GP{xIndent, 0}), RC2DO, projP);
			cmds.setClipTest(false);
		}
		else
		{
			text.draw(cmds, projP.alignToPixel(projP.unProjectRect(textRect).pos(LC2DO) + GP{xIndent, 0}), LC2DO, projP);
		}
	}
	if(control[0].isActive)
	{
		assumeExpr(leftSpr.image());
		cmds.setBlendMode(BLEND_MODE_ALPHA);
		cmds.set(ColorName::WHITE);
		cmds.set(imageCommonTextureSampler);
		auto trans = projP.makeTranslate(projP.unProjectRect(control[0].rect).pos(C2DO));
		if(rotateLeftBtn)
			trans = trans.rollRotate(angleFromDegree(90));
		leftSpr.setCommonProgram(cmds, IMG_MODE_MODULATE, trans);
		leftSpr.draw(cmds);
	}
	if(control[2].isActive)
	{
		assumeExpr(rightSpr.image());
		cmds.setBlendMode(BLEND_MODE_ALPHA);
		cmds.set(ColorName::WHITE);
		cmds.set(imageCommonTextureSampler);
		rightSpr.setCommonProgram(cmds, IMG_MODE_MODULATE, projP.makeTranslate(projP.unProjectRect(control[2].rect).pos(C2DO)));
		rightSpr.draw(cmds);
	}
}

void BasicNavView::place()
{
	using namespace Gfx;
	auto &r = renderer();
	NavView::place();
	if(leftSpr.image())
	{
		auto rect = projP.unProjectRect(control[0].rect);
		Gfx::GCRect scaledRect{-rect.size() / 3_gc, rect.size() / 3_gc};
		leftSpr.setPos(scaledRect);
	}
	if(rightSpr.image())
	{
		auto rect = projP.unProjectRect(control[2].rect);
		Gfx::GCRect scaledRect{-rect.size() / 3_gc, rect.size() / 3_gc};
		rightSpr.setPos(scaledRect);
	}
	bg.setPos(gradientStops.get(), bg.stops(), projP.unProjectRect(viewRect_));
}

void BasicNavView::showLeftBtn(bool show)
{
	control[0].isActive = show && leftSpr.image();
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
	control[2].isActive = show && rightSpr.image();
	if(!show && selected == 1)
	{
		if(control[0].isActive)
			selected = 0;
		else
			selected = -1;
	}
}
