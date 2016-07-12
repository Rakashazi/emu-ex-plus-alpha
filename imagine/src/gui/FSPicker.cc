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
	NavView::setTitle(str);
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
	auto ec = setPath(path, forcePathChange, e);
	if(ec && !forcePathChange)
		return;
	place();
	postDraw();
}

void FSPicker::setOnChangePath(OnChangePathDelegate del)
{
	onChangePath_ = del;
}

void FSPicker::setOnSelectFile(OnSelectFileDelegate del)
{
	onSelectFile_ = del;
}

void FSPicker::setOnClose(OnCloseDelegate del)
{
	onClose_ = del;
}

void FSPicker::onLeftNavBtn(Input::Event e)
{
	changeDirByInput(FS::dirname(currPath).data(), true, e);
}

void FSPicker::onRightNavBtn(Input::Event e)
{
	onClose_.callCopy(*this, e);
}

void FSPicker::setOnPathReadError(OnPathReadError del)
{
	onPathReadError_ = del;
}

void FSPicker::inputEvent(Input::Event e)
{
	if(e.isDefaultCancelButton() && e.state == Input::PUSHED)
	{
		onClose_.callCopy(*this, e);
		return;
	}

	const char* dirChange = 0;
	if(!singleDir && e.state == Input::PUSHED && e.isDefaultLeftButton())
	{
		logMsg("going up a dir");
		changeDirByInput(FS::dirname(currPath).data(), true, e);
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

std::error_code FSPicker::setPath(const char *path, bool forcePathChange, Input::Event e)
{
	assert(path);
	{
		std::error_code ec{};
		auto dirIt = FS::directory_iterator{path, ec};
		if(ec)
		{
			logErr("can't open %s", path);
			onPathReadError_.callSafe(*this, ec);
			if(!forcePathChange)
				return ec;
		}
		string_copy(currPath, path);
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
			auto filePath = makePathString(dir[i].data());
			bool isDir = FS::status(filePath.data()).type() == FS::file_type::directory;
			if(isDir)
			{
				text.emplace_back(dir[i].data(),
					[this, i](TextMenuItem &, View &, Input::Event e)
					{
						assert(!singleDir);
						auto filePath = makePathString(dir[i].data());
						logMsg("going to dir %s", filePath.data());
						changeDirByInput(filePath.data(), false, e);
					});
			}
			else
			{
				text.emplace_back(dir[i].data(),
					[this, i](TextMenuItem &, View &, Input::Event e)
					{
						onSelectFile_.callCopy(*this, dir[i].data(), e);
					});
			}
		}
	}
	if(!e.isPointer())
		tbl.highlightCell(0);
	else
		tbl.resetScroll();
	navV.setTitle(currPath.data());
	onChangePath_.callSafe(*this, currPath, e);
	return {};
}

std::error_code FSPicker::setPath(const char *path, bool forcePathChange)
{
	return setPath(path, forcePathChange, Input::defaultEvent());
}

FS::PathString FSPicker::path() const
{
	return currPath;
}

FS::PathString FSPicker::makePathString(const char *base) const
{
	return FS::makePathString(currPath.data(), base);
}
