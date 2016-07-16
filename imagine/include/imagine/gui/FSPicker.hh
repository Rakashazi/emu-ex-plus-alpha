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

#include <vector>
#include <system_error>
#include <imagine/config/defs.hh>
#include <imagine/gfx/GfxText.hh>
#include <imagine/gfx/GeomRect.hh>
#include <imagine/gfx/GfxLGradient.hh>
#include <imagine/gfx/Texture.hh>
#include <imagine/input/Input.hh>
#include <imagine/fs/FS.hh>
#include <imagine/resource/face/ResourceFace.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/gui/View.hh>
#include <imagine/gui/NavView.hh>

class FSPicker : public View
{
public:
	using FilterFunc = DelegateFunc<bool(FS::directory_entry &entry)>;
	using OnChangePathDelegate = DelegateFunc<void (FSPicker &picker, FS::PathString prevPath, Input::Event e)>;
	using OnSelectFileDelegate = DelegateFunc<void (FSPicker &picker, const char *name, Input::Event e)>;
	using OnCloseDelegate = DelegateFunc<void (FSPicker &picker, Input::Event e)>;
	using OnPathReadError = DelegateFunc<void (FSPicker &picker, std::error_code ec)>;
	static constexpr bool needsUpDirControl = !Config::envIsPS3;

	FSPicker(Base::Window &win, Gfx::PixmapTexture *backRes, Gfx::PixmapTexture *closeRes,
			FilterFunc filter = {}, bool singleDir = false, ResourceFace *face = View::defaultFace);
	void place() override;
	void inputEvent(Input::Event e) override;
	void draw() override;
	void onAddedToController(Input::Event e) override;
	void setOnChangePath(OnChangePathDelegate del);
	void setOnSelectFile(OnSelectFileDelegate del);
	void setOnClose(OnCloseDelegate del);
	void setOnPathReadError(OnPathReadError del);
	void onLeftNavBtn(Input::Event e);
	void onRightNavBtn(Input::Event e);
	std::error_code setPath(const char *path, bool forcePathChange, Input::Event e);
	std::error_code setPath(const char *path, bool forcePathChange);
	std::error_code setPath(FS::PathString path, bool forcePathChange, Input::Event e)
	{
		return setPath(path.data(), forcePathChange, e);
	}
	std::error_code setPath(FS::PathString path, bool forcePathChange)
	{
		return setPath(path.data(), forcePathChange);
	}
	FS::PathString path() const;
	IG::WindowRect &viewRect() override { return viewFrame; }
	void clearSelection() override
	{
		tbl.clearSelection();
	}
	FS::PathString makePathString(const char *base) const;

protected:
	FilterFunc filter{};
	TableView tbl;
	OnChangePathDelegate onChangePath_{};
	OnSelectFileDelegate onSelectFile_{};
	OnCloseDelegate onClose_
	{
		[](FSPicker &picker, Input::Event e)
		{
			picker.dismiss();
		}
	};
	OnPathReadError onPathReadError_{};
	std::vector<TextMenuItem> text{};
	std::vector<FS::FileString> dir{};
	FS::PathString currPath{};
	IG::WindowRect viewFrame{};
	ResourceFace *faceRes{};
	BasicNavView navV;
	std::array<char, 48> msgStr{};
	Gfx::Text msgText{};
	bool singleDir = false;

	void changeDirByInput(const char *path, bool forcePathChange, Input::Event e);
};
