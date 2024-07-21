#pragma once

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

#include <imagine/gfx/Renderer.hh>
#include <imagine/base/Application.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gui/View.hh>
#include <imagine/gui/ViewManager.hh>
#include <imagine/font/Font.hh>
#ifdef __ANDROID__
#include <imagine/base/android/RootCpufreqParamSetter.hh>
#endif
#include <optional>

namespace FrameRateTest
{

using namespace IG;

class FrameRateTestApplication final: public IG::Application
{
public:
	FrameRateTestApplication(IG::ApplicationInitParams, IG::ApplicationContext &);
	TestFramework *startTest(IG::Window &, const TestParams &t);

private:
	IG::FontManager fontManager;
	Gfx::Renderer renderer;
	IG::ViewManager viewManager;
	#ifdef __ANDROID__
	std::optional<IG::RootCpufreqParamSetter> cpuFreq{};
	#endif

	void setActiveTestHandlers(IG::Window &win);
	void setPickerHandlers(IG::Window &);
	void placeElements(const IG::Window &);
	void finishTest(Window &, SteadyClockTimePoint);
	void updateWindowSurface(Window &, Window::SurfaceChange);
};

}
