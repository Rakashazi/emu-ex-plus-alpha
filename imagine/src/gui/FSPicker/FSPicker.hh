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

#include <gfx/GfxText.hh>
#include <gfx/GeomRect.hh>
#include <gfx/GfxLGradient.hh>
#include <gfx/GfxBufferImage.hh>
#include <util/rectangle2.h>
#include <input/Input.hh>
#include <fs/sys.hh>
#include <resource2/face/ResourceFace.hh>
#include <gui/GuiTable1D/GuiTable1D.hh>
#include <gui/MenuItem/MenuItem.hh>
#include <util/DelegateFunc.hh>
#include <gui/View.hh>
#include <gui/NavView.hh>

class FSPicker : public View, public GuiTableSource
{
public:
	constexpr FSPicker() { }
	FsDirFilterFunc filter = nullptr;

	static const bool needsUpDirControl = !Config::envIsPS3;

	void init(const char *path, Gfx::BufferImage *backRes, Gfx::BufferImage *closeRes,
			FsDirFilterFunc filter = 0, bool singleDir = 0, ResourceFace *face = View::defaultFace);
	void deinit() override;
	void place() override;
	void inputEvent(const Input::Event &e) override;
	void draw(Gfx::FrameTimeBase frameTime) override;
	void drawElement(const GuiTable1D *table, uint element, Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const override;
	void onSelectElement(const GuiTable1D *table, const Input::Event &e, uint i) override;

	typedef DelegateFunc<void (const char* name, const Input::Event &e)> OnSelectFileDelegate;
	OnSelectFileDelegate onSelectFileD;
	typedef DelegateFunc<void (const Input::Event &e)> OnCloseDelegate;
	OnCloseDelegate onCloseD;
	OnSelectFileDelegate &onSelectFile() { return onSelectFileD; }
	OnCloseDelegate &onClose() { return onCloseD; }

	// convenience onClose handler
	static OnCloseDelegate onCloseModalDefault()
	{
		return [](const Input::Event &e)
			{
				View::removeModalView();
			};
	}

	void onLeftNavBtn(const Input::Event &e);
	void onRightNavBtn(const Input::Event &e);
	Rect2<int> &viewRect() { return viewFrame; }
	void clearSelection()
	{
		tbl.clearSelection();
	}

	ScrollableGuiTable1D tbl;
private:
	class FSNavView : public BasicNavView
	{
	public:
		constexpr FSNavView(FSPicker &inst): inst(inst) { }
		FSPicker &inst;
		void onLeftNavBtn(const Input::Event &e) override
		{
			inst.onLeftNavBtn(e);
		};
		void onRightNavBtn(const Input::Event &e) override
		{
			inst.onRightNavBtn(e);
		};
		void init(ResourceFace *face, Gfx::BufferImage *backRes, Gfx::BufferImage *closeRes, bool singleDir);
		void draw() override;
		void place() override;
	};

	TextMenuItem *text = nullptr;
	FsSys dir;
	Rect2<int> viewFrame;
	ResourceFace *faceRes = nullptr;
	FSNavView navV {*this};
	bool singleDir = 0;

	void loadDir(const char *path);
	void changeDirByInput(const char *path, const Input::Event &e);
};
