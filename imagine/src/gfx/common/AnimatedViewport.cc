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

#define LOGTAG "AnimatedViewport"
#include <imagine/gfx/AnimatedViewport.hh>
#include <imagine/gfx/Viewport.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Screen.hh>
#include <imagine/logger/logger.h>

namespace Gfx
{

void AnimatedViewport::start(Base::Window &w, Gfx::Viewport begin, Gfx::Viewport end)
{
	cancel();
	win = &w;
	if(begin == end)
	{
		animator[0] = {end.bounds().x};
		animator[1] = {end.bounds().y};
		animator[2] = {end.bounds().x2};
		animator[3] = {end.bounds().y2};
		return;
	}
	logMsg("animating from viewport %d:%d:%d:%d to %d:%d:%d:%d",
		begin.bounds().x, begin.bounds().y, begin.bounds().x2, begin.bounds().y2,
		end.bounds().x, end.bounds().y, end.bounds().x2, end.bounds().y2);
	auto now = IG::steadyClockTimestamp();
	IG::FrameTime duration{IG::Milliseconds{165}};
	animator[0] = {begin.bounds().x, end.bounds().x, {}, now, duration};
	animator[1] = {begin.bounds().y, end.bounds().y, {}, now, duration};
	animator[2] = {begin.bounds().x2, end.bounds().x2, {}, now, duration};
	animator[3] = {begin.bounds().y2, end.bounds().y2, {}, now, duration};
	win->setNeedsCustomViewportResize(true);
	animate =
		[this](IG::FrameParams params)
		{
			bool updating{};
			for(auto &d : animator)
			{
				updating |= d.update(params.timestamp());
			}
			win->setNeedsCustomViewportResize(true);
			win->postDraw();
			if(!updating)
			{
				animate = {};
				return false;
			}
			else
			{
				return true;
			}
		};
	w.addOnFrame(animate);
}

void AnimatedViewport::finish()
{
	cancel();
	animator[0].finish();
	animator[1].finish();
	animator[2].finish();
	animator[3].finish();
}

bool AnimatedViewport::isFinished() const
{
	return animator[0].isFinished();
}

void AnimatedViewport::cancel()
{
	if(animate)
	{
		assert(win);
		win->removeOnFrame(animate);
		animate = {};
	}
}

Gfx::Viewport AnimatedViewport::viewport() const
{
	return win ? Gfx::Viewport::makeFromWindow(*win,
		{{animator[0], animator[1]}, {animator[2], animator[3]}}) : Gfx::Viewport{};
}

}
