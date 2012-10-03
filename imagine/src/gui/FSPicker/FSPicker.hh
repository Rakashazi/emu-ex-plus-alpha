#pragma once

#include <gfx/GfxText.hh>
#include <gfx/GeomRect.hh>
#include <gfx/GfxLGradient.hh>
#include <util/rectangle2.h>
#include <input/interface.h>
#include <fs/sys.hh>
#include <resource2/font/freetype/ResourceFontFreetype.h>
#include <gui/GuiTable1D/GuiTable1D.hh>
#include <gui/MenuItem/MenuItem.hh>
#include <util/Delegate.hh>
#include <gui/View.hh>
#include <gui/NavView.hh>

class FSNavView : public BasicNavView
{
public:
	constexpr FSNavView() { }
	constexpr FSNavView(OnInputDelegate left, OnInputDelegate right): BasicNavView(left, right) { }
	void init(ResourceFace *face, ResourceImage *backRes, ResourceImage *closeRes, bool singleDir);
	void draw() override;
	void place() override;
};

class FSPicker : public View, public GuiTableSource
{
public:
	constexpr FSPicker() { }
	FsDirFilterFunc filter = nullptr;

	static const bool needsUpDirControl = !Config::envIsPS3;

	void init(const char *path, ResourceImage *backRes, ResourceImage *closeRes,
			FsDirFilterFunc filter = 0, bool singleDir = 0, ResourceFace *face = View::defaultFace);
	void deinit() override;
	void place() override;
	void inputEvent(const InputEvent &e) override;
	void draw() override;
	void drawElement(const GuiTable1D *table, uint element, Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const override;
	void onSelectElement(const GuiTable1D *table, const InputEvent &e, uint i) override;

	typedef Delegate<void (const char* name, const InputEvent &e)> OnSelectFileDelegate;
	OnSelectFileDelegate onSelectFile;
	typedef Delegate<void (const InputEvent &e)> OnCloseDelegate;
	OnCloseDelegate onClose;
	OnSelectFileDelegate &onSelectFileDelegate() { return onSelectFile; }
	OnCloseDelegate &onCloseDelegate() { return onClose; }

	void onLeftNavBtn(const InputEvent &e);
	void onRightNavBtn(const InputEvent &e);
	Rect2<int> &viewRect() { return viewFrame; }
	void clearSelection()
	{
		tbl.clearSelection();
	}

	ScrollableGuiTable1D tbl;
private:
	TextMenuItem *text = nullptr;
	FsSys dir;
	Rect2<int> viewFrame;
	ResourceFace *faceRes = nullptr;
	FSNavView navV { NavView::OnInputDelegate::create<FSPicker, &FSPicker::onLeftNavBtn>(this),
		NavView::OnInputDelegate::create<FSPicker, &FSPicker::onRightNavBtn>(this) };
	bool singleDir = 0;

	void loadDir(const char *path);
	void changeDirByInput(const char *path, const InputEvent &e);
};
