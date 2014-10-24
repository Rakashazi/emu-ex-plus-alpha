#pragma once

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

#include <imagine/gfx/GfxText.hh>
#include <imagine/gui/View.hh>
#include <imagine/util/Interpolator.hh>
#include <imagine/config/version.h>

#ifdef ENV_NOTE
#define PLATFORM_INFO_STR ENV_NOTE " (" CONFIG_ARCH_STR ")"
#else
#define PLATFORM_INFO_STR "(" CONFIG_ARCH_STR ")"
#endif
#define CREDITS_INFO_STRING "Built : " __DATE__ "\n" PLATFORM_INFO_STR "\n\n"

class CreditsView : public View
{
private:
	Gfx::Text text;
	TimedInterpolator<float> fade;
	Base::Screen::OnFrameDelegate animate;
	const char *str;
	IG::WindowRect rect;

public:
	CreditsView(const char *str, Base::Window &win);
	IG::WindowRect &viewRect() override { return rect; }
	void draw() override;
	void place() override;
	void inputEvent(const Input::Event &e) override;
	void init();
	void deinit() override;
};
