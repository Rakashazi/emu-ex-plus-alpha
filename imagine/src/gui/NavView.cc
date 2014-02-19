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
#include <gui/View.hh>

void NavView::init(ResourceFace *face)
{
	text.setFace(face);
	leftBtnActive = rightBtnActive = true;
	hasBackBtn = hasCloseBtn = false;
}

void NavView::inputEvent(const Input::Event &e)
{
	assert(e.isPointer());
	if(e.state == Input::PUSHED)
	{
		if(hasCloseBtn && rightBtnActive && rightBtn.overlaps({e.x, e.y}))
		{
			onRightNavBtn(e);
		}
		else if(hasBackBtn && leftBtnActive && leftBtn.overlaps({e.x, e.y}))
		{
			onLeftNavBtn(e);
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

void BasicNavView::init(ResourceFace *face, Gfx::BufferImage *backRes, Gfx::BufferImage *closeRes,
		const Gfx::LGradientStopDesc *gradStop, uint gradStops)
{
	NavView::init(face);
	leftSpr.init(-.5, -.5, .5, .5, backRes);
	rightSpr.init(-.5, -.5, .5, .5, closeRes);
	bool compiled = false;
	if(backRes)
	{
		compiled |= backRes->compileDefaultProgram(Gfx::IMG_MODE_MODULATE);
		hasBackBtn = 1;
	}
	if(closeRes)
	{
		compiled |= closeRes->compileDefaultProgram(Gfx::IMG_MODE_MODULATE);
		hasCloseBtn = 1;
	}
	if(compiled)
		Gfx::autoReleaseShaderCompiler();
	bg.init(gradStops, gradStop, 0, 0);
}

void BasicNavView::deinit()
{
	NavView::deinitText();
	rightSpr.deinit();
	leftSpr.deinit();
	bg.deinit();
}

void BasicNavView::setBackImage(Gfx::BufferImage *img)
{
	leftSpr.setImg(img);
	if(leftSpr.compileDefaultProgram(Gfx::IMG_MODE_MODULATE))
		Gfx::autoReleaseShaderCompiler();
	hasBackBtn = leftSpr.image();
}

void BasicNavView::draw(const Base::Window &win)
{
	using namespace Gfx;
	setBlendMode(0);
	noTexProgram.use(View::projP.makeTranslate());
	bg.draw();
	setColor(COLOR_WHITE);
	texAlphaReplaceProgram.use();
	//text.draw(unproject(viewRect, C2DO), C2DO);
	text.draw(View::projP.unProjectRect(viewRect).pos(C2DO), C2DO);
	if(leftSpr.image())
	{
		/*setBlendMode(BLEND_MODE_OFF);
		resetTransforms();
		setColor(.5, .5, .5, 1.);
		GeomRect::draw(leftBtn);*/
		if(leftBtnActive)
		{
			setBlendMode(BLEND_MODE_ALPHA);
			leftSpr.useDefaultProgram(IMG_MODE_MODULATE, View::projP.makeTranslate(View::projP.unProjectRect(leftBtn).pos(C2DO)));
			leftSpr.draw();
		}
	}
	if(rightSpr.image())
	{
		/*setBlendMode(BLEND_MODE_OFF);
		resetTransforms();
		setColor(.5, .5, .5, 1.);
		GeomRect::draw(rightBtn);*/
		if(rightBtnActive)
		{
			setBlendMode(BLEND_MODE_ALPHA);
			rightSpr.useDefaultProgram(IMG_MODE_MODULATE, View::projP.makeTranslate(View::projP.unProjectRect(rightBtn).pos(C2DO)));
			rightSpr.draw();
		}
	}
}

void BasicNavView::place()
{
	using namespace Gfx;
	NavView::place();
	if(hasBackBtn)
	{
		auto rect = View::projP.unProjectRect(leftBtn);
		Gfx::GCRect scaledRect{-rect.xSize() / 3_gc, -rect.ySize() / 3_gc, rect.xSize() / 3_gc, rect.ySize() / 3_gc};
		leftSpr.setPos(scaledRect);
	}
	if(hasCloseBtn)
	{
		auto rect = View::projP.unProjectRect(rightBtn);
		Gfx::GCRect scaledRect{-rect.xSize() / 3_gc, -rect.ySize() / 3_gc, rect.xSize() / 3_gc, rect.ySize() / 3_gc};
		rightSpr.setPos(scaledRect);
	}
	//bg.setPos(Gfx::gXPos(viewRect, LT2DO), Gfx::gYPos(viewRect, LT2DO), Gfx::gXPos(viewRect, RB2DO), Gfx::gYPos(viewRect, RB2DO));
	bg.setPos(View::projP.unProjectRect(viewRect));
}
