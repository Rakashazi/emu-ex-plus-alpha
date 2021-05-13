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

#define LOGTAG "ScrollView"

#include <imagine/gui/ScrollView.hh>
#include <imagine/logger/logger.h>
#include <imagine/input/DragTracker.hh>
#include <imagine/gfx/GeomRect.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Screen.hh>
#include <imagine/util/math/space.hh>
#include <imagine/util/math/int.hh>
#include <algorithm>
#include <cmath>

// minimum velocity before releasing a drag causes a scroll animation
static constexpr float SCROLL_MIN_START_VEL = 1.;
// scroll animation deceleration amount
static constexpr float SCROLL_DECEL = 1. * 60.;
// over scroll animation velocity scale
static constexpr float OVER_SCROLL_VEL_SCALE = .2 * 60.;

ScrollView::ScrollView(ViewAttachParams attach): ScrollView{{}, attach} {}

ScrollView::ScrollView(NameString name, ViewAttachParams attach):
	View{std::move(name), attach},
	animate
	{
		[this](IG::FrameParams params)
		{
			auto frames = params.elapsedFrames(std::exchange(lastFrameTimestamp, params.timestamp()));
			auto prevOffset = offset;
			if(scrollVel) // scrolling deceleration
			{
				auto prevSignBit = std::signbit(scrollVel);
				scrollVel += scrollAccel * frames;
				if(std::signbit(scrollVel) != prevSignBit) // stop when velocity overflows
					scrollVel = 0;
				//logMsg("velocity:%f", (double)scrollVel);
				offsetAsDec += scrollVel;
				offset = std::round(offsetAsDec);
				if(isOverScrolled())
					scrollVel = 0;
				if(scrollVel || isOverScrolled())
				{
					if(offset != prevOffset)
						postDraw();
					return true;
				}
			}
			else if(isOverScrolled())
			{
				//logMsg("animating over-scroll");
				int clip = offset < 0 ? 0 : offsetMax;
				int sign = offset < 0 ? 1 : -1;
				iterateTimes(frames, i)
				{
					int vel = std::abs((clip - offset) * overScrollVelScale);
					offset += sign * std::max(1, vel);
				}
				if((sign == 1 && offset >= 0)
					|| (sign == -1 && offset <= offsetMax))
				{
					//logMsg("done animating over-scroll");
					offset = clip;
				}
				else
				{
					if(offset != prevOffset)
						postDraw();
					return true;
				}
			}
			if(offset != prevOffset)
				postDraw();
			lastFrameTimestamp = {};
			return false;
		}
	}
{}

ScrollView::~ScrollView()
{
	stopScrollAnimation();
}

void ScrollView::onShow() {}

void ScrollView::onHide()
{
	setScrollOffset(offset);
}

bool ScrollView::isDoingScrollGesture() const
{
	return dragTracker.isDragging();
}

bool ScrollView::isOverScrolled() const
{
	return overScroll();
}

int ScrollView::overScroll() const
{
	return offset < 0 ? offset :
		offset > offsetMax ? offset - offsetMax :
		0;
}

void ScrollView::setContentSize(IG::WP size)
{
	IG::WP contentSize = {size.x, size.y};
	overScrollVelScale = OVER_SCROLL_VEL_SCALE / screen()->frameRate();
	dragTracker.setXDragStartDistance(-1);
	dragTracker.setYDragStartDistance(std::max(1, Config::envIsAndroid ? window().heightSMMInPixels(1.5) : window().heightSMMInPixels(1.)));
	const auto viewFrame = viewRect();
	offsetMax = std::max(0, contentSize.y - viewFrame.ySize());
	if(isOverScrolled())
	{
		window().addOnFrame(animate);
	}
	if(viewFrame.ySize() > 0)
		allowScrollWholeArea_ = contentSize.y / viewFrame.ySize() > 3;
	else
		allowScrollWholeArea_ = false;
	contentIsBiggerThanView = contentSize.y > viewFrame.ySize();
	auto scrollBarRightPadding = std::max(2, IG::makeEvenRoundedUp(window().widthSMMInPixels(.4)));
	auto scrollBarWidth = std::max(2, IG::makeEvenRoundedUp(window().widthSMMInPixels(.3)));
	scrollBarRect.x = (viewFrame.x2 - scrollBarRightPadding) - scrollBarWidth;
	scrollBarRect.x2 = scrollBarRect.x + scrollBarWidth;
	scrollBarRect.y = 0;
	scrollBarRect.y2 = std::max(10, (int)(viewFrame.ySize() * (viewFrame.ySize() / (Gfx::GC)contentSize.y)));
}

