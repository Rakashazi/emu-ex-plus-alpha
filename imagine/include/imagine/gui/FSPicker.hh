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
#include <imagine/gui/View.hh>
#include <imagine/gui/ViewStack.hh>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/string/CStringView.hh>
#include <vector>
#include <system_error>

namespace IG::Input
{
class Event;
}

namespace IG::FS
{
class directory_entry;
}

namespace IG
{

class TableView;

class FSPicker : public View
{
public:
	using FilterFunc = DelegateFunc<bool(const FS::directory_entry &entry)>;
	using OnChangePathDelegate = DelegateFunc<void (FSPicker &picker, FS::PathString prevPath, Input::Event e)>;
	using OnSelectFileDelegate = DelegateFunc<void (FSPicker &picker, IG::CStringView filePath, std::string_view displayName, Input::Event e)>;
	using OnCloseDelegate = DelegateFunc<void (FSPicker &picker, Input::Event e)>;
	enum class Mode : uint8_t { FILE, FILE_IN_DIR, DIR };

	FSPicker(ViewAttachParams attach, Gfx::TextureSpan backRes, Gfx::TextureSpan closeRes,
			FilterFunc filter = {}, Mode mode = Mode::FILE, Gfx::GlyphTextureSet *face = {});
	void place() override;
	bool inputEvent(Input::Event) override;
	void prepareDraw() override;
	void draw(Gfx::RendererCommands &) override;
	void onAddedToController(ViewController *, Input::Event) override;
	void setOnChangePath(OnChangePathDelegate);
	void setOnSelectFile(OnSelectFileDelegate);
	void setOnClose(OnCloseDelegate);
	void onLeftNavBtn(Input::Event);
	void onRightNavBtn(Input::Event);
	void setEmptyPath();
	std::error_code setPath(IG::CStringView path, FS::RootPathInfo rootInfo, Input::Event);
	std::error_code setPath(IG::CStringView path, FS::RootPathInfo rootInfo);
	std::error_code setPath(IG::CStringView path, Input::Event);
	std::error_code setPath(IG::CStringView path);
	FS::PathString path() const;
	FS::RootedPath rootedPath() const;
	void clearSelection() override;
	bool isSingleDirectoryMode() const;
	void goUpDirectory(Input::Event);
	void pushFileLocationsView(Input::Event);
	void setShowHiddenFiles(bool);

protected:
	struct FileEntry
	{
		std::string path{};
		bool isDir{};
		TextMenuItem text{};
	};

	FilterFunc filter{};
	ViewStack controller{};
	OnChangePathDelegate onChangePath_{};
	OnSelectFileDelegate onSelectFile_{};
	OnCloseDelegate onClose_;
	std::vector<FileEntry> dir{};
	FS::RootedPath root{};
	Gfx::Text msgText{};
	Mode mode_{};
	bool showHiddenFiles_{};

	std::error_code changeDirByInput(IG::CStringView path, FS::RootPathInfo rootInfo, Input::Event e);
	bool isAtRoot() const;
	Gfx::GlyphTextureSet &face();
	TableView &fileTableView();
};

}
