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

#define LOGTAG "FSPicker"

#include <imagine/gui/FSPicker.hh>

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
		backRes = nullptr;
	BasicNavView::init(face, backRes, closeRes, fsNavViewGrad, sizeofArray(fsNavViewGrad));
	//logMsg("has back:%p close:%p", leftSpr.img, rightSpr.img);
}

void FSPicker::FSNavView::draw(const Base::Window &win)
{
	using namespace Gfx;
	setBlendMode(0);
	noTexProgram.use(View::projP.makeTranslate());
	bg.draw();
	setColor(COLOR_WHITE);
	texAlphaReplaceProgram.use();
	if(text.xSize > projP.unprojectXSize(textRect) - GuiTable1D::globalXIndent*2)
	{
		setClipRectBounds(win, textRect);
		setClipRect(1);
		text.draw(projP.unProjectRect(textRect).pos(RC2DO) - GP{GuiTable1D::globalXIndent, 0}, RC2DO);
		setClipRect(0);
	}
	else
	{
		text.draw(projP.unProjectRect(textRect).pos(LC2DO) + GP{GuiTable1D::globalXIndent, 0}, LC2DO);
	}
	if(leftSpr.image())
	{
		if(leftBtnActive)
		{
			setColor(COLOR_WHITE);
			setBlendMode(BLEND_MODE_ALPHA);
			leftSpr.useDefaultProgram(IMG_MODE_MODULATE, View::projP.makeTranslate(projP.unProjectRect(leftBtn).pos(C2DO)));
			leftSpr.draw();
		}
	}
	if(rightSpr.image())
	{
		if(rightBtnActive)
		{
			setColor(COLOR_WHITE);
			setBlendMode(BLEND_MODE_ALPHA);
			rightSpr.useDefaultProgram(IMG_MODE_MODULATE, View::projP.makeTranslate(projP.unProjectRect(rightBtn).pos(C2DO)));
			rightSpr.draw();
		}
	}
}

// FSPicker

void FSPicker::init(const char *path, Gfx::BufferImage *backRes, Gfx::BufferImage *closeRes, FsDirFilterFunc filter,  bool singleDir, ResourceFace *face)
{
	deinit();
	#if defined(CONFIG_BASE_IOS) && !defined(CONFIG_BASE_IOS_JB)
	singleDir = 1; // stay in Documents dir when not in jailbreak environment
	#endif
	faceRes = face;
	var_selfs(filter);
	var_selfs(singleDir);
	navV.init(face, backRes, closeRes, singleDir);
	loadDir(path);
}

void FSPicker::deinit()
{
	dir.closeDir();
	if(text)
	{
		mem_free(text);
		text = nullptr;
	}
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
	navV.viewRect.setPosRel({viewFrame.x, viewFrame.y}, {viewFrame.xSize(), int(faceRes->nominalHeight() * 1.75)}, LT2DO);
	IG::WindowRect tableFrame = viewFrame;
	tableFrame.setYPos(navV.viewRect.yPos(LB2DO));
	tableFrame.y2 -= navV.viewRect.ySize();
	tbl.place(&tableFrame, *this);
	//logMsg("nav %d, table %d, content %d", gfx_toIYSize(navV.view.ySize), tbl.viewFrame.ySize(), tbl.contentFrame->ySize());

	navV.place();
}

void FSPicker::changeDirByInput(const char *path, const Input::Event &e)
{
	loadDir(path);
	if(!e.isPointer() && tbl.cells)
		tbl.selected = 0;
	place();
	postDraw();
}

void FSPicker::onLeftNavBtn(const Input::Event &e)
{
	changeDirByInput("..", e);
}

void FSPicker::onRightNavBtn(const Input::Event &e)
{
	onCloseD(*this, e);
}

void FSPicker::inputEvent(const Input::Event &e)
{
	if(e.isDefaultCancelButton() && e.state == Input::PUSHED)
	{
		onCloseD(*this, e);
		return;
	}

	const char* dirChange = 0;
	if(!singleDir && e.state == Input::PUSHED && e.isDefaultLeftButton())
	{
		logMsg("going up a dir");
		changeDirByInput("..", e);
	}
	else if(e.isPointer() && navV.viewRect.overlaps({e.x, e.y}) && !tbl.scroll.active)
	{
		navV.inputEvent(e);
		return;
	}
	else
	{
		tbl.inputEvent(e, *this);
	}
}

void FSPicker::draw(Base::FrameTimeBase frameTime)
{
	using namespace Gfx;
	setColor(COLOR_WHITE);
	tbl.draw(*this);
	navV.draw(window());
}

void FSPicker::drawElement(const GuiTable1D &table, uint i, Gfx::GCRect rect) const
{
	using namespace Gfx;
	setColor(COLOR_WHITE);
	text[i].draw(rect.x, rect.pos(C2DO).y, rect.xSize(), rect.ySize(), LC2DO);
}

void FSPicker::onSelectElement(const GuiTable1D *table, const Input::Event &e, uint i)
{
	assert(i < dir.numEntries());
	text[i].select(this, e);
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
		text = mem_newRealloc(text, dir.numEntries());
		if(!text)
		{
			logMsg("out of memory loading directory");
			Base::abort(); // TODO: handle without exiting
		}
		iterateTimes(dir.numEntries(), i)
		{
			text[i].init(dir.entryFilename(i), 1, faceRes);
			if(FsSys::fileType(dir.entryFilename(i)) == Fs::TYPE_DIR)
			{
				text[i].onSelect() = [this, i](TextMenuItem &item, const Input::Event &e)
					{
						assert(!singleDir);
						logMsg("going to dir %s", dir.entryFilename(i));
						changeDirByInput(dir.entryFilename(i), e);
					};
			}
			else
			{
				text[i].onSelect() = [this, i](TextMenuItem &item, const Input::Event &e)
					{
						onSelectFileD(*this, dir.entryFilename(i), e);
					};
			}
		}
	}
	tbl.init(this, dir.numEntries(), *this);
	#if defined(CONFIG_BASE_IOS) && !defined(CONFIG_BASE_IOS_JB)
	navV.setTitle("Documents");
	#else
	navV.setTitle(FsSys::workDir());
	#endif
}
