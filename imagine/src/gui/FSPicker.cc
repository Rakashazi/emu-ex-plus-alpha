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
	{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	{ .03, Gfx::VertexColorPixelFormat.build(1. * .4, 1. * .4, 1. * .4, 1.) },
	{ .3, Gfx::VertexColorPixelFormat.build(1. * .4, 1. * .4, 1. * .4, 1.) },
	{ .97, Gfx::VertexColorPixelFormat.build(.35 * .4, .35 * .4, .35 * .4, 1.) },
	{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
};

// FSNavView

void FSPicker::FSNavView::init(ResourceFace *face, Gfx::PixmapTexture *backRes, Gfx::PixmapTexture *closeRes, bool singleDir)
{
	if(singleDir)
		backRes = nullptr;
	BasicNavView::init(face, backRes, closeRes, fsNavViewGrad, sizeofArray(fsNavViewGrad));
	//logMsg("has back:%p close:%p", leftSpr.img, rightSpr.img);
}

void FSPicker::FSNavView::draw(const Base::Window &win, const Gfx::ProjectionPlane &projP)
{
	using namespace Gfx;
	setBlendMode(0);
	noTexProgram.use(projP.makeTranslate());
	bg.draw();
	setColor(COLOR_WHITE);
	texAlphaReplaceProgram.use();
	if(text.xSize > projP.unprojectXSize(textRect) - TableView::globalXIndent*2)
	{
		setClipRectBounds(win, textRect);
		setClipRect(1);
		text.draw(projP.unProjectRect(textRect).pos(RC2DO) - GP{TableView::globalXIndent, 0}, RC2DO, projP);
		setClipRect(0);
	}
	else
	{
		text.draw(projP.unProjectRect(textRect).pos(LC2DO) + GP{TableView::globalXIndent, 0}, LC2DO, projP);
	}
	if(leftSpr.image())
	{
		if(leftBtnActive)
		{
			setColor(COLOR_WHITE);
			setBlendMode(BLEND_MODE_ALPHA);
			TextureSampler::bindDefaultNearestMipClampSampler();
			leftSpr.useDefaultProgram(IMG_MODE_MODULATE, projP.makeTranslate(projP.unProjectRect(leftBtn).pos(C2DO)));
			leftSpr.draw();
		}
	}
	if(rightSpr.image())
	{
		if(rightBtnActive)
		{
			setColor(COLOR_WHITE);
			setBlendMode(BLEND_MODE_ALPHA);
			TextureSampler::bindDefaultNearestMipClampSampler();
			rightSpr.useDefaultProgram(IMG_MODE_MODULATE, projP.makeTranslate(projP.unProjectRect(rightBtn).pos(C2DO)));
			rightSpr.draw();
		}
	}
}

// FSPicker

void FSPicker::init(const char *path, Gfx::PixmapTexture *backRes, Gfx::PixmapTexture *closeRes, FsDirFilterFunc filter,  bool singleDir, ResourceFace *face)
{
	deinit();
	#ifdef CONFIG_BASE_IOS
	if(!Base::isSystemApp())
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
	navV.deinit();
	tbl.deinit();
	if(text)
	{
		mem_free(text);
		text = nullptr;
	}
	if(textPtr)
	{
		mem_free(textPtr);
		textPtr = nullptr;
	}
}

void FSPicker::place()
{
	navV.viewRect.setPosRel({viewFrame.x, viewFrame.y}, {viewFrame.xSize(), int(faceRes->nominalHeight() * 1.75)}, LT2DO);
	IG::WindowRect tableFrame = viewFrame;
	tableFrame.setYPos(navV.viewRect.yPos(LB2DO));
	tableFrame.y2 -= navV.viewRect.ySize();
	tbl.setViewRect(tableFrame, projP);
	tbl.place();
	navV.place(projP);
}

void FSPicker::changeDirByInput(const char *path, const Input::Event &e)
{
	loadDir(path);
	if(!e.isPointer())
		tbl.highlightFirstCell();
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
	else if(e.isPointer() && navV.viewRect.overlaps({e.x, e.y}) && !tbl.isDoingScrollGesture())
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
	tbl.draw();
	navV.draw(window(), projP);
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
		textPtr = mem_newRealloc(textPtr, dir.numEntries());
		if(!textPtr)
		{
			logMsg("out of memory loading directory");
			Base::abort(); // TODO: handle without exiting
		}
		iterateTimes(dir.numEntries(), i)
		{
			text[i].init(dir.entryFilename(i), 1, faceRes);
			textPtr[i] = &text[i];
			if(FsSys::fileType(dir.entryFilename(i)) == Fs::TYPE_DIR)
			{
				text[i].onSelect() = [this, i](TextMenuItem &, View &, const Input::Event &e)
					{
						assert(!singleDir);
						logMsg("going to dir %s", dir.entryFilename(i));
						changeDirByInput(dir.entryFilename(i), e);
					};
			}
			else
			{
				text[i].onSelect() = [this, i](TextMenuItem &, View &, const Input::Event &e)
					{
						onSelectFileD(*this, dir.entryFilename(i), e);
					};
			}
		}
	}
	tbl.init(textPtr, dir.numEntries(), false); // TODO: highlight first cell
	#ifdef CONFIG_BASE_IOS
	if(!Base::isSystemApp())
		navV.setTitle("Documents");
	else
	#endif
		navV.setTitle(FsSys::workDir());
}
