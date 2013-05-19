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

static const Gfx::LGradientStopDesc fsNavViewGrad[] =
{
	{ .0, VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	{ .03, VertexColorPixelFormat.build(1. * .4, 1. * .4, 1. * .4, 1.) },
	{ .3, VertexColorPixelFormat.build(1. * .4, 1. * .4, 1. * .4, 1.) },
	{ .97, VertexColorPixelFormat.build(.35 * .4, .35 * .4, .35 * .4, 1.) },
	{ 1., VertexColorPixelFormat.build(.5, .5, .5, 1.) },
};

// FSNavView

void FSPicker::FSNavView::init(ResourceFace *face, Gfx::BufferImage *backRes, Gfx::BufferImage *closeRes, bool singleDir)
{
	if(singleDir)
		backRes = 0;
	BasicNavView::init(face, backRes, closeRes, fsNavViewGrad, sizeofArray(fsNavViewGrad));
	//logMsg("has back:%p close:%p", leftSpr.img, rightSpr.img);
}

void FSPicker::FSNavView::draw()
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
	if(leftSpr.image())
	{
		if(leftBtnActive)
		{
			setColor(COLOR_WHITE);
			setBlendMode(BLEND_MODE_INTENSITY);
			loadTranslate(gXPos(leftBtn, C2DO), gYPos(leftBtn, C2DO));
			//applyRollRotate(angle_fromDegree(90));
			leftSpr.draw();
		}
	}
	if(rightSpr.image())
	{
		if(rightBtnActive)
		{
			setColor(COLOR_WHITE);
			setBlendMode(BLEND_MODE_INTENSITY);
			loadTranslate(gXPos(rightBtn, C2DO), gYPos(rightBtn, C2DO));
			rightSpr.draw();
		}
	}
}

void FSPicker::FSNavView::place()
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

void FSPicker::init(const char *path, Gfx::BufferImage *backRes, Gfx::BufferImage *closeRes, FsDirFilterFunc filter,  bool singleDir, ResourceFace *face)
{
	#if defined(CONFIG_BASE_IOS) && !defined(CONFIG_BASE_IOS_JB)
	singleDir = 1; // stay in Documents dir when not in jailbreak environment
	#endif
	text = 0;
	faceRes = face;
	var_selfs(filter);
	var_selfs(singleDir);
	navV.init(face, backRes, closeRes, singleDir);
	loadDir(path);
}

void FSPicker::deinit()
{
	dir.closeDir();
	delete[] text;
	navV.deinit();
	tbl.cells = 0;
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

void FSPicker::changeDirByInput(const char *path, const Input::Event &e)
{
	loadDir(path);
	if(!e.isPointer() && tbl.cells)
		tbl.selected = 0;
	place();
	Base::displayNeedsUpdate();
}

void FSPicker::onLeftNavBtn(const Input::Event &e)
{
	changeDirByInput("..", e);
}

void FSPicker::onRightNavBtn(const Input::Event &e)
{
	onCloseD(e);
}

void FSPicker::inputEvent(const Input::Event &e)
{
	if(e.isDefaultCancelButton() && e.state == Input::PUSHED)
	{
		onCloseD(e);
		return;
	}

	const char* dirChange = 0;
	if(!singleDir && e.state == Input::PUSHED && e.isDefaultLeftButton())
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

void FSPicker::draw(Gfx::FrameTimeBase frameTime)
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

void FSPicker::onSelectElement(const GuiTable1D *table, const Input::Event &e, uint i)
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
		onSelectFileD(dir.entryFilename(i), e);
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
		if(!text)
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
