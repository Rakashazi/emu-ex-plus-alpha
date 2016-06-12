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

// FSNavView

void FSPicker::FSNavView::init(ResourceFace *face, Gfx::PixmapTexture *backRes, Gfx::PixmapTexture *closeRes, bool singleDir)
{
	if(singleDir)
		backRes = nullptr;
	BasicNavView::init(face, backRes, closeRes);
	//logMsg("has back:%p close:%p", leftSpr.img, rightSpr.img);
	const Gfx::LGradientStopDesc fsNavViewGrad[]
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build(1. * .4, 1. * .4, 1. * .4, 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build(1. * .4, 1. * .4, 1. * .4, 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build(.35 * .4, .35 * .4, .35 * .4, 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};
	setBackgroundGradient(fsNavViewGrad);
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

FSPicker::FSPicker(Base::Window &win, Gfx::PixmapTexture *backRes, Gfx::PixmapTexture *closeRes,
	FilterFunc filter,  bool singleDir, ResourceFace *face):
	View{win},
	filter{filter},
	tbl{win, text},
	faceRes{face},
	singleDir{singleDir}
{
	navV.init(face, backRes, closeRes, singleDir);
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

void FSPicker::changeDirByInput(const char *path, bool forcePathChange, Input::Event e)
{
	if(setPath(path, forcePathChange, e) != OK && !forcePathChange)
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
	changeDirByInput("..", true, e);
}

void FSPicker::onRightNavBtn(Input::Event e)
{
	onCloseD.callCopy(*this, e);
}

void FSPicker::setOnPathReadError(OnPathReadError del)
{
	onPathReadError = del;
}

void FSPicker::inputEvent(Input::Event e)
{
	if(e.isDefaultCancelButton() && e.state == Input::PUSHED)
	{
		onCloseD.callCopy(*this, e);
		return;
	}

	const char* dirChange = 0;
	if(!singleDir && e.state == Input::PUSHED && e.isDefaultLeftButton())
	{
		logMsg("going up a dir");
		changeDirByInput("..", true, e);
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

CallResult FSPicker::setPath(const char *path, bool forcePathChange, Input::Event e)
{
	assert(path);
	CallResult res = OK;
	{
		auto dirIt = FS::directory_iterator{path, res};
		if(res != OK)
		{
			logErr("can't open %s", path);
			onPathReadError.callSafe(*this, res);
			if(!forcePathChange)
				return res;
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
	text.clear();
	if(dir.size())
	{
		text.reserve(dir.size());
		iterateTimes(dir.size(), i)
		{
			bool isDir = FS::status(dir[i].data()).type() == FS::file_type::directory;
			if(isDir)
			{
				text.emplace_back(dir[i].data(),
					[this, i](TextMenuItem &, View &, Input::Event e)
					{
						assert(!singleDir);
						logMsg("going to dir %s", dir[i].data());
						changeDirByInput(dir[i].data(), false, e);
					});
			}
			else
			{
				text.emplace_back(dir[i].data(),
					[this, i](TextMenuItem &, View &, Input::Event e)
					{
						onSelectFileD.callCopy(*this, dir[i].data(), e);
					});
			}
		}
	}
	if(!e.isPointer())
		tbl.highlightCell(0);
	else
		tbl.resetScroll();
	navV.setTitle(FS::current_path().data());
	return res;
}

CallResult FSPicker::setPath(const char *path, bool forcePathChange)
{
	return setPath(path, forcePathChange, Input::defaultEvent());
}
