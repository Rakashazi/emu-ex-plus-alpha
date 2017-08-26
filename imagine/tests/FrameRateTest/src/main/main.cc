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
static Gfx::Renderer renderer;
static Gfx::Drawable drawable;
static Gfx::ProjectionPlane projP;
static Gfx::Mat4 projMat;
static Gfx::GCRect testRect;
static TestFramework *activeTest{};
static TestPicker picker{{mainWin, renderer}};
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

static void placeElements(Gfx::Renderer &r)
{
	TableView::setDefaultXIndent(projP);
	if(!activeTest)
	{
		picker.setViewRect(projP.viewport.bounds(), projP);
		picker.place();
	}
	else
	{
		activeTest->place(r, projP, testRect);
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

static void finishTest(Base::Window &win, Gfx::Renderer &r, Base::FrameTimeBase frameTime)
{
	if(activeTest)
	{
		activeTest->finish(frameTime);
	}
	cleanupTest();
	placeElements(r);
	win.postDraw();
}

TestFramework *startTest(Base::Window &win, Gfx::Renderer &r, const TestParams &t)
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
	activeTest->init(r, t.pixmapSize);
	win.postDraw();
	Base::setIdleDisplayPowerSave(false);
	Input::setKeyRepeat(false);
	initCPUFreqStatus();
	initCPULoadStatus();
	placeElements(r);

	win.screen()->addOnFrame(
		[&win](Base::Screen::FrameParams params)
		{
			auto atOnFrame = IG::Time::now();
			auto timestamp = params.timestamp();
			renderer.restoreBind();
			activeTest->frameUpdate(renderer, params.screen(), timestamp);
			activeTest->lastFramePresentTime.atOnFrame = atOnFrame;
			activeTest->lastFramePresentTime.frameTime = IG::Time::makeWithNSecs(Base::frameTimeBaseToNSecs(timestamp));
			if(activeTest->frames == framesToRun || activeTest->shouldEndTest)
			{
				finishTest(win, renderer, timestamp);
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
			drawable.freeCaches();
			renderer.finish();
		});

	{
		Gfx::Error err{};
		renderer = Gfx::Renderer::makeConfiguredRenderer(err);
		if(err)
		{
			Base::exitWithErrorMessagePrintf(-1, "Error creating renderer: %s", err->what());
			return;
		}
	}
	View::compileGfxPrograms(renderer);
	View::defaultFace = Gfx::GlyphTextureSet::makeSystem(renderer, IG::FontSettings{});
	WindowConfig winConf;

	winConf.setOnSurfaceChange(
		[](Base::Window &win, Base::Window::SurfaceChange change)
		{
			if(change.resized())
			{
				renderer.restoreBind();
				auto viewport = Gfx::Viewport::makeFromWindow(win);
				projMat = Gfx::Mat4::makePerspectiveFovRH(M_PI/4.0, viewport.realAspectRatio(), 1.0, 100.);
				projP = Gfx::ProjectionPlane::makeWithMatrix(viewport, projMat);
				testRect = projP.unProjectRect(viewport.rectWithRatioBestFitFromViewport(0, 0, 4./3., C2DO, C2DO));
				placeElements(renderer);
			}
			renderer.updateDrawableForSurfaceChange(drawable, change);
		});

	winConf.setOnDraw(
		[](Base::Window &win, Base::Window::DrawParams params)
		{
			renderer.updateCurrentDrawable(drawable, win, params, projP.viewport, projMat);
			if(!activeTest)
			{
				renderer.setClearColor(0, 0, 0);
				renderer.clear();
				picker.draw();
			}
			else
			{
				activeTest->draw(renderer);
			}
			renderer.setClipRect(false);
			if(activeTest)
			{
				activeTest->lastFramePresentTime.atWinPresent = IG::Time::now();
			}
			renderer.presentDrawable(drawable);
			if(activeTest)
			{
				activeTest->lastFramePresentTime.atWinPresentEnd = IG::Time::now();
			}
			renderer.finishPresentDrawable(drawable);
		});

	winConf.setOnInputEvent(
		[](Base::Window &, Input::Event e)
		{
			renderer.restoreBind();
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

	renderer.initWindow(mainWin, winConf);
	mainWin.setTitle("Frame Rate Test");
	uint faceSize = mainWin.heightSMMInPixels(3.5);
	View::defaultFace.setFontSettings(renderer, faceSize);
	View::defaultFace.precacheAlphaNum(renderer);
	View::defaultFace.precache(renderer, ":.%()");
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
