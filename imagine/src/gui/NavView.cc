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

NavView::NavView(Gfx::GlyphTextureSet *face):
	text{"", face}
{
	text.setFace(face);
}

void NavView::setOnPushLeftBtn(OnPushDelegate del)
{
	onPushLeftBtn_ = del;
}

void NavView::setOnPushRightBtn(OnPushDelegate del)
{
	onPushRightBtn_ = del;
}

void NavView::setOnPushMiddleBtn(OnPushDelegate del)
{
	onPushMiddleBtn_ = del;
}

void NavView::inputEvent(Input::Event e)
{
	assert(e.isPointer());
	if(e.state == Input::PUSHED)
	{
		if(hasCloseBtn && rightBtn.overlaps(e.pos()))
		{
			onPushRightBtn_.callCopySafe(e);
		}
		else if(hasBackBtn && leftBtn.overlaps(e.pos()))
		{
			onPushLeftBtn_.callCopySafe(e);
		}
		else
		{
			auto centerBtnRect = textRect +
				IG::WindowRect{textRect.xSize() / 4, 0, -textRect.xSize() / 4, -textRect.ySize() / 8};
			if(centerBtnRect.overlaps(e.pos()))
			{
				onPushMiddleBtn_.callCopySafe(e);
			}
		}
	}
}

void NavView::place(const Gfx::ProjectionPlane &projP)
{
	text.compile(projP);
	//logMsg("setting textRect");
	textRect.setPosRel(viewRect_.pos(LT2DO), viewRect_.size(), LT2DO);
	leftBtn.setPosRel(viewRect_.pos(LT2DO), viewRect_.ySize(), LT2DO);
	if(hasBackBtn)
		textRect.x += leftBtn.xSize();
	rightBtn.setPosRel(viewRect_.pos(RT2DO), viewRect_.ySize(), RT2DO);
	if(hasCloseBtn)
		textRect.x2 -= rightBtn.xSize();
}

IG::WindowRect &NavView::viewRect()
{
	return viewRect_;
}

Gfx::GlyphTextureSet *NavView::titleFace()
{
	return text.face;
}

// BasicNavView

BasicNavView::BasicNavView(Gfx::GlyphTextureSet *face, Gfx::PixmapTexture *backRes, Gfx::PixmapTexture *closeRes):
	NavView{face}
{
	leftSpr.init({-.5, -.5, .5, .5});
	rightSpr.init({-.5, -.5, .5, .5});
	bool compiled = false;
	if(backRes)
	{
		leftSpr.setImg(*backRes);
		compiled |= backRes->compileDefaultProgram(Gfx::IMG_MODE_MODULATE);
		hasBackBtn = true;
	}
	if(closeRes)
	{
		rightSpr.setImg(*closeRes);
		compiled |= closeRes->compileDefaultProgram(Gfx::IMG_MODE_MODULATE);
		hasCloseBtn = true;
	}
	if(compiled)
		Gfx::autoReleaseShaderCompiler();
}

void BasicNavView::setBackImage(Gfx::PixmapTexture *img)
{
	leftSpr.setImg(img);
	if(leftSpr.compileDefaultProgram(Gfx::IMG_MODE_MODULATE))
		Gfx::autoReleaseShaderCompiler();
	hasBackBtn = leftSpr.image();
}

void BasicNavView::setBackgroundGradient(const Gfx::LGradientStopDesc *gradStop, uint gradStops)
{
	gradientStops = std::make_unique<Gfx::LGradientStopDesc[]>(gradStops);
	memcpy(gradientStops.get(), gradStop, sizeof(Gfx::LGradientStopDesc) * gradStops);
	bg.setPos(gradientStops.get(), gradStops, {});
}

void BasicNavView::draw(const Base::Window &win, const Gfx::ProjectionPlane &projP)
{
	using namespace Gfx;
	if(bg)
	{
		setBlendMode(0);
		noTexProgram.use(projP.makeTranslate());
		bg.draw();
	}
	setColor(COLOR_WHITE);
	texAlphaReplaceProgram.use();
	if(centerTitle)
	{
		text.draw(projP.alignToPixel(projP.unProjectRect(viewRect_).pos(C2DO)), C2DO, projP);
	}
	else
	{
		if(text.xSize > projP.unprojectXSize(textRect) - TableView::globalXIndent*2)
		{
			setClipRectBounds(win, textRect);
			setClipRect(true);
			text.draw(projP.alignToPixel(projP.unProjectRect(textRect).pos(RC2DO) - GP{TableView::globalXIndent, 0}), RC2DO, projP);
			setClipRect(false);
		}
		else
		{
			text.draw(projP.alignToPixel(projP.unProjectRect(textRect).pos(LC2DO) + GP{TableView::globalXIndent, 0}), LC2DO, projP);
		}
	}
	if(hasBackBtn)
	{
		assumeExpr(leftSpr.image());
		setColor(COLOR_WHITE);
		setBlendMode(BLEND_MODE_ALPHA);
		TextureSampler::bindDefaultNearestMipClampSampler();
		auto trans = projP.makeTranslate(projP.unProjectRect(leftBtn).pos(C2DO));
		if(rotateLeftBtn)
			trans = trans.rollRotate(angleFromDegree(90));
		leftSpr.useDefaultProgram(IMG_MODE_MODULATE, trans);
		leftSpr.draw();
	}
	if(hasCloseBtn)
	{
		assumeExpr(rightSpr.image());
		setColor(COLOR_WHITE);
		setBlendMode(BLEND_MODE_ALPHA);
		TextureSampler::bindDefaultNearestMipClampSampler();
		rightSpr.useDefaultProgram(IMG_MODE_MODULATE, projP.makeTranslate(projP.unProjectRect(rightBtn).pos(C2DO)));
		rightSpr.draw();
	}
}

void BasicNavView::place(const Gfx::ProjectionPlane &projP)
{
	using namespace Gfx;
	NavView::place(projP);
	if(leftSpr.image())
	{
		auto rect = projP.unProjectRect(leftBtn);
		Gfx::GCRect scaledRect{-rect.xSize() / 3_gc, -rect.ySize() / 3_gc, rect.xSize() / 3_gc, rect.ySize() / 3_gc};
		leftSpr.setPos(scaledRect);
	}
	if(rightSpr.image())
	{
		auto rect = projP.unProjectRect(rightBtn);
		Gfx::GCRect scaledRect{-rect.xSize() / 3_gc, -rect.ySize() / 3_gc, rect.xSize() / 3_gc, rect.ySize() / 3_gc};
		rightSpr.setPos(scaledRect);
	}
	bg.setPos(gradientStops.get(), bg.stops(), projP.unProjectRect(viewRect_));
}

void BasicNavView::showLeftBtn(bool show)
{
	hasBackBtn = show && leftSpr.image();
}

void BasicNavView::showRightBtn(bool show)
{
	hasCloseBtn = show && rightSpr.image();
}
