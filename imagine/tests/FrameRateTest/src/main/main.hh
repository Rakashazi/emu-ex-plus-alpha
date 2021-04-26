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
#ifdef __ANDROID__
#include <imagine/base/android/RootCpufreqParamSetter.hh>
#endif
#include <optional>

class FrameRateTestApplication final: public Base::Application
{
public:
	FrameRateTestApplication(Base::ApplicationInitParams, Base::ApplicationContext &, Gfx::Error &);
	TestFramework *startTest(Base::Window &, const TestParams &t);

private:
	Gfx::Renderer renderer;
	ViewManager viewManager{};
	#ifdef __ANDROID__
	std::optional<Base::RootCpufreqParamSetter> cpuFreq{};
	#endif

	void setActiveTestHandlers(Base::Window &win);
	void setPickerHandlers(Base::Window &);
	void placeElements(const Base::Window &);
	void finishTest(Base::Window &, IG::FrameTime);
};

static FrameRateTestApplication &mainApp(Base::ApplicationContext ctx)
{
	return static_cast<FrameRateTestApplication &>(ctx.application());
}
