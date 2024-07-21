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

#include <imagine/gui/FSPicker.hh>
#include <imagine/gui/TextTableView.hh>
#include <imagine/gui/TextEntry.hh>
#include <imagine/gui/NavView.hh>
#include <imagine/fs/FS.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/BasicEffect.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/math.hh>
#include <imagine/util/format.hh>
#include <imagine/util/string.h>
#include <string>
#include <system_error>

namespace IG
{

constexpr SystemLogger log{"FSPicker"};

FSPicker::FSPicker(ViewAttachParams attach, Gfx::TextureSpan backRes, Gfx::TextureSpan closeRes,
	FilterFunc filter, Mode mode, Gfx::GlyphTextureSet *face_):
	View{attach},
	filter{filter},
	controller{attach},
	msgText{attach.rendererTask, face_ ? face_ : &defaultFace()},
	dirListEvent{{.debugLabel = "FSPicker::dirListEvent", .eventLoop = EventLoop::forThread()}, {}},
	mode_{mode}
{
	auto nav = makeView<BasicNavView>
		(
			&face(),
			isSingleDirectoryMode() ? Gfx::TextureSpan{} : backRes,
			closeRes
		);
	const Gfx::LGradientStopDesc fsNavViewGrad[]
	{
		{ .0, Gfx::PackedColor::format.build(1. * .4, 1. * .4, 1. * .4, 1.) },
		{ .3, Gfx::PackedColor::format.build(1. * .4, 1. * .4, 1. * .4, 1.) },
		{ .97, Gfx::PackedColor::format.build(.35 * .4, .35 * .4, .35 * .4, 1.) },
		{ 1., nav->separatorColor() },
	};
	nav->setBackgroundGradient(fsNavViewGrad);
	nav->setCenterTitle(false);
	nav->setOnPushLeftBtn(
		[this](const Input::Event &e)
		{
			onLeftNavBtn(e);
		});
	nav->setOnPushRightBtn(
		[this](const Input::Event &e)
		{
			onRightNavBtn(e);
		});
	nav->setOnPushMiddleBtn(
		[this](const Input::Event &e)
		{
			pushFileLocationsView(e);
		});
	controller.setNavView(std::move(nav));
	controller.push(makeView<TableView>(dir));
	controller.navView()->showLeftBtn(true);
	dir.reserve(16); // start with some initial capacity to avoid small reallocations
}

void FSPicker::place()
{
	controller.place(viewRect(), displayRect());
	if(dirListThread.isWorking())
		return;
	msgText.compile();
}

void FSPicker::changeDirByInput(CStringView path, FS::RootPathInfo rootInfo, const Input::Event &e,
	DepthMode depthMode)
{
	newFileUIState = {};
	if(depthMode == DepthMode::reset)
	{
		fileUIStates.clear();
	}
	else if(depthMode == DepthMode::decrement)
	{
		if(fileUIStates.size())
		{
			newFileUIState = fileUIStates.back();
			fileUIStates.pop_back();
		}
	}
	else // increment
	{
		fileUIStates.push_back(fileTableView().saveUIState());
	}
	setPath(path, std::move(rootInfo), e);
	place();
	postDraw();
}

void FSPicker::setOnChangePath(OnChangePathDelegate del)
{
	onChangePath_ = del;
}

void FSPicker::setOnSelectPath(OnSelectPathDelegate del)
{
	onSelectPath_ = del;
}

void FSPicker::onLeftNavBtn(const Input::Event &e)
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

void FSPicker::onRightNavBtn(const Input::Event &e)
{
	if(mode_ == Mode::DIR)
		onSelectPath_.callCopy(*this, root.path, appContext().fileUriDisplayName(root.path), e);
	else
		dismiss();
}

bool FSPicker::inputEvent(const Input::Event& e, ViewInputEventParams)
{
	if(e.keyEvent())
	{
		auto &keyEv = *e.keyEvent();
		if(keyEv.pushed(Input::DefaultKey::CANCEL))
		{
			if(fileUIStates.size())
				onLeftNavBtn(e);
			else
				dismiss();
			return true;
		}
		else if(controller.viewHasFocus() && keyEv.pushed(Input::DefaultKey::LEFT))
		{
			controller.moveFocusToNextView(e, CT2DO);
			controller.top().setFocus(false);
			return true;
		}
		else if(keyEv.pushed(Input::Keycode::GAME_B) || keyEv.pushed(Input::Keycode::F1))
		{
			pushFileLocationsView(e);
			return true;
		}
	}
	return controller.inputEvent(e);
}

void FSPicker::prepareDraw()
{
	controller.navView()->prepareDraw();
	controller.top().prepareDraw();
	if(dirListThread.isWorking())
		return;
	msgText.makeGlyphs();
}

void FSPicker::draw(Gfx::RendererCommands &__restrict__ cmds, ViewDrawParams) const
{
	if(!dirListThread.isWorking())
	{
		if(dir.size())
		{
			controller.top().draw(cmds);
		}
		else
		{
			using namespace IG::Gfx;
			cmds.basicEffect().enableAlphaTexture(cmds);
			msgText.draw(cmds, controller.top().viewRect().pos(C2DO), C2DO, ColorName::WHITE);
		}
	}
	controller.navView()->draw(cmds);
}

void FSPicker::onAddedToController(ViewController *, const Input::Event &e)
{
	controller.top().onAddedToController(&controller, e);
}

void FSPicker::setEmptyPath(std::string_view message)
{
	log.info("setting empty path");
	dirListThread.stop();
	dirListEvent.cancel();
	root = {};
	newFileUIState = {};
	fileUIStates.clear();
	dir.clear();
	msgText.resetString(message);
	if(mode_ == Mode::FILE_IN_DIR)
	{
		fileTableView().resetName();
	}
	else
	{
		fileTableView().resetName("Select File Location");
	}
	if(viewRect().x)
		place();
}

void FSPicker::setEmptyPath()
{
	setEmptyPath("No folder is set");
}

void FSPicker::setPath(CStringView path, FS::RootPathInfo rootInfo, const Input::Event &e)
{
	if(!strlen(path))
	{
		setEmptyPath();
		return;
	}
	if(e.keyEvent() && newFileUIState.highlightedCell == -1)
		newFileUIState.highlightedCell = 0;
	startDirectoryListThread(path);
	root.path = path;
	auto pathLen = path.size();
	// verify root info
	if(rootInfo.length && rootInfo.length > pathLen)
	{
		log.warn("invalid root length:{} with path length:{}", rootInfo.length, pathLen);
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
		log.info("root info:{}:{}", rootInfo.length, rootInfo.name);
		root.info = rootInfo;
		if(pathLen > rootInfo.length)
			rootedPath = format<FS::PathString>("{}{}", rootInfo.name, &path[rootInfo.length]);
		else
			rootedPath = rootInfo.name;
	}
	else
	{
		log.info("no root info");
		root.info = {};
		rootedPath = root.path;
	}
	if(isUri)
	{
		rootedPath = IG::decodeUri<FS::PathString>(rootedPath);
	}
	fileTableView().resetName(rootedPath);
	onChangePath_.callSafe(*this, e);
}

void FSPicker::setPath(CStringView path, FS::RootPathInfo rootInfo)
{
	return setPath(path, std::move(rootInfo), appContext().defaultInputEvent());
}

void FSPicker::setPath(CStringView path, const Input::Event &e)
{
	return setPath(path, appContext().rootPathInfo(path), e);
}

void FSPicker::setPath(CStringView path)
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

void FSPicker::goUpDirectory(const Input::Event &e)
{
	clearSelection();
	changeDirByInput(FS::dirnameUri(root.path), root.info, e, DepthMode::decrement);
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

void FSPicker::pushFileLocationsView(const Input::Event &e)
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
	static constexpr std::string_view failedSystemPickerMsg = "This device doesn't have a document browser, please select a media folder instead";
	if(appContext().hasSystemPathPicker())
	{
		view->appendItem("Browse For Folder",
			[this](View& view, const Input::Event&)
			{
				if(!appContext().showSystemPathPicker())
				{
					setEmptyPath(failedSystemPickerMsg);
					view.dismiss();
				}
			});
	}
	if(mode_ != Mode::DIR && appContext().hasSystemDocumentPicker())
	{
		view->appendItem("Browse For File",
			[this](View& view, const Input::Event&)
			{
				if(!appContext().showSystemDocumentPicker())
				{
					setEmptyPath(failedSystemPickerMsg);
					view.dismiss();
				}
			});
	}
	for(auto &loc : view->locations())
	{
		view->appendItem(loc.description,
			[this, &loc](View& view, const Input::Event& e)
			{
				auto ctx = appContext();
				if(ctx.usesPermission(Permission::WRITE_EXT_STORAGE))
				{
					if(!ctx.requestPermission(Permission::WRITE_EXT_STORAGE))
						return;
				}
				changeDirByInput(loc.root.path, loc.root.info, e, DepthMode::reset);
				view.dismiss();
			});
	}
	if(Config::envIsLinux)
	{
		view->appendItem("Root Filesystem",
			[this](View& view, const Input::Event& e)
			{
				changeDirByInput("/", {}, e, DepthMode::reset);
				view.dismiss();
			});
	}
	view->appendItem("Custom Path",
		[this](const Input::Event& e)
		{
			auto textInputView = makeView<CollectTextInputView>(
				"Input a directory path", root.path, Gfx::TextureSpan{},
				[this](CollectTextInputView &view, const char *str)
				{
					if(!str || !strlen(str))
					{
						view.dismiss();
						return false;
					}
					changeDirByInput(str, appContext().rootPathInfo(str), appContext().defaultInputEvent(), DepthMode::reset);
					dismissPrevious();
					view.dismiss();
					return false;
				});
			pushAndShow(std::move(textInputView), e);
		});
	pushAndShow(std::move(view), e);
}

bool FSPicker::onDocumentPicked(const DocumentPickerEvent& e)
{
	if(appContext().fileUriType(e.uri) == FS::file_type::directory)
	{
		if(mode_ == Mode::DIR)
		{
			onSelectPath_.callCopy(*this, e.uri, e.displayName, appContext().defaultInputEvent());
		}
		else
		{
			popTo(*this);
			changeDirByInput(e.uri, appContext().rootPathInfo(e.uri), appContext().defaultInputEvent(), DepthMode::reset);
		}
	}
	else
	{
		onSelectPath_.callCopy(*this, e.uri, e.displayName, appContext().defaultInputEvent());
	}
	return true;
}

Gfx::GlyphTextureSet &FSPicker::face()
{
	return *msgText.face();
}

TableView &FSPicker::fileTableView()
{
	return static_cast<TableView&>(controller.top());
}

void FSPicker::setShowHiddenFiles(bool on)
{
	showHiddenFiles_ = on;
}

void FSPicker::startDirectoryListThread(CStringView path)
{
	if(dirListThread.isWorking())
	{
		log.info("deferring listing directory until worker thread stops");
		dirListThread.requestStop();
		dirListEvent.setCallback([this]()
		{
			startDirectoryListThread(root.path);
		});
		return;
	}
	dir.clear();
	fileTableView().resetItemSource();
	dirListEvent.setCallback([this]()
	{
		fileTableView().resetItemSource(dir);
		place();
		fileTableView().restoreUIState(std::exchange(newFileUIState, {}));
		postDraw();
	});
	dirListEvent.cancel();
	dirListThread.reset([this](WorkThread::Context ctx, const std::string &path)
	{
		listDirectory(path, ctx.stop);
		if(ctx.stop.isQuitting()) [[unlikely]]
			return;
		ctx.finishedWork();
		dirListEvent.notify();
	}, std::string{path});
}

void FSPicker::listDirectory(CStringView path, ThreadStop &stop)
{
	try
	{
		appContext().forEachInDirectoryUri(path,
			[this, &stop](auto &entry)
			{
				//log.info("entry:{}", entry.path());
				if(stop) [[unlikely]]
				{
					log.info("interrupted listing directory");
					return false;
				}
				bool isDir = entry.type() == FS::file_type::directory;
				if(mode_ == Mode::FILE_IN_DIR) // filter directories
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
				auto &item = dir.emplace_back(attachParams(), std::string{entry.path()}, entry.name());
				if(isDir)
					item.text.flags.user |= FileEntry::isDirFlag;
				if(mode_ == Mode::DIR && !isDir)
					item.text.setActive(false);
				return true;
			});
		std::ranges::sort(dir,
			[](const FileEntry &e1, const FileEntry &e2)
			{
				if(e1.isDir() && !e2.isDir())
					return true;
				else if(!e1.isDir() && e2.isDir())
					return false;
				else
					return caselessLexCompare(e1.path, e2.path);
			});
		if(dir.size())
		{
			for(auto &d : dir)
			{
				if(!d.text.active())
					continue;
				if(d.isDir())
				{
					d.text.onSelect =
						[this, &dirPath = d.path](const Input::Event &e)
						{
							assert(!isSingleDirectoryMode());
							auto path = std::move(dirPath);
							log.info("entering dir:{}", path);
							changeDirByInput(path, root.info, e);
						};
				}
				else
				{
					d.text.onSelect =
						[this, &dirPath = d.path](const Input::Event &e)
						{
							onSelectPath_.callCopy(*this, dirPath, appContext().fileUriDisplayName(dirPath), e);
						};
				}
			}
			msgText.resetString();
		}
		else // no entries, show a message instead
		{
			msgText.resetString("Empty Directory");
			msgText.compile();
		}
	}
	catch(std::system_error &err)
	{
		log.error("can't open:{}", path);
		auto ec = err.code();
		std::string_view extraMsg = mode_ == Mode::FILE_IN_DIR ? "" : "\nPick a path from the top bar";
		msgText.resetString(std::format("Can't open directory:\n{}{}", ec.message(), extraMsg));
		msgText.compile();
	}
}

}
