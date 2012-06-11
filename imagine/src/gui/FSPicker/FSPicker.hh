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


// TODO: move *NavView into separate file,
class NavView
{
public:
	typedef Delegate<void (const InputEvent &e)> OnInputDelegate;

	constexpr NavView() { }
	constexpr NavView(OnInputDelegate onLeftNavBtn, OnInputDelegate onRightNavBtn):
			onLeftNavBtn(onLeftNavBtn), onRightNavBtn(onRightNavBtn) { }

	OnInputDelegate onLeftNavBtn, onRightNavBtn;
	OnInputDelegate &leftNavBtnDelegate() { return onLeftNavBtn; }
	OnInputDelegate &rightNavBtnDelegate() { return onRightNavBtn; }

	Rect2<int> leftBtn, rightBtn, textRect;
	GfxText text;
	Rect2<int> viewRect;
	bool hasBackBtn = 0, leftBtnActive = 0, hasCloseBtn = 0, rightBtnActive = 0;

	void setLeftBtnActive(bool on) { leftBtnActive = on; }
	void setRightBtnActive(bool on) { rightBtnActive = on; }
	void setTitle(const char *title) { text.setString(title); }

	void init(ResourceFace *face);
	virtual void deinit() = 0;
	void deinitText();
	virtual void place();
	void inputEvent(const InputEvent &e);
	virtual void draw() = 0;
};

class BasicNavView : public NavView
{
public:
	constexpr BasicNavView() { }
	constexpr BasicNavView(OnInputDelegate left, OnInputDelegate right): NavView(left, right) { }
	GfxSprite leftSpr, rightSpr;
	GfxLGradient bg;
	void init(ResourceFace *face, ResourceImage *leftRes, ResourceImage *rightRes,
			const GfxLGradientStopDesc *gradStop, uint gradStops);
	void setBackImage(ResourceImage *img);
	void draw();
	void place();
	void deinit();
};


class FSNavView : public BasicNavView
{
public:
	constexpr FSNavView() { }
	constexpr FSNavView(OnInputDelegate left, OnInputDelegate right): BasicNavView(left, right) { }
	void init(ResourceFace *face, ResourceImage *backRes, ResourceImage *closeRes, bool singleDir);
	void draw();
	void place();
};

class FSPicker : public View, public GuiTableSource
{
public:
	constexpr FSPicker(): navV(NavView::OnInputDelegate::create<FSPicker, &FSPicker::onLeftNavBtn>(this),
			NavView::OnInputDelegate::create<FSPicker, &FSPicker::onRightNavBtn>(this)) { }
	FsDirFilterFunc filter = nullptr;

	static const bool needsUpDirControl = !Config::envIsPS3;

	void init(const char *path, ResourceImage *backRes, ResourceImage *closeRes,
			FsDirFilterFunc filter = 0, bool singleDir = 0, ResourceFace *face = View::defaultFace);
	void deinit();
	void place();
	void place(Rect2<int> rect)
	{
		View::place(rect);
	}
	void inputEvent(const InputEvent &e);
	void draw();
	void drawElement(const GuiTable1D *table, uint element, Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const;
	void onSelectElement(const GuiTable1D *table, const InputEvent &e, uint i);

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
	FSNavView navV;
	bool singleDir = 0;

	void loadDir(const char *path);
	void changeDirByInput(const char *path, const InputEvent &e);
};