void ScrollView::drawScrollContent(Gfx::RendererCommands &cmds)
{
	using namespace Gfx;
	if(contentIsBiggerThanView && (allowScrollWholeArea_ || dragTracker.isDragging()))
	{
		cmds.setCommonProgram(CommonProgram::NO_TEX, projP.makeTranslate());
		cmds.setBlendMode(0);
		if(scrollWholeArea_)
		{
			if(dragTracker.isDragging())
				cmds.setColor(.8, .8, .8);
			else
				cmds.setColor(.5, .5, .5);
		}
		else
			cmds.setColor(.5, .5, .5);
		scrollBarRect.setYPos(
			IG::scalePointRange((Gfx::GC)offset, 0_gc, Gfx::GC(offsetMax), (Gfx::GC)viewRect().y, Gfx::GC(viewRect().y2 - scrollBarRect.ySize())));
		GeomRect::draw(cmds, scrollBarRect, projP);
	}
}

bool ScrollView::scrollInputEvent(Input::Event e)
{
	if(!e.isPointer() || (!dragTracker.isDragging() && !pointIsInView(e.pos())))
		return false;
	// mouse wheel scroll
	if(Config::Input::MOUSE_DEVICES && !dragTracker.isDragging()
		&& (e.pushed(Input::Pointer::WHEEL_UP) || e.pushed(Input::Pointer::WHEEL_DOWN)))
	{
		auto prevOffset = offset;
		auto vel = window().heightSMMInPixels(10.0);
		offset += e.mapKey() == Input::Pointer::WHEEL_UP ? -vel : vel;
		offset = std::clamp(offset, 0, offsetMax);
		if(offset != prevOffset)
			postDraw();
		return true;
	}
	// click & drag scroll
	bool isDragging = dragTracker.inputEvent(e,
		[&](Input::DragTrackerState)
		{
			stopScrollAnimation();
			velTracker = {std::chrono::duration_cast<VelocityTrackerType::TimeType>(e.time()), {(float)e.pos().y}};
			scrollVel = 0;
			onDragOffset = offset;
			const auto viewFrame = viewRect();
			if(allowScrollWholeArea_ && (e.pos().x > viewFrame.xSize() - window().widthSMMInPixels(7.5)))
			{
				logMsg("will scroll all content");
				scrollWholeArea_ = true;
			}
			else
			{
				scrollWholeArea_ = false;
			}
		},
		[&](Input::DragTrackerState state, Input::DragTrackerState)
		{
			velTracker.update(std::chrono::duration_cast<VelocityTrackerType::TimeType>(e.time()), {(float)e.pos().y});
			if(state.isDragging())
			{
				auto prevOffset = offset;
				const auto viewFrame = viewRect();
				if(scrollWholeArea_)
				{
					offset = IG::scalePointRange((Gfx::GC)e.pos().y, (Gfx::GC)viewFrame.y, (Gfx::GC)viewFrame.y + (Gfx::GC)viewFrame.ySize(), (Gfx::GC)0, (Gfx::GC)offsetMax);
					offset = std::clamp(offset, 0, offsetMax);
				}
				else
				{
					offset = onDragOffset - state.downPosDiff().y;
					if(isOverScrolled())
					{
						int clip = offset < 0 ? 0 : offsetMax;
						offset += (clip - offset) / 2;
					}
				}
				if(offset != prevOffset)
					postDraw();
			}
		},
		[&](Input::DragTrackerState state)
		{
			if(state.isDragging() && !isOverScrolled())
			{
				//logMsg("release velocity %f", (double)velTracker.velocity(0));
				scrollVel = -velTracker.velocity(0) / screen()->frameRate();
				float decelAmount = SCROLL_DECEL / screen()->frameRate();
				scrollAccel = scrollVel > 0 ? -decelAmount : decelAmount;
				offsetAsDec = offset;
				if(std::abs(scrollVel) <= SCROLL_MIN_START_VEL)
					scrollVel = 0;
			}
			if(scrollVel || isOverScrolled())
			{
				overScrollVelScale = OVER_SCROLL_VEL_SCALE / screen()->frameRate();
				window().addOnFrame(animate);
			}
			else
			{
				postDraw(); // scroll bar visual update on input release
			}
		});
	return isDragging;
}

void ScrollView::setScrollOffset(int o)
{
	dragTracker.finish();
	stopScrollAnimation();
	offset = std::clamp(o, 0, offsetMax);
}

int ScrollView::scrollOffset() const
{
	return offset;
}

void ScrollView::stopScrollAnimation()
{
	lastFrameTimestamp = {};
	window().removeOnFrame(animate);
}
