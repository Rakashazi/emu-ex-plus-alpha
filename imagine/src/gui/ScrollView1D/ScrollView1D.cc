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

#define thisModuleName "ScrollView1D"

#include "ScrollView1D.hh"
#include <input/DragPointer.hh>
#include <gfx/GeomRect.hh>
#include <base/Base.hh>
#include <algorithm>
#include <cmath>

void ContentDrag::init(uint axis)
{
	dragStartX = dragStartY = 0;
	active = pushed = 0;
	this->axis = axis;
}

ContentDrag::State ContentDrag::inputEvent(const Rect2<int> &bt, const Input::Event &e)
{
	#ifdef INPUT_SUPPORTS_POINTER
	if(pushed && e.devId != devId)
		return NO_CHANGE;

	auto dragState = Input::dragState(e.devId);
	if(e.pushed(Input::Pointer::LBUTTON) && bt.overlaps(e.x, e.y))
	{
		pushed = 1;
		devId = e.devId;
		//logMsg("pushed");
		return PUSHED;
	}
	else if(pushed && !active && dragState->draggedFromRect(bt) &&
			((testingXAxis() && std::abs(dragState->pushX - e.x) > dragStartX) ||
			(testingYAxis() && std::abs(dragState->pushY - e.y) > dragStartY)))
	{
		//logMsg("in scroll");
		active = 1;
		return ENTERED_ACTIVE;
	}
	else if(active && (e.state == Input::RELEASED || e.state == Input::EXIT_VIEW))
	{
		active = 0;
		pushed = 0;
		return LEFT_ACTIVE;
	}
	else if(active && dragState->draggedFromRect(bt))
	{
		return ACTIVE;
	}
	else if(e.state == Input::RELEASED)
	{
		pushed = 0;
		return RELEASED;
	}
	else
	{
		return INACTIVE;
	}
	#else
	return NO_CHANGE;
	#endif
}

void KScroll::init(const Rect2<int> *viewFrame, const Rect2<int> *contentFrame)
{
	ContentDrag::init();
	offset = start = 0;
	prevOffset = 0;
	vel = 0;
	assert(contentFrame);
	assert(viewFrame);
	this->viewFrame = viewFrame;
	this->contentFrame = contentFrame;
	scrollWholeArea = allowScrollWholeArea = 0;
}

void KScroll::place()
{
	assert(contentFrame);
	assert(viewFrame);
	dragStartY = std::max(1, Config::envIsAndroid ? Gfx::ySMMSizeToPixel(1.5) : Gfx::ySMMSizeToPixel(1.));
	maxClip = contentFrame->ySize() - viewFrame->ySize();
	if(viewFrame->ySize() > 0)
		allowScrollWholeArea = contentFrame->ySize() / viewFrame->ySize() > 3;
	else
		allowScrollWholeArea = 0;
}

bool KScroll::clipOverEdge(int minC, int maxC)
{
	using namespace IG;
	if(!active)
	{
		if(maxC < minC)
			maxC = minC;
		int clip = offset < minC ? minC : maxC;
		int sign = offset < minC ? 1 : -1;
		if(offset < minC || offset > maxC)
		{
			//logMsg("clip over edge");
			offset += sign * std::max(1, (int)std::abs((clip - offset) * GC(.2)));
			if((sign == 1 && offset > minC)
				|| (sign == -1 && offset < maxC))
			{
				//logMsg("reached clip");
				offset = clip;
			}
			vel = 0;
			Base::displayNeedsUpdate();
			return 1;
		}
	}
	return 0;
}

void KScroll::clipDragOverEdge(int minC, int maxC)
{
	if(active)
	{
		if(maxC < minC)
			maxC = minC;
		//int clip = offset < minC ? minC : maxC;
		//int sign = offset < minC ? 1 : -1;
		if(offset < minC || offset > maxC)
		{
			int clip = offset < minC ? minC : maxC;
			//int sign = offset < minC ? 1 : -1;

			offset += (clip - offset)/2;
		}
	}
}

void KScroll::decel2()
{
	GC stoppingVel = 1;
	if(!active && vel != (GC)0)
	{
		vel *= 0.9f;
		offset += vel;
		if(std::abs(vel) <= stoppingVel)
			vel = 0;
		Base::displayNeedsUpdate();
		//logMsg("did decel scroll");
	}
}

