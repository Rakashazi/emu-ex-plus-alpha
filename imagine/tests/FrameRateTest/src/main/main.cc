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

static constexpr uint framesToRun = 60*60;
static Base::Window mainWin;
static Gfx::Renderer renderer;
static Gfx::RendererTask rendererTask{renderer};
static Gfx::DrawableHolder drawableHolder;
static Gfx::ProjectionPlane projP;
static Gfx::Mat4 projMat;
static IG::WindowRect testRectWin;
static Gfx::GCRect testRect;
static TestFramework *activeTest{};
static std::unique_ptr<TestPicker> picker;
static TestParams testParam[]
{
	{TEST_CLEAR},
	{TEST_DRAW, {320, 224}},
	{TEST_WRITE, {320, 224}},
};
#ifdef __ANDROID__
static std::unique_ptr<Base::RootCpufreqParamSetter> cpuFreq{};
#endif

static void placeElements(Gfx::Renderer &r)
{
	TableView::setDefaultXIndent(projP);
	if(!activeTest)
	{
		picker->setViewRect(projP.viewport.bounds(), projP);
		picker->place();
	}
	else
	{
		activeTest->place(r, projP, testRect);
	}
}

static void cleanupTest(TestFramework *test)
{
	rendererTask.runSync([](Gfx::RendererTask &){ activeTest = nullptr; });
	if(test)
	{
		test->deinit();
	}
	delete test;
	deinitCPUFreqStatus();
	deinitCPULoadStatus();
	Base::setIdleDisplayPowerSave(true);
	Input::setKeyRepeat(true);
	#ifdef __ANDROID__
	if(cpuFreq)
		cpuFreq->setDefaults();
	Base::setSustainedPerformanceMode(false);
	#endif
}

static void finishTest(Base::Window &win, Gfx::Renderer &r, Base::FrameTimeBase frameTime)
{
	if(activeTest)
	{
		activeTest->finish(frameTime);
	}
	cleanupTest(activeTest);
	placeElements(r);
	win.postDraw();
}

TestFramework *startTest(Base::Window &win, Gfx::Renderer &r, const TestParams &t)
{
	#ifdef __ANDROID__
	if(cpuFreq)
		cpuFreq->setLowLatency();
	Base::setSustainedPerformanceMode(true);
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
	Base::setIdleDisplayPowerSave(false);
	Input::setKeyRepeat(false);
	initCPUFreqStatus();
	initCPULoadStatus();
	placeElements(r);

	win.screen()->addOnFrame(
		[&win](Base::Screen::FrameParams params)
		{
			if(unlikely(!activeTest))
				return false;
			auto atOnFrame = IG::Time::now();
			auto timestamp = params.timestamp();
			if(activeTest->started)
			{
				if(activeTest->lastFramePresentTime.atWinPresent < activeTest->lastFramePresentTime.atOnFrame)
				{
					/*logWarn("previous frame:%f not yet presented, last present time:%f",
						double(activeTest->lastFramePresentTime.atWinPresent),
						double(activeTest->lastFramePresentTime.atOnFrame));*/
				}
				activeTest->frameUpdate(rendererTask, win, timestamp);
			}
			else
			{
				activeTest->started = true;
			}
			activeTest->lastFramePresentTime.atOnFrame = atOnFrame;
			if(activeTest->frames == framesToRun || activeTest->shouldEndTest)
			{
				finishTest(win, renderer, timestamp);
				return false;
			}
			else
			{
				win.postDraw();
				return true;
			}
		});
	return activeTest;
}

namespace Base
{

void onInit(int argc, char** argv)
{
	Base::addOnExit(
		[](bool backgrounded)
		{
			if(activeTest)
			{
				auto time = Base::frameTimeBaseFromNSecs(IG::Time::now().nSecs());
				activeTest->finish(time);
			}
			cleanupTest(activeTest);
			View::defaultFace.freeCaches();
			if(!backgrounded)
			{
				picker.reset();
			}
			return true;
		});

	{
		Gfx::Error err{};
		renderer = Gfx::Renderer::makeConfiguredRenderer(Gfx::Renderer::ThreadMode::AUTO, err);
		if(err)
		{
			Base::exitWithErrorMessagePrintf(-1, "Error creating renderer: %s", err->what());
			return;
		}
	}
	rendererTask.start();
	View::compileGfxPrograms(renderer);
	View::defaultFace = Gfx::GlyphTextureSet::makeSystem(renderer, IG::FontSettings{});
	WindowConfig winConf;

	winConf.setOnSurfaceChange(
		[](Base::Window &win, Base::Window::SurfaceChange change)
		{
			rendererTask.updateDrawableForSurfaceChange(drawableHolder, change);
			if(change.resized())
			{
				auto viewport = Gfx::Viewport::makeFromWindow(win);
				projMat = Gfx::Mat4::makePerspectiveFovRH(M_PI/4.0, viewport.realAspectRatio(), 1.0, 100.);
				projP = Gfx::ProjectionPlane::makeWithMatrix(viewport, projMat);
				testRectWin = viewport.rectWithRatioBestFitFromViewport(0, 0, 4./3., C2DO, C2DO);
				testRect = projP.unProjectRect(testRectWin);
				placeElements(renderer);
			}
		});

	winConf.setOnDraw(
		[](Base::Window &win, Base::Window::DrawParams params)
		{
			if(!activeTest)
			{
				picker->prepareDraw();
			}
			else if(activeTest->started)
			{
				activeTest->prepareDraw(renderer);
			}
			rendererTask.draw(drawableHolder, win, params, {},
				[](Gfx::Drawable &drawable, Base::Window &win, Gfx::SyncFence fence, Gfx::RendererDrawTask task)
				{
					auto cmds = task.makeRendererCommands(drawable, projP.viewport, projMat);
					cmds.setClipTest(false);
					cmds.waitSync(fence);
					if(!activeTest)
					{
						cmds.setClearColor(0, 0, 0);
						cmds.clear();
						picker->draw(cmds);
					}
					else if(activeTest->started)
					{
						activeTest->draw(cmds, renderer.makeClipRect(win, testRectWin));
						activeTest->lastFramePresentTime.atWinPresent = IG::Time::now();
					}
					cmds.present();
					if(activeTest && activeTest->started)
					{
						activeTest->presentedTest(cmds);
						activeTest->lastFramePresentTime.atWinPresentEnd = IG::Time::now();
					}
				});
			return false;
		});

	winConf.setOnInputEvent(
		[](Base::Window &, Input::Event e)
		{
			if(!activeTest)
			{
				if(e.pushed() && e.isDefaultCancelButton())
				{
					Base::exit();
					return true;
				}
				return picker->inputEvent(e);
			}
			else if(e.pushed() && (e.isDefaultCancelButton() || Config::envIsIOS))
			{
				logMsg("canceled activeTest from input");
				activeTest->shouldEndTest = true;
				return true;
			}
			return false;
		});

	renderer.initWindow(mainWin, winConf);
	mainWin.setTitle("Frame Rate Test");
	uint faceSize = mainWin.heightSMMInPixels(3.5);
	View::defaultFace.setFontSettings(renderer, faceSize);
	View::defaultFace.precacheAlphaNum(renderer);
	View::defaultFace.precache(renderer, ":.%()");
	picker = std::make_unique<TestPicker>(ViewAttachParams{mainWin, rendererTask});
	picker->setTests(testParam, IG::size(testParam));
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
	#endif
}

}
