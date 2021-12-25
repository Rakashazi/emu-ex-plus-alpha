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
#include <imagine/gui/TextEntry.hh>
#include <imagine/gui/NavView.hh>
#include <imagine/fs/FS.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/math/int.hh>
#include <imagine/util/format.hh>
#include <imagine/util/string.h>
#include <string>

FSPicker::FSPicker(ViewAttachParams attach, Gfx::TextureSpan backRes, Gfx::TextureSpan closeRes,
	FilterFunc filter, Mode mode, Gfx::GlyphTextureSet *face_):
	View{attach},
	filter{filter},
	onClose_
	{
		[](FSPicker &picker, Input::Event e)
		{
			picker.dismiss();
		}
	},
	msgText{face_ ? face_ : &defaultFace()},
	mode_{mode}
{
	const Gfx::LGradientStopDesc fsNavViewGrad[]
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build(1. * .4, 1. * .4, 1. * .4, 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build(1. * .4, 1. * .4, 1. * .4, 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build(.35 * .4, .35 * .4, .35 * .4, 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};
	auto nav = makeView<BasicNavView>
		(
			&face(),
			isSingleDirectoryMode() ? nullptr : backRes,
			closeRes
		);
	nav->setBackgroundGradient(fsNavViewGrad);
	nav->setCenterTitle(false);
	nav->setOnPushLeftBtn(
		[this](Input::Event e)
		{
			onLeftNavBtn(e);
		});
	nav->setOnPushRightBtn(
		[this](Input::Event e)
		{
			onRightNavBtn(e);
		});
	nav->setOnPushMiddleBtn(
		[this](Input::Event e)
		{
			pushFileLocationsView(e);
		});
	controller.setNavView(std::move(nav));
	controller.push(makeView<TableView>([&d = dir](const TableView &) { return d.size(); },
		[&d = dir](const TableView &, size_t idx) -> MenuItem& { return d[idx].text; }));
	controller.navView()->showLeftBtn(true);
}

void FSPicker::place()
{
	controller.place(viewRect(), projP);
	msgText.compile(renderer(), projP);
}

std::error_code FSPicker::changeDirByInput(IG::CStringView path, FS::RootPathInfo rootInfo, Input::Event e)
{
	auto ec = setPath(path, rootInfo, e);
	place();
	postDraw();
	return ec;
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
	if(!isAtRoot())
	{
		goUpDirectory(e);
	}
	else
	{
		pushFileLocationsView(e);
	}
}

void FSPicker::onRightNavBtn(Input::Event e)
{
	onClose_.callCopy(*this, e);
}

bool FSPicker::inputEvent(Input::Event e)
{
	if(e.isDefaultCancelButton() && e.pushed())
	{
		onRightNavBtn(e);
		return true;
	}
	else if(controller.viewHasFocus() && e.pushed() && e.isDefaultLeftButton())
	{
		controller.moveFocusToNextView(e, CT2DO);
		controller.top().setFocus(false);
		return true;
	}
	else if(e.pushedKey(Input::Keycode::GAME_B) || e.pushedKey(Input::Keycode::F1))
	{
		pushFileLocationsView(e);
		return true;
	}
	return controller.inputEvent(e);
}

void FSPicker::prepareDraw()
{
	if(dir.size())
	{
		controller.top().prepareDraw();
	}
	else
	{
		msgText.makeGlyphs(renderer());
	}
	controller.navView()->prepareDraw();
}

void FSPicker::draw(Gfx::RendererCommands &cmds)
{
	if(dir.size())
	{
		controller.top().draw(cmds);
	}
	else
	{
		using namespace Gfx;
		cmds.set(ColorName::WHITE);
		cmds.setCommonProgram(CommonProgram::TEX_ALPHA, projP.makeTranslate());
		auto textRect = controller.top().viewRect();
		if(IG::isOdd(textRect.ySize()))
			textRect.y2--;
		msgText.draw(cmds, projP.unProjectRect(textRect).pos(C2DO), C2DO, projP);
	}
	controller.navView()->draw(cmds);
}

void FSPicker::onAddedToController(ViewController *, Input::Event e)
{
	controller.top().onAddedToController(&controller, e);
}

void FSPicker::setEmptyPath()
{
	logMsg("setting empty path");
	root = {};
	dir.clear();
	msgText.setString("No folder is set");
	if(mode_ == Mode::FILE_IN_DIR)
	{
		controller.top().setName({});
	}
	else
	{
		controller.top().setName("Select File Location");
	}
}

std::error_code FSPicker::setPath(IG::CStringView path, FS::RootPathInfo rootInfo, Input::Event e)
{
	if(!strlen(path))
	{
		setEmptyPath();
		return {};
	}
	auto prevPath = root.path;
	auto ctx = appContext();
	std::error_code ec{};
	try
	{
		root.path = path;
		dir.clear();
		ctx.forEachInDirectoryUri(path,
			[this](auto &entry)
			{
				//logMsg("entry:%s", entry.path().data());
				bool isDir = entry.type() == FS::file_type::directory;
				if(mode_ == Mode::DIR) // filter non-directories
				{
					if(!isDir)
						return true;
				}
				else if(mode_ == Mode::FILE_IN_DIR) // filter directories
				{
					if(isDir)
						return true;
				}
				if(!showHiddenFiles_ && entry.name().starts_with('.'))
				{
					return true;
				}
				if(filter && !filter(entry))
				{
					return true;
				}
				dir.emplace_back(FileEntry{std::string{entry.path()}, isDir, {entry.name(), &face(), nullptr}});
				return true;
			});
		std::sort(dir.begin(), dir.end(),
			[](const FileEntry &e1, const FileEntry &e2)
			{
				if(e1.isDir && !e2.isDir)
					return true;
				else if(!e1.isDir && e2.isDir)
					return false;
				else
					return IG::stringNoCaseLexCompare(e1.path, e2.path);
			});
		if(dir.size())
		{
			for(auto &d : dir)
			{
				if(d.isDir)
				{
					d.text.setOnSelect(
						[this, &dirPath = d.path](Input::Event e)
						{
							assert(!isSingleDirectoryMode());
							auto path = std::move(dirPath);
							logMsg("entering dir:%s", path.data());
							changeDirByInput(path, root.info, e);
						});
				}
				else
				{
					d.text.setOnSelect(
						[this, &dirPath = d.path](Input::Event e)
						{
							onSelectFile_.callCopy(*this, dirPath, appContext().fileUriDisplayName(dirPath), e);
						});
				}
			}
			msgText.setString({});
		}
		else // no entries, show a message instead
		{
			msgText.setString("Empty Directory");
		}
	}
	catch(std::system_error &err)
	{
		logErr("can't open %s", path.data());
		ec = err.code();
		std::string_view extraMsg = mode_ == Mode::FILE_IN_DIR ? "" : "\nPick a path from the top bar";
		msgText.setString(fmt::format("Can't open directory:\n{}{}", ec.message(), extraMsg));
	}
	if(!e.isPointer())
		static_cast<TableView*>(&controller.top())->highlightCell(0);
	else
		static_cast<TableView*>(&controller.top())->resetScroll();
	auto pathLen = path.size();
	// verify root info
	if(rootInfo.length && rootInfo.length > pathLen)
	{
		logWarn("invalid root length:%zu with path length:%zu", rootInfo.length, pathLen);
		rootInfo.length = 0;
	}
	// if the path is a URI and no root info is provided, root at the URI itself
	bool isUri = IG::isUri(path);
	if(!rootInfo.length && isUri)
	{
		rootInfo = {appContext().fileUriDisplayName(path), path.size()};
	}
	FS::PathString rootedPath{};
	if(rootInfo.length)
	{
		logMsg("root info:%d:%s", (int)rootInfo.length, rootInfo.name.data());
		root.info = rootInfo;
		if(pathLen > rootInfo.length)
			rootedPath = IG::format<FS::PathString>("{}{}", rootInfo.name, &path[rootInfo.length]);
		else
			rootedPath = rootInfo.name;
	}
	else
	{
		logMsg("no root info");
		root.info = {};
		rootedPath = root.path;
	}
	if(isUri)
	{
		rootedPath = IG::decodeUri<FS::PathString>(rootedPath);
	}
	controller.top().setName(rootedPath);
	onChangePath_.callSafe(*this, prevPath, e);
	return ec;
}

std::error_code FSPicker::setPath(IG::CStringView path, FS::RootPathInfo rootInfo)
{
	return setPath(path, rootInfo, appContext().defaultInputEvent());
}

std::error_code FSPicker::setPath(IG::CStringView path, Input::Event e)
{
	return setPath(path, appContext().rootPathInfo(path), e);
}

std::error_code FSPicker::setPath(IG::CStringView path)
{
	return setPath(path, appContext().rootPathInfo(path));
}

FS::PathString FSPicker::path() const
{
	return root.path;
}

FS::RootedPath FSPicker::rootedPath() const
{
	return root;
}

void FSPicker::clearSelection()
{
	controller.top().clearSelection();
}

bool FSPicker::isSingleDirectoryMode() const
{
	return mode_ == Mode::FILE_IN_DIR;
}

void FSPicker::goUpDirectory(Input::Event e)
{
	clearSelection();
	changeDirByInput(FS::dirnameUri(root.path), root.info, e);
}

bool FSPicker::isAtRoot() const
{
	if(root.info.length)
	{
		return root.pathIsRoot();
	}
	else
	{
		return root.path.empty() || root.path == "/";
	}
}

void FSPicker::pushFileLocationsView(Input::Event e)
{
	if(isSingleDirectoryMode())
		return;
	class FileLocationsTextTableView : public TextTableView
	{
	public:
		FileLocationsTextTableView(ViewAttachParams attach,
			std::vector<FS::PathLocation> locations, size_t customItems):
				TextTableView{"File Locations", attach, locations.size() + customItems},
				locations_{std::move(locations)} {}
		const std::vector<FS::PathLocation> &locations() const { return locations_; }

	protected:
		std::vector<FS::PathLocation> locations_;
	};

	int customItems = 1 + Config::envIsLinux + appContext().hasSystemPathPicker() + appContext().hasSystemDocumentPicker();
	auto view = makeView<FileLocationsTextTableView>(appContext().rootFileLocations(), customItems);
	for(auto &loc : view->locations())
	{
		view->appendItem(loc.description,
			[this, &loc](View &view, Input::Event e)
			{
				auto ctx = appContext();
				if(ctx.usesPermission(Base::Permission::WRITE_EXT_STORAGE))
				{
					if(!ctx.requestPermission(Base::Permission::WRITE_EXT_STORAGE))
						return;
				}
				changeDirByInput(loc.root.path, loc.root.info, e);
				view.dismiss();
			});
	}
	if(Config::envIsLinux)
	{
		view->appendItem("Root Filesystem",
			[this](View &view, Input::Event e)
			{
				changeDirByInput("/", {}, e);
				view.dismiss();
			});
	}
	if(appContext().hasSystemPathPicker())
	{
		view->appendItem("Browse For Folder",
			[this](View &view, Input::Event e)
			{
				appContext().showSystemPathPicker(
					[this, &view](IG::CStringView uri, IG::CStringView displayName)
					{
						view.dismiss();
						auto ec = changeDirByInput(uri, appContext().rootPathInfo(uri), appContext().defaultInputEvent());
						if(mode_ == Mode::DIR && !ec)
							onClose_.callCopy(*this, appContext().defaultInputEvent());
					});
			});
	}
	if(mode_ != Mode::DIR && appContext().hasSystemDocumentPicker())
	{
		view->appendItem("Browse For File",
			[this](View &view, Input::Event e)
			{
				appContext().showSystemDocumentPicker(
					[this, &view](IG::CStringView uri, IG::CStringView displayName)
					{
						onSelectFile_.callCopy(*this, uri, displayName, appContext().defaultInputEvent());
					});
			});
	}
	view->appendItem("Custom Path",
		[this](Input::Event e)
		{
			auto textInputView = makeView<CollectTextInputView>(
				"Input a directory path", root.path, nullptr,
				[this](CollectTextInputView &view, const char *str)
				{
					if(!str || !strlen(str))
					{
						view.dismiss();
						return false;
					}
					changeDirByInput(str, appContext().rootPathInfo(str), appContext().defaultInputEvent());
					dismissPrevious();
					view.dismiss();
					return false;
				});
			pushAndShow(std::move(textInputView), e);
		});
	pushAndShow(std::move(view), e);
}

Gfx::GlyphTextureSet &FSPicker::face()
{
	return *msgText.face();
}

void FSPicker::setShowHiddenFiles(bool on)
{
	showHiddenFiles_ = on;
}
