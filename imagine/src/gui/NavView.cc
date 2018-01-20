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
	else if(e.state() == Input::PUSHED)
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

void NavView::onAddedToController(Input::Event e) {}

IG::WindowRect &NavView::viewRect()
{
	return viewRect_;
}

Gfx::GlyphTextureSet *NavView::titleFace()
{
	return text.face;
}

bool NavView::hasButtons() const
{
	return control[0].isActive || control[2].isActive;
}

// BasicNavView

BasicNavView::BasicNavView(ViewAttachParams attach, Gfx::GlyphTextureSet *face, Gfx::PixmapTexture *backRes, Gfx::PixmapTexture *closeRes):
	NavView{attach, face}
{
	leftSpr.init({-.5, -.5, .5, .5});
	rightSpr.init({-.5, -.5, .5, .5});
	bool compiled = false;
	if(backRes)
	{
		leftSpr.setImg(*backRes);
		compiled |= backRes->compileDefaultProgram(Gfx::IMG_MODE_MODULATE);
		control[0].isActive = true;
	}
	if(closeRes)
	{
		rightSpr.setImg(*closeRes);
		compiled |= closeRes->compileDefaultProgram(Gfx::IMG_MODE_MODULATE);
		control[2].isActive = true;
	}
	if(compiled)
		renderer().autoReleaseShaderCompiler();
}

void BasicNavView::setBackImage(Gfx::PixmapTexture *img)
{
	leftSpr.setImg(img);
	if(leftSpr.compileDefaultProgram(Gfx::IMG_MODE_MODULATE))
		renderer().autoReleaseShaderCompiler();
	control[0].isActive = leftSpr.image();
}

void BasicNavView::setBackgroundGradient(const Gfx::LGradientStopDesc *gradStop, uint gradStops)
{
	gradientStops = std::make_unique<Gfx::LGradientStopDesc[]>(gradStops);
	memcpy(gradientStops.get(), gradStop, sizeof(Gfx::LGradientStopDesc) * gradStops);
	bg.setPos(gradientStops.get(), gradStops, {});
}

void BasicNavView::draw()
{
	using namespace Gfx;
	auto &r = renderer();
	auto const &textRect = control[1].rect;
	if(bg)
	{
		r.setBlendMode(0);
		r.noTexProgram.use(r, projP.makeTranslate());
		bg.draw(r);
	}
	if(selected != -1 && control[selected].isActive)
	{
		r.setBlendMode(BLEND_MODE_ALPHA);
		r.setColor(.2, .71, .9, 1./3.);
		r.noTexProgram.use(r, projP.makeTranslate());
		GeomRect::draw(r, control[selected].rect, projP);
	}
	r.setColor(COLOR_WHITE);
	r.texAlphaReplaceProgram.use(r);
	if(centerTitle)
	{
		text.draw(r, projP.alignToPixel(projP.unProjectRect(viewRect_).pos(C2DO)), C2DO, projP);
	}
	else
	{
		if(text.xSize > projP.unprojectXSize(textRect) - TableView::globalXIndent*2)
		{
			r.setClipRectBounds(window(), textRect);
			r.setClipRect(true);
			text.draw(r, projP.alignToPixel(projP.unProjectRect(textRect).pos(RC2DO) - GP{TableView::globalXIndent, 0}), RC2DO, projP);
			r.setClipRect(false);
		}
		else
		{
			text.draw(r, projP.alignToPixel(projP.unProjectRect(textRect).pos(LC2DO) + GP{TableView::globalXIndent, 0}), LC2DO, projP);
		}
	}
	if(control[0].isActive)
	{
		assumeExpr(leftSpr.image());
		r.setBlendMode(BLEND_MODE_ALPHA);
		r.setColor(COLOR_WHITE);
		TextureSampler::bindDefaultNearestMipClampSampler(r);
		auto trans = projP.makeTranslate(projP.unProjectRect(control[0].rect).pos(C2DO));
		if(rotateLeftBtn)
			trans = trans.rollRotate(angleFromDegree(90));
		leftSpr.useDefaultProgram(IMG_MODE_MODULATE, trans);
		leftSpr.draw(r);
	}
	if(control[2].isActive)
	{
		assumeExpr(rightSpr.image());
		r.setBlendMode(BLEND_MODE_ALPHA);
		r.setColor(COLOR_WHITE);
		TextureSampler::bindDefaultNearestMipClampSampler(r);
		rightSpr.useDefaultProgram(IMG_MODE_MODULATE, projP.makeTranslate(projP.unProjectRect(control[2].rect).pos(C2DO)));
		rightSpr.draw(r);
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
		Gfx::GCRect scaledRect{-rect.xSize() / 3_gc, -rect.ySize() / 3_gc, rect.xSize() / 3_gc, rect.ySize() / 3_gc};
		leftSpr.setPos(scaledRect);
	}
	if(rightSpr.image())
	{
		auto rect = projP.unProjectRect(control[2].rect);
		Gfx::GCRect scaledRect{-rect.xSize() / 3_gc, -rect.ySize() / 3_gc, rect.xSize() / 3_gc, rect.ySize() / 3_gc};
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
