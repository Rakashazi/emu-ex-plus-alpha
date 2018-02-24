/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuLoadProgressView.hh>
#include <imagine/util/math/space.hh>

void EmuLoadProgressView::setMax(uint val)
{
	if(val)
	{
		max = val;
	}
}

void EmuLoadProgressView::setPos(uint val)
{
	pos = val;
}

void EmuLoadProgressView::setLabel(const char *labelStr)
{
	string_copy(str, labelStr);
}

void EmuLoadProgressView::place()
{
	text.setString(str.data());
	text.compile(renderer(), projP);
}

bool EmuLoadProgressView::inputEvent(Input::Event e)
{
	return true;
}

void EmuLoadProgressView::draw()
{
	if(!strlen(str.data()))
		return;
	using namespace Gfx;
	auto &r = renderer();
	projP.resetTransforms(r);
	r.setBlendMode(0);
	if(max)
	{
		r.noTexProgram.use(r);
		r.setColor(.0, .0, .75);
		Gfx::GC barHeight = text.ySize*1.5;
		auto bar = makeGCRectRel(projP.bounds().pos(LC2DO) - GP{0_gc, barHeight/2_gc},
			{IG::scalePointRange((Gfx::GC)pos, 0_gc, (Gfx::GC)max, 0_gc, projP.w), barHeight});
		GeomRect::draw(r, bar);
	}
	r.texAlphaProgram.use(r);
	r.setColor(COLOR_WHITE);
	text.draw(r, 0, 0, C2DO, projP);
}
