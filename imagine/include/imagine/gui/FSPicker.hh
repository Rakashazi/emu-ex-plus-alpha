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
#include <imagine/engine-globals.h>
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
	using OnSelectFileDelegate = DelegateFunc<void (FSPicker &picker, const char* name, Input::Event e)>;
	using OnCloseDelegate = DelegateFunc<void (FSPicker &picker, Input::Event e)>;
	using OnPathReadError = DelegateFunc<void (FSPicker &picker, CallResult res)>;
	static constexpr bool needsUpDirControl = !Config::envIsPS3;

	FSPicker(Base::Window &win): View{win}, tbl{win} {}
	void init(Gfx::PixmapTexture *backRes, Gfx::PixmapTexture *closeRes,
			FilterFunc filter = {}, bool singleDir = false, ResourceFace *face = View::defaultFace);
	void deinit() override;
	void place() override;
	void inputEvent(Input::Event e) override;
	void draw() override;
	void onAddedToController(Input::Event e) override;
	void setOnSelectFile(OnSelectFileDelegate del);
	void setOnClose(OnCloseDelegate del);
	void onLeftNavBtn(Input::Event e);
	void onRightNavBtn(Input::Event e);
	void setOnPathReadError(OnPathReadError del);
	CallResult setPath(const char *path, Input::Event e);
	CallResult setPath(const char *path);
	IG::WindowRect &viewRect() override { return viewFrame; }
	void clearSelection() override
	{
		tbl.clearSelection();
	}

protected:
	class FSNavView : public BasicNavView
	{
	public:
		FSPicker &inst;
		FS::PathString titleStr{};

		FSNavView(FSPicker &inst): inst(inst) {}
		void onLeftNavBtn(Input::Event e) override
		{
			inst.onLeftNavBtn(e);
		};
		void onRightNavBtn(Input::Event e) override
		{
			inst.onRightNavBtn(e);
		};
		void init(ResourceFace *face, Gfx::PixmapTexture *backRes, Gfx::PixmapTexture *closeRes, bool singleDir);
		void draw(const Base::Window &win, const Gfx::ProjectionPlane &projP) override;
		void setTitle(const char *str);
	};

	FilterFunc filter{};
	TableView tbl;
	OnSelectFileDelegate onSelectFileD{};
	OnCloseDelegate onCloseD
	{
		[](FSPicker &picker, Input::Event e)
		{
			picker.dismiss();
		}
	};
	OnPathReadError onPathReadError{};
	MenuItem **textPtr{};
	TextMenuItem *text{};
	std::vector<FS::FileString> dir{};
	IG::WindowRect viewFrame{};
	ResourceFace *faceRes{};
	FSNavView navV{*this};
	bool singleDir = false;

	void changeDirByInput(const char *path, Input::Event e);
};
