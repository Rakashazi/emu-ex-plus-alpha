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
#include <imagine/base/CustomEvent.hh>
#include <imagine/thread/WorkThread.hh>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/string/CStringView.hh>
#include <vector>
#include <string>
#include <string_view>

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
	using FilterFunc = DelegateFunc<bool(const FS::directory_entry &)>;
	using OnChangePathDelegate = DelegateFunc<void (FSPicker &, const Input::Event &)>;
	using OnSelectPathDelegate = DelegateFunc<void (FSPicker &, CStringView filePath, std::string_view displayName, const Input::Event &)>;
	enum class Mode : uint8_t { FILE, FILE_IN_DIR, DIR };

	struct FileEntry
	{
		static constexpr auto isDirFlag = bit(0);

		std::string path;
		TextMenuItem text;

		FileEntry(ViewAttachParams attach, auto &&path, UTF16Convertible auto &&name):
			path{IG_forward(path)}, text{IG_forward(name), attach} {}
		bool isDir() const { return text.flags.user & isDirFlag; }
		TextMenuItem &menuItem() { return text; }
	};

	enum class DepthMode { increment, decrement, reset };

	FSPicker(ViewAttachParams attach, Gfx::TextureSpan backRes, Gfx::TextureSpan closeRes,
			FilterFunc filter = {}, Mode mode = Mode::FILE, Gfx::GlyphTextureSet *face = {});
	void place() override;
	bool inputEvent(const Input::Event&, ViewInputEventParams p = {}) override;
	void prepareDraw() override;
	void draw(Gfx::RendererCommands &__restrict__, ViewDrawParams p = {}) const override;
	void onAddedToController(ViewController *, const Input::Event &) override;
	void setOnChangePath(OnChangePathDelegate);
	void setOnSelectPath(OnSelectPathDelegate);
	void onLeftNavBtn(const Input::Event &);
	void onRightNavBtn(const Input::Event &);
	void setEmptyPath();
	void setPath(CStringView path, FS::RootPathInfo, const Input::Event &);
	void setPath(CStringView path, FS::RootPathInfo);
	void setPath(CStringView path, const Input::Event &);
	void setPath(CStringView path);
	FS::PathString path() const;
	FS::RootedPath rootedPath() const;
	void clearSelection() override;
	bool isSingleDirectoryMode() const;
	void goUpDirectory(const Input::Event &);
	void pushFileLocationsView(const Input::Event &);
	void setShowHiddenFiles(bool);
	bool onDocumentPicked(const DocumentPickerEvent&) override;

protected:
	FilterFunc filter{};
	ViewStack controller;
	OnChangePathDelegate onChangePath_;
	OnSelectPathDelegate onSelectPath_;
	std::vector<FileEntry> dir;
	std::vector<TableUIState> fileUIStates;
	FS::RootedPath root;
	Gfx::Text msgText;
	CustomEvent dirListEvent;
	TableUIState newFileUIState{};
	Mode mode_{};
	bool showHiddenFiles_{};
	WorkThread dirListThread{};

	void changeDirByInput(CStringView path, FS::RootPathInfo, const Input::Event &,
		DepthMode depthMode = DepthMode::increment);
	bool isAtRoot() const;
	Gfx::GlyphTextureSet &face();
	TableView &fileTableView();
	void startDirectoryListThread(CStringView path);
	void listDirectory(CStringView path, ThreadStop &stop);
	void setEmptyPath(std::string_view message);
};

}
