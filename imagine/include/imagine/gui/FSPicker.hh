#pragma once

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

#include <imagine/config/defs.hh>
#include <imagine/gfx/GfxText.hh>
#include <imagine/fs/FSDefs.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/gui/View.hh>
#include <imagine/gui/ViewStack.hh>
#include <vector>
#include <system_error>

namespace Input
{
class Event;
}

class FSPicker : public View
{
public:
	using FilterFunc = DelegateFunc<bool(FS::directory_entry &entry)>;
	using OnChangePathDelegate = DelegateFunc<void (FSPicker &picker, FS::PathString prevPath, Input::Event e)>;
	using OnSelectFileDelegate = DelegateFunc<void (FSPicker &picker, const char *name, Input::Event e)>;
	using OnCloseDelegate = DelegateFunc<void (FSPicker &picker, Input::Event e)>;
	using OnPathReadError = DelegateFunc<void (FSPicker &picker, std::error_code ec)>;
	static constexpr bool needsUpDirControl = true;

	FSPicker(ViewAttachParams attach, Gfx::TextureSpan backRes, Gfx::TextureSpan closeRes,
			FilterFunc filter = {}, bool singleDir = false, Gfx::GlyphTextureSet *face = {});
	void place() override;
	bool inputEvent(Input::Event e) override;
	void prepareDraw() override;
	void draw(Gfx::RendererCommands &cmds) override;
	void onAddedToController(ViewController *c, Input::Event e) override;
	void setOnChangePath(OnChangePathDelegate del);
	void setOnSelectFile(OnSelectFileDelegate del);
	void setOnClose(OnCloseDelegate del);
	void setOnPathReadError(OnPathReadError del);
	void onLeftNavBtn(Input::Event e);
	void onRightNavBtn(Input::Event e);
	std::error_code setPath(const char *path, bool forcePathChange, FS::RootPathInfo rootInfo, Input::Event e);
	std::error_code setPath(const char *path, bool forcePathChange, FS::RootPathInfo rootInfo);
	std::error_code setPath(FS::PathString path, bool forcePathChange, FS::RootPathInfo rootInfo, Input::Event e);
	std::error_code setPath(FS::PathString path, bool forcePathChange, FS::RootPathInfo rootInfo);
	std::error_code setPath(FS::PathLocation location, bool forcePathChange);
	std::error_code setPath(FS::PathLocation location, bool forcePathChange, Input::Event e);
	FS::PathString path() const;
	void clearSelection() override;
	FS::PathString makePathString(const char *base) const;
	bool isSingleDirectoryMode() const;
	void goUpDirectory(Input::Event e);

protected:
	struct FileEntry
	{
		FS::FileString name{};
		bool isDir{};

		constexpr FileEntry() {}
		constexpr FileEntry(FS::FileString name, bool isDir):
			name{name}, isDir{isDir}
		{}
	};

	FilterFunc filter{};
	ViewStack controller{};
	OnChangePathDelegate onChangePath_{};
	OnSelectFileDelegate onSelectFile_{};
	OnCloseDelegate onClose_;
	OnPathReadError onPathReadError_{};
	std::vector<TextMenuItem> text{};
	std::vector<FileEntry> dir{};
	std::vector<FS::PathLocation> rootLocation{};
	FS::RootPathInfo root{};
	FS::PathString currPath{};
	FS::PathString rootedPath{};
	Gfx::Text msgText{};
	bool singleDir = false;

	void changeDirByInput(const char *path, FS::RootPathInfo rootInfo, bool forcePathChange, Input::Event e);
	bool isAtRoot() const;
	void pushFileLocationsView(Input::Event e);
	Gfx::GlyphTextureSet &face();
};
