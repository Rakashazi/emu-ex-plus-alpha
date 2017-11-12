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
#include <imagine/gui/TextTableView.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/math/int.hh>
#include <imagine/util/string.h>
#include <string>

static bool isValidRootEndChar(char c)
{
	return c == '/' || c == '\0';
}

FSPicker::FSPicker(ViewAttachParams attach, Gfx::PixmapTexture *backRes, Gfx::PixmapTexture *closeRes,
	FilterFunc filter,  bool singleDir, Gfx::GlyphTextureSet *face):
	View{attach},
	filter{filter},
	tbl{attach, text},
	faceRes{face},
	navV{attach.renderer, face, singleDir ? nullptr : backRes, closeRes},
	singleDir{singleDir}
{
	msgText = {msgStr.data(), face};
	const Gfx::LGradientStopDesc fsNavViewGrad[]
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build(1. * .4, 1. * .4, 1. * .4, 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build(1. * .4, 1. * .4, 1. * .4, 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build(.35 * .4, .35 * .4, .35 * .4, 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};
	navV.setBackgroundGradient(fsNavViewGrad);
	navV.centerTitle = false;
	navV.setOnPushLeftBtn(
		[this](Input::Event e)
		{
			onLeftNavBtn(e);
		});
	navV.setOnPushRightBtn(
		[this](Input::Event e)
		{
			onRightNavBtn(e);
		});
	navV.setOnPushMiddleBtn(
		[this](Input::Event e)
		{
			if(!this->singleDir)
			{
				pushFileLocationsView(e);
			}
		});
}

void FSPicker::place()
{
	navV.viewRect().setPosRel({viewFrame.x, viewFrame.y}, {viewFrame.xSize(), int(faceRes->nominalHeight() * 1.75)}, LT2DO);
	IG::WindowRect tableFrame = viewFrame;
	tableFrame.setYPos(navV.viewRect().yPos(LB2DO));
	tableFrame.y2 -= navV.viewRect().ySize();
	tbl.setViewRect(tableFrame, projP);
	tbl.place();
	navV.place(renderer(), projP);
	msgText.compile(renderer(), projP);
}

void FSPicker::changeDirByInput(const char *path, FS::RootPathInfo rootInfo, bool forcePathChange, Input::Event e)
{
	auto ec = setPath(path, forcePathChange, rootInfo, e);
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
	changeDirByInput(FS::dirname(currPath).data(), root, true, e);
}

void FSPicker::onRightNavBtn(Input::Event e)
{
	onClose_.callCopy(*this, e);
}

void FSPicker::setOnPathReadError(OnPathReadError del)
{
	onPathReadError_ = del;
}

bool FSPicker::inputEvent(Input::Event e)
{
	if(e.isDefaultCancelButton() && e.state() == Input::PUSHED)
	{
		onClose_.callCopy(*this, e);
		return true;
	}
	if(!singleDir && e.state() == Input::PUSHED && e.isDefaultLeftButton())
	{
		logMsg("going up a dir");
		changeDirByInput(FS::dirname(currPath).data(), root, true, e);
		return true;
	}
	else if(!singleDir && (e.pushedKey(Input::Keycode::GAME_B) || e.pushedKey(Input::Keycode::F1)))
	{
		pushFileLocationsView(e);
		return true;
	}
	else if(e.isPointer() && navV.viewRect().overlaps(e.pos()) && !tbl.isDoingScrollGesture())
	{
		return navV.inputEvent(e);
	}
	else
	{
		return tbl.inputEvent(e);
	}
	return false;
}

void FSPicker::draw()
{
	auto &r = renderer();
	if(dir.size())
	{
		tbl.draw();
	}
	else
	{
		using namespace Gfx;
		r.setColor(COLOR_WHITE);
		r.texAlphaProgram.use(r, projP.makeTranslate());
		auto textRect = tbl.viewRect();
		if(IG::isOdd(textRect.ySize()))
			textRect.y2--;
		msgText.draw(r, projP.unProjectRect(textRect).pos(C2DO), C2DO, projP);
	}
	navV.draw(r, window(), projP);
}

void FSPicker::onAddedToController(Input::Event e)
{
	tbl.onAddedToController(e);
}

std::error_code FSPicker::setPath(const char *path, bool forcePathChange, FS::RootPathInfo rootInfo, Input::Event e)
{
	assert(path);
	auto prevPath = currPath;
	std::error_code ec{};
	{
		auto dirIt = FS::directory_iterator{path, ec};
		if(ec)
		{
			logErr("can't open %s", path);
			if(!forcePathChange)
			{
				onPathReadError_.callSafe(*this, ec);
				return ec;
			}
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
		msgStr = {};
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
						changeDirByInput(filePath.data(), root, false, e);
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
	else
	{
		// no entires, show a message instead
		if(ec)
			string_printf(msgStr, "Can't open directory:\n%s", ec.message().c_str());
		else
			string_copy(msgStr, "Empty Directory");
	}
	if(!e.isPointer())
		tbl.highlightCell(0);
	else
		tbl.resetScroll();
	uint pathLen = strlen(path);
	// verify root info
	if(rootInfo.length &&
		(rootInfo.length > pathLen || !isValidRootEndChar(path[rootInfo.length]) || !strlen(rootInfo.name.data())))
	{
		logWarn("invalid root parameters");
		rootInfo.length = 0;
	}
	if(rootInfo.length)
	{
		logMsg("root info:%d:%s", (int)rootInfo.length, rootInfo.name.data());
		root = rootInfo;
		if(pathLen > rootInfo.length)
			rootedPath = FS::makePathStringPrintf("%s%s", rootInfo.name.data(), &path[rootInfo.length]);
		else
			rootedPath = FS::makePathString(rootInfo.name.data());
	}
	else
	{
		logMsg("no root info");
		root = {};
		rootedPath = currPath;
	}
	navV.showLeftBtn(!isAtRoot());
	navV.setTitle(rootedPath.data());
	onChangePath_.callSafe(*this, prevPath, e);
	return {};
}

std::error_code FSPicker::setPath(const char *path, bool forcePathChange, FS::RootPathInfo rootInfo)
{
	return setPath(path, forcePathChange, rootInfo, Input::defaultEvent());
}

FS::PathString FSPicker::path() const
{
	return currPath;
}

FS::PathString FSPicker::makePathString(const char *base) const
{
	return FS::makePathString(strlen(currPath.data()) > 1 ? currPath.data() : "", base);
}

bool FSPicker::isAtRoot() const
{
	if(root.length)
	{
		auto pathLen = strlen(currPath.data());
		assumeExpr(pathLen >= root.length);
		return pathLen == root.length;
	}
	else
	{
		return string_equal(currPath.data(), "/");
	}
}

void FSPicker::pushFileLocationsView(Input::Event e)
{
	rootLocation = Base::rootFileLocations();
	auto &view = *new TextTableView("File Locations", attachParams(), rootLocation.size() + 1);
	for(auto &loc : rootLocation)
	{
		view.appendItem(loc.description.data(), [this, &loc](TextMenuItem &, View &, Input::Event e)
			{
				auto pathLen = strlen(loc.path.data());
				changeDirByInput(loc.path.data(), loc.root, true, e);
				popAndShow();
			});
	}
	view.appendItem("Root Filesystem", [this](TextMenuItem &, View &, Input::Event e)
		{
			changeDirByInput("/", {}, true, e);
			popAndShow();
		});
	pushAndShow(view, e);
}
