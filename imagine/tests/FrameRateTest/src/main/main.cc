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
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/DrawableHolder.hh>
#include <imagine/base/Base.hh>
#include <imagine/base/Screen.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/platformExtras.hh>
#include <imagine/util/string.h>
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
#ifdef __ANDROID__
static std::unique_ptr<Base::RootCpufreqParamSetter> cpuFreq{};
#endif

static void placeElements(Base::Window &win, Gfx::Renderer &r)
{
	TableView::setDefaultXIndent(win, projP);
	if(!activeTest)
	{
		picker->setViewRect(projP.viewport().bounds(), projP);
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
	delete test;
	deinitCPUFreqStatus();
	deinitCPULoadStatus();
	Base::setIdleDisplayPowerSave(true);
	#ifdef __ANDROID__
	if(cpuFreq)
		cpuFreq->setDefaults();
	Base::setSustainedPerformanceMode(false);
	#endif
}

static void finishTest(Base::Window &win, Gfx::Renderer &r, IG::FrameTime frameTime)
{
	if(activeTest)
	{
		activeTest->finish(frameTime);
	}
	cleanupTest(activeTest);
	placeElements(win, r);
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
	activeTest->init(r, t.pixmapSize, t.bufferMode);
	Base::setIdleDisplayPowerSave(false);
	initCPUFreqStatus();
	initCPULoadStatus();
	placeElements(win, r);

	Base::OnFrameDelegate onFrameUpdate =
		[&win](Base::FrameParams params)
		{
			if(unlikely(!activeTest))
				return false;
			auto atOnFrame = IG::steadyClockTimestamp();
			if(activeTest->started)
			{
				if(activeTest->lastFramePresentTime.atWinPresent < activeTest->lastFramePresentTime.atOnFrame)
				{
					/*logWarn("previous frame:%f not yet presented, last present time:%f",
						double(activeTest->lastFramePresentTime.atWinPresent),
						double(activeTest->lastFramePresentTime.atOnFrame));*/
				}
				activeTest->frameUpdate(rendererTask, win, params);
			}
			else
			{
				activeTest->started = true;
			}
			activeTest->lastFramePresentTime.atOnFrame = atOnFrame;
			if(activeTest->frames == framesToRun || activeTest->shouldEndTest)
			{
				finishTest(win, renderer, params.timestamp());
				return false;
			}
			else
			{
				win.postDraw();
				return true;
			}
		};
	if(Base::Screen::supportsTimestamps())
	{
		win.screen()->addOnFrame(onFrameUpdate);
	}
	else
	{
		drawableHolder.addOnFrame(onFrameUpdate);
		win.postDraw();
	}
	return activeTest;
}

namespace Base
{

void onInit(int argc, char** argv)
{
	Base::addOnResume(
		[](bool focused)
		{
			rendererTask.start();
			picker->prepareDraw();
			if(activeTest)
			{
				activeTest->prepareDraw(renderer);
			}
			return true;
		});

	Base::addOnExit(
		[](bool backgrounded)
		{
			if(activeTest)
			{
				auto time = IG::steadyClockTimestamp();
				activeTest->finish(time);
			}
			cleanupTest(activeTest);
			View::defaultFace.freeCaches();
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
				placeElements(win, renderer);
			}
		});

	winConf.setOnDraw(
		[](Base::Window &win, Base::Window::DrawParams params)
		{
			rendererTask.draw(drawableHolder, win, params, {},
				[](Gfx::Drawable &drawable, Base::Window &win, Gfx::SyncFence fence, Gfx::RendererDrawTask task)
				{
					auto cmds = task.makeRendererCommands(drawable, projP.viewport(), projMat);
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
						activeTest->lastFramePresentTime.atWinPresent = IG::steadyClockTimestamp();
					}
					cmds.present();
					if(activeTest && activeTest->started)
					{
						activeTest->presentedTest(cmds);
						activeTest->lastFramePresentTime.atWinPresentEnd = IG::steadyClockTimestamp();
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
	std::vector<TestDesc> testDesc;
	testDesc.emplace_back(TEST_CLEAR, "Clear");
	IG::WP pixmapSize{256, 256};
	for(auto desc: renderer.textureBufferModes())
	{
		testDesc.emplace_back(TEST_DRAW, string_makePrintf<64>("Draw RGB565 %ux%u (%s)", pixmapSize.x, pixmapSize.y, desc.name).data(),
			pixmapSize, desc.mode);
		testDesc.emplace_back(TEST_WRITE, string_makePrintf<64>("Write RGB565 %ux%u (%s)", pixmapSize.x, pixmapSize.y, desc.name).data(),
			pixmapSize, desc.mode);
	}
	picker = std::make_unique<TestPicker>(ViewAttachParams{mainWin, rendererTask});
	picker->setTests(testDesc.data(), testDesc.size());
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
