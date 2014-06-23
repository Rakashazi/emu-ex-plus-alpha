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

#include <imagine/gui/View.hh>

ResourceFace *View::defaultFace = nullptr;
ResourceFace *View::defaultSmallFace = nullptr;
bool View::needsBackControl = needsBackControlDefault;

void View::pushAndShow(View &v, bool needsNavView)
{
	assert(controller);
	controller->pushAndShow(v, needsNavView);
}

void View::dismiss()
{
	if(controller)
	{
		controller->dismissView(*this);
	}
	else
	{
		logWarn("called dismiss with no controller");
	}
}

bool View::compileGfxPrograms()
{
	auto compiled = Gfx::noTexProgram.compile();
	// for text
	compiled |= Gfx::texAlphaProgram.compile();
	compiled |= Gfx::texAlphaReplaceProgram.compile();
	return compiled;
}
