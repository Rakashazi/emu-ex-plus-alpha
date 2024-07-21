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
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/BasicEffect.hh>
#include <imagine/gfx/Mat4.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Screen.hh>
#include <imagine/util/math.hh>
#include <algorithm>
#include <cmath>

namespace IG
{

// minimum velocity before releasing a drag causes a scroll animation
static constexpr float SCROLL_MIN_START_VEL = 1.;
// scroll animation deceleration amount
static constexpr float SCROLL_DECEL = 1. * 60.;
// over scroll animation velocity scale
static constexpr float OVER_SCROLL_VEL_SCALE = .2 * 60.;

ScrollView::ScrollView(ViewAttachParams attach):
	View{attach},
	animate
	{
		[this](IG::FrameParams params)
		{
			auto frames = params.elapsedFrames(std::exchange(lastFrameTimestamp, params.timestamp));
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
				for([[maybe_unused]] auto i : iotaCount(frames))
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
	},
	scrollBarQuads{attach.rendererTask, {.size = 1}} {}

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

void ScrollView::setContentSize(WSize contentSize)
{
	overScrollVelScale = OVER_SCROLL_VEL_SCALE / screen()->frameRate();
	dragTracker.setDragStartPixels(std::max(1, Config::envIsAndroid ? window().heightMMInPixels(1.5) : window().heightMMInPixels(1.)));
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
	auto scrollBarRightPadding = std::max(2, IG::makeEvenRoundedUp(window().widthMMInPixels(.4)));
	auto scrollBarWidth = std::max(2, IG::makeEvenRoundedUp(window().widthMMInPixels(.3)));
	WRect scrollBarRect;
	scrollBarRect.x = (viewFrame.x2 - scrollBarRightPadding) - scrollBarWidth;
	scrollBarRect.x2 = scrollBarRect.x + scrollBarWidth;
	scrollBarRect.y = 0;
	scrollBarRect.y2 = std::max(10, (int)(viewFrame.ySize() * (viewFrame.ySize() / (float)contentSize.y)));
	scrollBarYSize = scrollBarRect.ySize();
	scrollBarQuads.write(0, {.bounds = scrollBarRect.as<int16_t>()});
}

void ScrollView::drawScrollContent(Gfx::RendererCommands &cmds) const
{
	using namespace IG::Gfx;
	if(contentIsBiggerThanView && (allowScrollWholeArea_ || dragTracker.isDragging()))
	{
		cmds.basicEffect().disableTexture(cmds);
		cmds.set(BlendMode::OFF);
		if(scrollWholeArea_)
		{
			if(dragTracker.isDragging())
				cmds.setColor({.8, .8, .8});
			else
				cmds.setColor({.5, .5, .5});
		}
		else
			cmds.setColor({.5, .5, .5});
		cmds.basicEffect().setModelView(cmds, Mat4::makeTranslate(WPt{0,
			remap(offset, 0, offsetMax, viewRect().y, viewRect().y2 - scrollBarYSize)}));
		cmds.drawQuad(scrollBarQuads, 0);
	}
}

bool ScrollView::scrollInputEvent(const Input::MotionEvent &e)
{
	if(!e.isPointer() || (!dragTracker.isDragging() && !pointIsInView(e.pos())))
		return false;
	// mouse wheel scroll
	if(Config::Input::MOUSE_DEVICES && !dragTracker.isDragging() && e.scrolledVertical())
	{
		auto prevOffset = offset;
		auto vel = window().heightMMInPixels(10.0);
		offset += e.scrolledVertical() < 0 ? -vel : vel;
		offset = std::clamp(offset, 0, offsetMax);
		if(offset != prevOffset)
			postDraw();
		return true;
	}
	// click & drag scroll
	bool isDragging = dragTracker.inputEvent(e,
		[&](Input::DragTrackerState, auto)
		{
			stopScrollAnimation();
			velTracker = {e.time(), {(float)e.pos().y}};
			scrollVel = 0;
			onDragOffset = offset;
			const auto viewFrame = viewRect();
			if(allowScrollWholeArea_ && (e.pos().x > viewFrame.xSize() - window().widthMMInPixels(7.5)))
			{
				logMsg("will scroll all content");
				scrollWholeArea_ = true;
			}
			else
			{
				scrollWholeArea_ = false;
			}
		},
		[&](Input::DragTrackerState state, Input::DragTrackerState, auto)
		{
			velTracker.update(e.time(), {(float)e.pos().y});
			if(state.isDragging())
			{
				auto prevOffset = offset;
				const auto viewFrame = viewRect();
				if(scrollWholeArea_)
				{
					offset = IG::remap((float)e.pos().y, (float)viewFrame.y, (float)viewFrame.y + (float)viewFrame.ySize(), 0.f, (float)offsetMax);
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
		[&](Input::DragTrackerState state, auto)
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
	dragTracker.reset();
	stopScrollAnimation();
	offset = std::clamp(o, 0, offsetMax);
}

void ScrollView::stopScrollAnimation()
{
	lastFrameTimestamp = {};
	window().removeOnFrame(animate);
}

}
