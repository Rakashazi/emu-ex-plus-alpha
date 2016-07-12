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

#define LOGTAG "main"
#include <memory>
#include <imagine/logger/logger.h>
#include <imagine/gfx/GfxSprite.hh>
#include <imagine/gfx/GfxText.hh>
#include "tests.hh"
#include "TestPicker.hh"
#include "cpuUtils.hh"
#ifdef __ANDROID__
#include <imagine/base/android/RootCpufreqParamSetter.hh>
#endif

static const uint framesToRun = 60*60;
static Base::Window mainWin;
static Gfx::ProjectionPlane projP;
static Gfx::Mat4 projMat;
static Gfx::GCRect testRect;
static TestFramework *activeTest{};
static TestPicker picker{mainWin};
static TestParams testParam[] =
{
	{TEST_CLEAR},
	{TEST_DRAW, {320, 224}},
	{TEST_WRITE, {320, 224}},
};
#ifdef __ANDROID__
static std::unique_ptr<Base::RootCpufreqParamSetter> cpuFreq{};
static std::unique_ptr<Base::UserActivityFaker> userActivityFaker{};
#endif

static void placeElements()
{
	TableView::setDefaultXIndent(projP);
	if(!activeTest)
	{
		picker.setViewRect(projP.viewport.bounds(), projP);
		picker.place();
	}
	else
	{
		activeTest->place(projP, testRect);
	}
}

static void cleanupTest()
{
	if(activeTest)
	{
		activeTest->deinit();
	}
	delete activeTest;
	activeTest = nullptr;
	deinitCPUFreqStatus();
	deinitCPULoadStatus();
	Base::setIdleDisplayPowerSave(true);
	Input::setKeyRepeat(true);
	#ifdef __ANDROID__
	if(cpuFreq)
		cpuFreq->setDefaults();
	if(userActivityFaker)
		userActivityFaker->stop();
	#endif
}

static void finishTest(Base::Window &win, Base::FrameTimeBase frameTime)
{
	if(activeTest)
	{
		activeTest->finish(frameTime);
	}
	cleanupTest();
	Gfx::setClearColor(0, 0, 0);
	placeElements();
	win.postDraw();
}

TestFramework *startTest(Base::Window &win, const TestParams &t)
{
	#ifdef __ANDROID__
	if(cpuFreq)
		cpuFreq->setLowLatency();
	if(userActivityFaker)
		userActivityFaker->start();
	#endif

	switch(t.test)
	{
		bcase TEST_CLEAR:
			activeTest = new ClearTest{};
		bcase TEST_DRAW:
			activeTest = new DrawTest{};
		bcase TEST_WRITE:
			activeTest = new WriteTest{};
	}
	activeTest->init(t.pixmapSize);
	win.postDraw();
	Base::setIdleDisplayPowerSave(false);
	Input::setKeyRepeat(false);
	initCPUFreqStatus();
	initCPULoadStatus();
	placeElements();

	win.screen()->addOnFrame(
		[&win](Base::Screen::FrameParams params)
		{
			auto atOnFrame = IG::Time::now();
			auto timestamp = params.timestamp();
			Gfx::bind();
			activeTest->frameUpdate(params.screen(), timestamp);
			activeTest->lastFramePresentTime.atOnFrame = atOnFrame;
			activeTest->lastFramePresentTime.frameTime = IG::Time::makeWithNSecs(Base::frameTimeBaseToNSecs(timestamp));
			if(activeTest->frames == framesToRun || activeTest->shouldEndTest)
			{
				finishTest(win, timestamp);
			}
			else
			{
				win.postDraw();
				params.readdOnFrame();
			}
		});
	return activeTest;
}

namespace Base
{

void onInit(int argc, char** argv)
{
	Base::setOnExit(
		[](bool backgrounded)
		{
			cleanupTest();
			if(!backgrounded)
			{
				View::defaultFace->free();
			}
		});

	Gfx::init();
	View::compileGfxPrograms();
	View::defaultFace = ResourceFace::loadSystem();
	assert(View::defaultFace);
	WindowConfig winConf;

	winConf.setOnSurfaceChange(
		[](Base::Window &win, Base::Window::SurfaceChange change)
		{
			if(change.resized())
			{
				Gfx::bind();
				auto viewport = Gfx::Viewport::makeFromWindow(win);
				projMat = Gfx::Mat4::makePerspectiveFovRH(M_PI/4.0, viewport.realAspectRatio(), 1.0, 100.);
				projP = Gfx::ProjectionPlane::makeWithMatrix(viewport, projMat);
				testRect = projP.unProjectRect(viewport.rectWithRatioBestFitFromViewport(0, 0, 4./3., C2DO, C2DO));
				placeElements();
			}
		});

	winConf.setOnDraw(
		[](Base::Window &win, Base::Window::DrawParams params)
		{
			Gfx::updateCurrentWindow(win, params, projP.viewport, projMat);
			if(!activeTest)
			{
				Gfx::clear();
				picker.draw();
			}
			else
			{
				activeTest->draw();
			}
			Gfx::setClipRect(false);
			if(activeTest)
			{
				activeTest->lastFramePresentTime.atWinPresent = IG::Time::now();
			}
			Gfx::presentWindow(win);
			if(activeTest)
			{
				activeTest->lastFramePresentTime.atWinPresentEnd = IG::Time::now();
			}
		});

	winConf.setOnInputEvent(
		[](Base::Window &, Input::Event e)
		{
			Gfx::bind();
			if(!activeTest)
			{
				if(e.pushed() && e.isDefaultCancelButton())
				{
					Base::exit();
				}
				picker.inputEvent(e);
			}
			else if(e.pushed() && (e.isDefaultCancelButton() || Config::envIsIOS))
			{
				logMsg("canceled activeTest from input");
				activeTest->shouldEndTest = true;
			}
		});

	Gfx::initWindow(mainWin, winConf);
	mainWin.setTitle("Frame Rate Test");
	uint faceSize = mainWin.heightSMMInPixels(3.5);
	View::defaultFace->applySettings(faceSize);
	View::defaultFace->precacheAlphaNum();
	View::defaultFace->precache(":.%()");
	picker.setTests(testParam, IG::size(testParam));
	mainWin.show();

	#ifdef __ANDROID__
	bool manageCPUFreq = false;
	if(manageCPUFreq)
	{
		cpuFreq = std::make_unique<Base::RootCpufreqParamSetter>();
		if(!(*cpuFreq))
		{
			cpuFreq.reset();
		}
	}
	bool fakeUserActivity = true;
	if(fakeUserActivity)
	{
		userActivityFaker = std::make_unique<Base::UserActivityFaker>();
	}
	#endif
}

}
