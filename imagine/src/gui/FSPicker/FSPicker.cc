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

#define thisModuleName "FSPicker"

#include "FSPicker.hh"

static const GfxLGradientStopDesc fsNavViewGrad[] =
{
	{ .0, VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	{ .03, VertexColorPixelFormat.build(1. * .4, 1. * .4, 1. * .4, 1.) },
	{ .3, VertexColorPixelFormat.build(1. * .4, 1. * .4, 1. * .4, 1.) },
	{ .97, VertexColorPixelFormat.build(.35 * .4, .35 * .4, .35 * .4, 1.) },
	{ 1., VertexColorPixelFormat.build(.5, .5, .5, 1.) },
};

// NavView

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

// FSNavView

void FSNavView::init(ResourceFace *face, ResourceImage *backRes, ResourceImage *closeRes, bool singleDir)
{
	if(singleDir)
		backRes = 0;
	BasicNavView::init(face, backRes, closeRes, fsNavViewGrad, sizeofArray(fsNavViewGrad));
	//logMsg("has back:%p close:%p", leftSpr.img, rightSpr.img);
}

void FSNavView::draw()
{
	using namespace Gfx;
	resetTransforms();
	shadeMod();//shadeModAlpha();
	bg.draw();
	setColor(COLOR_WHITE);
	if(text.xSize > gXSize(textRect) - GuiTable1D::globalXIndent*2)
	{
		setClipRectBounds(textRect);
		setClipRect(1);
		text.draw(gXPos(textRect, RC2DO) - GuiTable1D::globalXIndent, gYPos(viewRect, RC2DO), RC2DO);
		setClipRect(0);
	}
	else
	{
		text.draw(gXPos(textRect, LC2DO) + GuiTable1D::globalXIndent, gYPos(viewRect, LC2DO), LC2DO);
	}
	if(leftSpr.img)
	{
		if(leftBtnActive)
		{
			setColor(COLOR_WHITE);
			setBlendMode(BLEND_MODE_INTENSITY);
			loadTranslate(gXPos(leftBtn, C2DO), gYPos(leftBtn, C2DO));
			//applyRollRotate(angle_fromDegree(90));
			leftSpr.draw(0);
		}
	}
	if(rightSpr.img)
	{
		if(rightBtnActive)
		{
			setColor(COLOR_WHITE);
			setBlendMode(BLEND_MODE_INTENSITY);
			loadTranslate(gXPos(rightBtn, C2DO), gYPos(rightBtn, C2DO));
			rightSpr.draw(0);
		}
	}
}

void FSNavView::place()
{
	NavView::place();
	if(hasBackBtn)
	{
		leftSpr.setPos(-Gfx::gXSize(leftBtn)/3., -Gfx::gYSize(leftBtn)/6., Gfx::gXSize(leftBtn)/3., Gfx::gYSize(leftBtn)/5.);
	}
	if(hasCloseBtn)
	{
		rightSpr.setPos(-Gfx::gXSize(rightBtn)/3., -Gfx::gYSize(rightBtn)/3., Gfx::gXSize(rightBtn)/3., Gfx::gYSize(rightBtn)/3.);
	}
	bg.setPos(Gfx::gXPos(viewRect, LT2DO), Gfx::gYPos(viewRect, LT2DO), Gfx::gXPos(viewRect, RB2DO), Gfx::gYPos(viewRect, RB2DO));
}

// FSPicker

void FSPicker::init(const char *path, ResourceImage *backRes, ResourceImage *closeRes, FsDirFilterFunc filter,  bool singleDir, ResourceFace *face)
{
	#if defined(CONFIG_BASE_IOS) && !defined(CONFIG_BASE_IOS_JB)
	singleDir = 1; // stay in Documents dir when not in jailbreak environment
	#endif
	text = 0;
	faceRes = face;
	//viewFrame = view;
	this->filter = filter;
	this->singleDir = singleDir;
	navV.init(face, backRes, closeRes, singleDir);
	loadDir(path);
}

void FSPicker::deinit()
{
	dir.closeDir();
	delete[] text;
	navV.deinit();
}

void FSPicker::place()
{
	iterateTimes(tbl.cells, i)
	{
		text[i].compile();
	}

	tbl.setYCellSize(faceRes->nominalHeight()*2);

	//logMsg("setting viewRect");
	navV.viewRect.setPosRel(viewFrame.x, viewFrame.y, viewFrame.xSize(), faceRes->nominalHeight() * 1.75, LT2DO);
	Rect2<int> tableFrame = viewFrame;
	tableFrame.setYPos(navV.viewRect.yPos(LB2DO));
	tableFrame.y2 -= navV.viewRect.ySize();
	tbl.place(&tableFrame);
	//logMsg("nav %d, table %d, content %d", gfx_toIYSize(navV.view.ySize), tbl.viewFrame.ySize(), tbl.contentFrame->ySize());

	navV.place();
}

void FSPicker::changeDirByInput(const char *path, const InputEvent &e)
{
	loadDir(path);
	if(!e.isPointer() && tbl.cells)
		tbl.selected = 0;
	place();
	Base::displayNeedsUpdate();
}

void FSPicker::onLeftNavBtn(const InputEvent &e)
{
	changeDirByInput("..", e);
}

void FSPicker::onRightNavBtn(const InputEvent &e)
{
	//onClose();
	onClose.invoke(e);
}

void FSPicker::inputEvent(const InputEvent &e)
{
	const char* dirChange = 0;
	if(!singleDir && e.state == INPUT_PUSHED && e.isDefaultLeftButton())
	{
		logMsg("going up a dir");
		changeDirByInput("..", e);
	}
	else if(e.isPointer() && navV.viewRect.overlaps(e.x, e.y) && !tbl.scroll.active)
	{
		navV.inputEvent(e);
		return;
	}
	else
	{
		tbl.inputEvent(e);
	}
}

void FSPicker::draw()
{
	using namespace Gfx;
	setColor(COLOR_WHITE);
	tbl.draw();
	/*gfx_resetTransforms();
	gfx_setBlendMode(GFX_BLEND_MODE_ALPHA);
	gfx_setColor(1., 0, 0, .25);
	GeomRect::draw(&tableFrame, 1.);*/
	navV.draw();
}

void FSPicker::drawElement(const GuiTable1D *table, uint i, Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const
{
	text[i].draw(xPos, yPos, xSize, ySize, align);
}

void FSPicker::onSelectElement(const GuiTable1D *table, const InputEvent &e, uint i)
{
	assert(i < dir.numEntries());
	if(FsSys::fileType(dir.entryFilename(i)) == Fs::TYPE_DIR)
	{
		assert(!singleDir);
		logMsg("going to dir %s", dir.entryFilename(i));
		changeDirByInput(dir.entryFilename(i), e);
	}
	else
	{
		//onSelectFile(dir.entryFilename(i));
		onSelectFile.invokeSafe(dir.entryFilename(i), e);
	}
}

void FSPicker::loadDir(const char *path)
{
	assert(path);
	FsSys::chdir(path);
	dir.openDir(".", 0, filter);
	logMsg("%d entries", dir.numEntries());
	if(dir.numEntries())
	{
		// TODO free old pointer on failure
		text = realloc(text, dir.numEntries());
		if(text == 0)
		{
			logMsg("out of memory loading directory");
			Base::exit(); // TODO: handle without exiting
		}
		iterateTimes(dir.numEntries(), i)
		{
			text[i].init(dir.entryFilename(i), 1, faceRes);
		}
	}
	tbl.init(this, dir.numEntries());
	#if defined(CONFIG_BASE_IOS) && !defined(CONFIG_BASE_IOS_JB)
	navV.setTitle("Documents");
	#else
	navV.setTitle(FsSys::workDir());
	#endif
}

#undef thisModuleName
