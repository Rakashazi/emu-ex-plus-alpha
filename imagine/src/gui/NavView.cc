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

#include <gui/NavView.hh>

void NavView::init(ResourceFace *face)
{
	text.init();
	text.setFace(face);
	leftBtnActive = rightBtnActive = 1;
	hasBackBtn = hasCloseBtn = 0;
}

void NavView::inputEvent(const InputEvent &e)
{
	assert(e.isPointer());
	if(e.state == INPUT_PUSHED)
	{
		if(hasCloseBtn && rightBtnActive && rightBtn.overlaps(e.x, e.y))
		{
			onRightNavBtn.invokeSafe(e);
		}
		else if(hasBackBtn && leftBtnActive && leftBtn.overlaps(e.x, e.y))
		{
			onLeftNavBtn.invokeSafe(e);
		}
	}
}

void NavView::deinitText()
{
	text.deinit();
}

void NavView::place()
{
	text.compile();
	//logMsg("setting textRect");
	textRect.setPosRel(viewRect.pos(LT2DO), viewRect.size(), LT2DO);
	leftBtn.setPosRel(viewRect.pos(LT2DO), viewRect.ySize(), LT2DO);
	if(hasBackBtn)
		textRect.x += leftBtn.xSize();
	rightBtn.setPosRel(viewRect.pos(RT2DO), viewRect.ySize(), RT2DO);
	if(hasCloseBtn)
		textRect.x2 -= rightBtn.xSize();
}

// BasicNavView

void BasicNavView::init(ResourceFace *face, ResourceImage *backRes, ResourceImage *closeRes,
		const GfxLGradientStopDesc *gradStop, uint gradStops)
{
	NavView::init(face);
	leftSpr.init(-.5, -.5, .5, .5, backRes);
	if(backRes) backRes->ref();
	rightSpr.init(-.5, -.5, .5, .5, closeRes);
	if(closeRes) closeRes->ref();
	if(backRes)
		hasBackBtn = 1;
	if(closeRes)
		hasCloseBtn = 1;
	bg.init(gradStops, gradStop, 0, 0);
}

void BasicNavView::deinit()
{
	NavView::deinitText();
	rightSpr.deinitAndFreeImg();
	leftSpr.deinitAndFreeImg();
	bg.deinit();
}

void BasicNavView::setBackImage(ResourceImage *img)
{
	if(leftSpr.img)
		leftSpr.img->deinit();
	leftSpr.setImg(img);
	if(img) img->ref();
	hasBackBtn = leftSpr.img != nullptr;
}

void BasicNavView::draw()
{
	using namespace Gfx;
	resetTransforms();
	shadeMod();//shadeModAlpha();
	bg.draw();
	setColor(COLOR_WHITE);
	text.draw(gXPos(viewRect, C2DO), gYPos(viewRect, C2DO), C2DO);
	if(leftSpr.img)
	{
		/*setBlendMode(BLEND_MODE_OFF);
		resetTransforms();
		setColor(.5, .5, .5, 1.);
		GeomRect::draw(leftBtn);*/
		if(leftBtnActive)
		{
			setColor(COLOR_WHITE);
			setBlendMode(BLEND_MODE_INTENSITY);
			loadTranslate(gXPos(leftBtn, C2DO), gYPos(leftBtn, C2DO));
			applyRollRotate(angle_fromDegree(90));
			leftSpr.draw(0);
		}
	}
	if(rightSpr.img)
	{
		/*setBlendMode(BLEND_MODE_OFF);
		resetTransforms();
		setColor(.5, .5, .5, 1.);
		GeomRect::draw(rightBtn);*/
		if(rightBtnActive)
		{
			setColor(COLOR_WHITE);
			setBlendMode(BLEND_MODE_INTENSITY);
			loadTranslate(gXPos(rightBtn, C2DO), gYPos(rightBtn, C2DO));
			applyRollRotate(angle_fromDegree(180));
			rightSpr.draw(0);
		}
	}
}

void BasicNavView::place()
{
	NavView::place();
	//logMsg("place nav view");
	//text.compile();
	//logMsg("setting textRect");
	//textRect.setPosRel(viewRect.pos(LT2DO), viewRect.size(), LT2DO);
	if(hasBackBtn)
	{
		//logMsg("setting leftBtn");
		//leftBtn.setPosRel(viewRect.pos(LT2DO), viewRect.ySize(), LT2DO);
		leftSpr.setPos(-Gfx::gXSize(leftBtn)/3., -Gfx::gYSize(leftBtn)/4., Gfx::gXSize(leftBtn)/3., Gfx::gYSize(leftBtn)/3.);
		//textRect.x += leftBtn.xSize();
	}
	if(hasCloseBtn)
	{
		//logMsg("setting rightBtn");
		//rightBtn.setPosRel(viewRect.pos(RT2DO), viewRect.ySize(), RT2DO);
		rightSpr.setPos(-Gfx::gXSize(rightBtn)/3., -Gfx::gYSize(rightBtn)/6., Gfx::gXSize(rightBtn)/3., Gfx::gYSize(rightBtn)/5.);
		//textRect.x2 -= rightBtn.xSize();
	}

	bg.setPos(Gfx::gXPos(viewRect, LT2DO), Gfx::gYPos(viewRect, LT2DO), Gfx::gXPos(viewRect, RB2DO), Gfx::gYPos(viewRect, RB2DO));
}
