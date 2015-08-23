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
#include <imagine/logger/logger.h>

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

void FSPicker::FSNavView::setTitle(const char *str)
{
	string_copy(titleStr, str);
	NavView::setTitle(titleStr.data());
}

// FSPicker

void FSPicker::init(Gfx::PixmapTexture *backRes, Gfx::PixmapTexture *closeRes, FilterFunc filter,  bool singleDir, ResourceFace *face)
{
	deinit();
	faceRes = face;
	var_selfs(filter);
	var_selfs(singleDir);
	navV.init(face, backRes, closeRes, singleDir);
}

void FSPicker::deinit()
{
	dir.clear();
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

void FSPicker::changeDirByInput(const char *path, Input::Event e)
{
	if(setPath(path, e) != OK)
		return;
	place();
	postDraw();
}

void FSPicker::setOnSelectFile(OnSelectFileDelegate del)
{
	onSelectFileD = del;
}

void FSPicker::setOnClose(OnCloseDelegate del)
{
	onCloseD = del;
}

void FSPicker::onLeftNavBtn(Input::Event e)
{
	changeDirByInput("..", e);
}

void FSPicker::onRightNavBtn(Input::Event e)
{
	onCloseD(*this, e);
}

void FSPicker::setOnPathReadError(OnPathReadError del)
{
	onPathReadError = del;
}

void FSPicker::inputEvent(Input::Event e)
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

void FSPicker::onAddedToController(Input::Event e)
{
	tbl.onAddedToController(e);
}

CallResult FSPicker::setPath(const char *path, Input::Event e)
{
	assert(path);
	{
		CallResult dirResult = OK;
		auto dirIt = FS::directory_iterator{path, dirResult};
		if(dirResult != OK)
		{
			logErr("can't open %s", path);
			onPathReadError.callSafe(*this, dirResult);
			return dirResult;
		}
		FS::current_path(path);
		dir.clear();
		for(auto &entry : dirIt)
		{
			if(filter && !filter(entry))
			{
				continue;
			}
			dir.emplace_back(FS::makeFileString(entry.name()));
		}
	}
	std::sort(dir.begin(), dir.end(), FS::fileStringNoCaseLexCompare());
	if(dir.size())
	{
		// TODO free old pointer on failure
		text = mem_newRealloc(text, dir.size());
		if(!text)
		{
			logMsg("out of memory loading directory");
			Base::abort(); // TODO: handle without exiting
		}
		textPtr = mem_newRealloc(textPtr, dir.size());
		if(!textPtr)
		{
			logMsg("out of memory loading directory");
			Base::abort(); // TODO: handle without exiting
		}
		iterateTimes(dir.size(), i)
		{
			text[i].init(dir[i].data(), 1, faceRes);
			textPtr[i] = &text[i];
			if(FS::status(dir[i].data()).type() == FS::file_type::directory)
			{
				text[i].onSelect() = [this, i](TextMenuItem &, View &, Input::Event e)
					{
						assert(!singleDir);
						logMsg("going to dir %s", dir[i].data());
						changeDirByInput(dir[i].data(), e);
					};
			}
			else
			{
				text[i].onSelect() = [this, i](TextMenuItem &, View &, Input::Event e)
					{
						onSelectFileD(*this, dir[i].data(), e);
					};
			}
		}
	}
	else
	{
		mem_free(text);
		text = nullptr;
		mem_free(textPtr);
		textPtr = nullptr;
	}
	tbl.init(textPtr, dir.size());
	if(!e.isPointer())
		tbl.highlightCell(0);
	navV.setTitle(FS::current_path().data());
	return OK;
}

CallResult FSPicker::setPath(const char *path)
{
	return setPath(path, Input::defaultEvent());
}