bool KScroll::inputEvent(const Input::Event &e)
{
	#ifdef INPUT_SUPPORTS_POINTER
	auto dragState = Input::dragState(e.devId);
	switch(ContentDrag::inputEvent(*viewFrame, e))
	{
		case ContentDrag::PUSHED:
		{
			start = offset;
		}
		return 0;

		case ContentDrag::ENTERED_ACTIVE:
		{
			//logMsg("in scroll");
			if(allowScrollWholeArea && (e.x > viewFrame->xSize() - Gfx::xSMMSizeToPixel(7.5)))
			{
				logMsg("scrolling all content");
				scrollWholeArea = 1;
			}
			else
			{
				scrollWholeArea = 0;
			}
		}
		return 1;

		case ContentDrag::LEFT_ACTIVE:
		{
			//logMsg("out of scroll, with yVel %f", (double)vel);
			//if(vel != (GC)0) // TODO: situations where a redraw is needed even with vel == 0
				Base::displayNeedsUpdate();
		}
		return 1;

		case ContentDrag::ACTIVE:
		{
			if(scrollWholeArea)
			{
				//logMsg("%d from %d-%d to %d-%d", e.y, 0, gfx_viewPixelHeight(), 0, maxClip);
				offset = IG::scalePointRange((GC)e.y, (GC)viewFrame->y, (GC)viewFrame->y + (GC)viewFrame->ySize(), (GC)0, (GC)maxClip);
				//logMsg("offset %d", offset);
				offset = IG::clipToBounds(offset, 0, maxClip);
			}
			else
			{
				offset = start - (e.y - dragState->pushY);
				vel = (offset - prevOffset);
				//logMsg("dragging with vel %f", vel);
			}
			Base::displayNeedsUpdate();
		}
		return 1;

		default: return 0;
	}
	#else
	return 0;
	#endif
}

bool KScroll::inputEvent(int minClip, int maxClip, const Input::Event &e)
{
	prevOffset = offset;
	bool ret = 0;
	#ifdef INPUT_SUPPORTS_MOUSE
	if(e.isPointer() && (e.button == Input::Pointer::WHEEL_UP || e.button == Input::Pointer::WHEEL_DOWN))
	{
		if(e.pushed() && offset >= minClip && offset <= maxClip)
		{
			bool clip = 1; // snap to edges
			if(offset == minClip || offset == maxClip)
			{
				clip = 0; // if exactly at edge don't clip for snap-back
			}
			auto vel = Gfx::ySMMSizeToPixel(10.0);
			offset += e.button == Input::Pointer::WHEEL_UP ? -vel : vel;
			if(clip)
			{
				if(offset < minClip)
					offset = minClip;
				else if(offset > maxClip)
					offset = maxClip;
			}
			Base::displayNeedsUpdate();
		}
		ret = 1;
	}
	else
	#endif
	{
		ret = e.isPointer() ? inputEvent(e) : 0;
	}
	clipDragOverEdge(minClip, maxClip);
	if(maxClip < minClip)
		maxClip = minClip;
	if(offset < minClip || offset > maxClip)
	{
		//logMsg("offset needs to be clipped");
		Base::displayNeedsUpdate();
	}
	return ret;
}

void KScroll::setOffset(int o)
{
	prevOffset = offset = o;
	vel = 0;
	active = 0;
	pushed = 0;
}

void KScroll::animate(int minClip, int maxClip)
{
	if(!clipOverEdge(minClip, maxClip))
		decel2();
}

void ScrollView1D::init(Rect2<int> *contentFrame)
{
	assert(contentFrame);
	this->contentFrame = contentFrame;
	scroll.init(&viewFrame, contentFrame);
}

void ScrollView1D::updateView() // move content frame in position along view frame
{
	contentFrame->setPos({viewFrame.xPos(LT2DO), viewFrame.yPos(LT2DO) - scroll.offset}, LT2DO);
}

void ScrollView1D::place(Rect2<int> *frame)
{
	assert(frame);
	assert(contentFrame);
	viewFrame.setPosRel(frame->pos(LT2DO), frame->size(), LT2DO);
	scroll.place();
	contentIsBiggerThanView = contentFrame->ySize() > viewFrame.ySize();
	scrollBarRect.x = viewFrame.x2 - 5;
	scrollBarRect.x2 = scrollBarRect.x + 3;
	scrollBarRect.y = 0;
	/*if(contentFrame->ySize() == 0)
		scrollBarRect.y2 = 0;*/
	scrollBarRect.y2 = viewFrame.ySize() * (viewFrame.ySize() / (GC)contentFrame->ySize());
	if(scrollBarRect.y2 < 10)
		scrollBarRect.y2 = 10;
}

void ScrollView1D::updateGfx()
{
	scroll.animate(0, contentFrame->ySize() - viewFrame.ySize());
	updateView();
}

void ScrollView1D::draw()
{
	using namespace Gfx;
	if(contentIsBiggerThanView && (scroll.allowScrollWholeArea || scroll.active))
	{
		setImgMode(IMG_MODE_MODULATE);
		setBlendMode(0);
		if(scroll.scrollWholeArea)
		{
			if(scroll.active)
				setColor(.8, .8, .8);
			else
				setColor(.5, .5, .5);
		}
		else
			setColor(.5, .5, .5);
		resetTransforms();
		scrollBarRect.setYPos(IG::scalePointRange((GC)scroll.offset, (GC)0, GC(scroll.maxClip), (GC)viewFrame.y, GC(viewFrame.y2 - scrollBarRect.ySize())));
		GeomRect::draw(scrollBarRect);
		setColor(COLOR_WHITE);
	}
}

int ScrollView1D::inputEvent(const Input::Event &e)
{
	int scrollHasControl = 0;
	auto oldOffset = scroll.offset;
	if(scroll.inputEvent(0, contentFrame->ySize() - viewFrame.ySize(), e))
	{
		scrollHasControl = 1;
	}
	if(oldOffset != scroll.offset)
	{
		updateView();
	}
	return scrollHasControl;
}
