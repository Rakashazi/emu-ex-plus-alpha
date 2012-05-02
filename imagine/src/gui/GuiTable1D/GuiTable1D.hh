#pragma once
#include <util/rectangle2.h>
#include <input/interface.h>
#include <gfx/Gfx.hh>
#include <config/env.hh>

class GuiTable1D;

class GuiTableSource
{
public:
	constexpr GuiTableSource() { }
	virtual void drawElement(const GuiTable1D *table, uint element, GC xPos, GC yPos, GC xSize, GC ySize, _2DOrigin align) const = 0;
	virtual void onSelectElement(const GuiTable1D *table, const InputEvent &event, uint element) = 0;
};

class GuiTable1D
{
public:
	constexpr GuiTable1D() : yCellSize(0), cells(0), selected(-1), selectedIsActivated(0),
		src(0) { }
	int yCellSize;
	int cells, selected, selectedIsActivated;
	GuiTableSource *src;
	Rect2<int> viewRect;
	_2DOrigin align;

	void init(GuiTableSource *src, int cells, _2DOrigin align = LC2DO);
	void setXCellSize(int s);
	void setYCellSize(int s);
	int inputEvent(const InputEvent &event);
	void draw();
	Rect2<int> focusRect();
	static GC globalXIndent;

	static void setDefaultXIndent()
	{
		GuiTable1D::globalXIndent =
			(Config::envIsAndroid || Config::envIsIOS || Config::envIsWebOS) ? /*floor*/(Gfx::xMMSize(1)) :
			(Config::envIsPS3) ? /*floor*/(Gfx::xMMSize(16)) :
			/*floor*/(Gfx::xMMSize(2));
	}

	void clearSelection()
	{
		selected = -1;
	}

private:
	int visibleCells() const;
	int offscreenCells() const;
};

#include <gui/ScrollView1D/ScrollView1D.hh>

class ScrollableGuiTable1D : public GuiTable1D, public ScrollView1D
{
public:
	constexpr ScrollableGuiTable1D() : onlyScrollIfNeeded(0) { }
	uint onlyScrollIfNeeded;

	void init(GuiTableSource *src, int cells, _2DOrigin align = LC2DO)
	{
		onlyScrollIfNeeded = 0;
		GuiTable1D::init(src, cells, align);
		ScrollView1D::init(&viewRect);
	}

	void deinit()
	{

	}

	void draw()
	{
		using namespace Gfx;
		ScrollView1D::updateGfx();
		setClipRectBounds(ScrollView1D::viewFrame);
		setClipRect(1);
		ScrollView1D::draw();
		GuiTable1D::draw();
		setClipRect(0);
	}

	void place(Rect2<int> *frame)
	{
		assert(frame);
		setXCellSize(frame->xSize());
		ScrollView1D::place(frame);
		updateView();
	}

	void setScrollableIfNeeded(bool yes)
	{
		onlyScrollIfNeeded = 1;
	}

	void scrollToFocusRect()
	{
		Rect2<int> focus = focusRect();
		//logMsg("focus box %d,%d %d,%d, scroll %d", focus.x, focus.y, focus.x2, focus.y2, gfx_toIYSize(scroll.offset));
		if(focus.ySize() > 1 && !viewFrame.contains(focus))
		{
			int diff;
			if(focus.y < viewFrame.y)
				diff = focus.y - viewFrame.y;
			else
				diff = focus.y2 - viewFrame.y2;
			diff--;
			logMsg("focus not in view by %d", diff);

			scroll.setOffset(scroll.offset + /*gfx_iYSize*/(diff));
			ScrollView1D::updateView();
		}
	}

	void updateView() // move content frame in position along view frame
	{
		ScrollView1D::updateView();
		scrollToFocusRect();
	}

	void inputEvent(const InputEvent &e)
	{
		bool handleScroll = !onlyScrollIfNeeded || contentIsBiggerThanView;
		if(handleScroll && ScrollView1D::inputEvent(e))
		{
			selected = -1;
			return;
		}
		if(e.isPointer() && !ScrollView1D::viewFrame.overlaps(e.x, e.y))
			return;
		GuiTable1D::inputEvent(e);
		if(handleScroll && !e.isPointer())
		{
			scrollToFocusRect();
		}
	}
};
