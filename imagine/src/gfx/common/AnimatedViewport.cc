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

#include <imagine/gfx/AnimatedViewport.hh>
#include <imagine/logger/logger.h>

namespace Gfx
{

void AnimatedViewport::start(Base::Window &w, Gfx::Viewport begin, Gfx::Viewport end)
{
	cancel();
	win = &w;
	if(begin == end)
	{
		animator[0].set(end.bounds().x);
		animator[1].set(end.bounds().y);
		animator[2].set(end.bounds().x2);
		animator[3].set(end.bounds().y2);
		return;
	}
	logMsg("animating from viewport %d:%d:%d:%d to %d:%d:%d:%d",
		begin.bounds().x, begin.bounds().y, begin.bounds().x2, begin.bounds().y2,
		end.bounds().x, end.bounds().y, end.bounds().x2, end.bounds().y2);
	auto type = INTERPOLATOR_TYPE_EASEINOUTQUAD;
	int time = 10;
	animator[0].set(begin.bounds().x, end.bounds().x, type, time);
	animator[1].set(begin.bounds().y, end.bounds().y, type, time);
	animator[2].set(begin.bounds().x2, end.bounds().x2, type, time);
	animator[3].set(begin.bounds().y2, end.bounds().y2, type, time);
	win->setNeedsCustomViewportResize(true);
	animate =
		[this](Base::Screen &screen, Base::Screen::FrameParams param)
		{
			for(auto &d : animator)
			{
				d.update(1);
			}
			win->setNeedsCustomViewportResize(true);
			if(!animator[0].isComplete())
			{
				screen.postOnFrame(param.thisOnFrame());
			}
			else
			{
				animate = {};
			}
		};
	w.screen()->postOnFrame(animate);
}

void AnimatedViewport::finish()
{
	cancel();
	animator[0].set(animator[0].destVal);
	animator[1].set(animator[1].destVal);
	animator[2].set(animator[2].destVal);
	animator[3].set(animator[3].destVal);
}

bool AnimatedViewport::isFinished()
{
	return animator[0].isComplete();
}

void AnimatedViewport::cancel()
{
	if(animate)
	{
		assert(win);
		win->screen()->removeOnFrame(animate);
		animate = {};
	}
}

Gfx::Viewport AnimatedViewport::viewport()
{
	return win ? Gfx::Viewport::makeFromWindow(*win,
		{animator[0].now(), animator[1].now(), animator[2].now(), animator[3].now()}) : Gfx::Viewport{};
}

}
