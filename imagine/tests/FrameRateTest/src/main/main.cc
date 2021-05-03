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
#include <imagine/logger/logger.h>
#include <imagine/gfx/GfxSprite.hh>
#include <imagine/gfx/GfxText.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/Projection.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/base/Screen.hh>
#include <imagine/base/Window.hh>
#include <imagine/util/string.h>
#include "tests.hh"
#include "TestPicker.hh"
#include "cpuUtils.hh"
#include "main.hh"
#include <meta.h>
#include <memory>

static constexpr unsigned framesToRun = 60*60;

struct WindowData
{
	WindowData(ViewAttachParams attachParams):picker{attachParams} {}

	Gfx::Projection proj{};
	IG::WindowRect testRectWin{};
	Gfx::GCRect testRect{};
	TestPicker picker;
	std::unique_ptr<TestFramework> activeTest{};
};

static WindowData &windowData(const Base::Window &win)
{
	return *win.appData<WindowData>();
}

FrameRateTestApplication::FrameRateTestApplication(Base::ApplicationInitParams initParams,
	Base::ApplicationContext &ctx, Gfx::Error &rendererErr):
	Application{initParams},
	renderer{ctx, rendererErr}
{
	if(rendererErr)
	{
		ctx.exitWithErrorMessagePrintf(-1, "Error creating renderer: %s", rendererErr->what());
		return;
	}

	Base::WindowConfig winConf;
	winConf.setTitle(ctx.applicationName);

	ctx.makeWindow(winConf,
		[this](Base::ApplicationContext ctx, Base::Window &win)
		{
			if(auto err = renderer.initMainTask(&win);
				err)
			{
				ctx.exitWithErrorMessagePrintf(-1, "Error creating renderer: %s", err->what());
				return;
			}
			viewManager = {renderer};
			auto defaultFace = Gfx::GlyphTextureSet::makeSystem(renderer, win.heightSMMInPixels(3.5));
			defaultFace.precacheAlphaNum(renderer);
			defaultFace.precache(renderer, ":.%()");
			viewManager.setDefaultFace(std::move(defaultFace));
			auto &winData = win.makeAppData<WindowData>(ViewAttachParams{viewManager, win, renderer.task()});
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
			auto &picker = winData.picker;
			picker.setTests(testDesc.data(), testDesc.size());
			setPickerHandlers(win);

			win.setOnSurfaceChange(
				[this](Base::Window &win, Base::Window::SurfaceChange change)
				{
					auto &winData = windowData(win);
					renderer.task().updateDrawableForSurfaceChange(win, change);
					if(change.resized())
					{
						auto viewport = Gfx::Viewport::makeFromWindow(win);
						winData.proj = {viewport, Gfx::Mat4::makePerspectiveFovRH(M_PI/4.0, viewport.realAspectRatio(), 1.0, 100.)};
						winData.testRectWin = viewport.rectWithRatioBestFitFromViewport(0, 0, 4./3., C2DO, C2DO);
						winData.testRect = winData.proj.plane().unProjectRect(winData.testRectWin);
						placeElements(win);
					}
				});

			win.setOnInputEvent(
				[this](Base::Window &win, Input::Event e)
				{
					auto &activeTest = windowData(win).activeTest;
					if(!activeTest)
					{
						if(e.pushed() && !e.repeated() && e.isDefaultCancelButton())
						{
							win.appContext().exit();
							return true;
						}
						return windowData(win).picker.inputEvent(e);
					}
					else if(e.pushed() && (e.isDefaultCancelButton() || Config::envIsIOS))
					{
						logMsg("canceled activeTest from input");
						activeTest->shouldEndTest = true;
						return true;
					}
					else if(e.pushed(Input::Keycode::D))
					{
						logMsg("posting extra draw");
						win.postDraw();
					}
					return false;
				});

			ctx.addOnResume(
				[this, &win](Base::ApplicationContext, bool focused)
				{
					windowData(win).picker.prepareDraw();
					auto &activeTest = windowData(win).activeTest;
					if(activeTest)
					{
						activeTest->prepareDraw(renderer);
					}
					return true;
				});

			ctx.addOnExit(
				[this, &win](Base::ApplicationContext ctx, bool backgrounded)
				{
					if(backgrounded)
					{
						if(windowData(win).activeTest)
						{
							finishTest(win, IG::steadyClockTimestamp());
						}
						viewManager.defaultFace().freeCaches();
					}
					return true;
				});

			win.show();
		});

	#ifdef __ANDROID__
	bool manageCPUFreq = false;
	if(manageCPUFreq)
	{
		cpuFreq.emplace();
		if(!(*cpuFreq))
		{
			cpuFreq.reset();
		}
	}
	#endif
}

void FrameRateTestApplication::setPickerHandlers(Base::Window &win)
{
	win.setOnDraw(
		[&task = renderer.task()](Base::Window &win, Base::Window::DrawParams params)
		{
			auto &winData = windowData(win);
			return task.draw(win, params, {}, winData.proj.plane().viewport(), winData.proj.matrix(),
				[&picker = winData.picker](Base::Window &win, Gfx::RendererCommands &cmds)
				{
					cmds.clear();
					picker.draw(cmds);
					cmds.setClipTest(false);
					cmds.present();
				});
		});
}

void FrameRateTestApplication::setActiveTestHandlers(Base::Window &win)
{
	win.addOnFrame([this, &win](Base::FrameParams params)
		{
			auto atOnFrame = IG::steadyClockTimestamp();
			renderer.setPresentationTime(win, params.presentTime());
			auto &activeTest = windowData(win).activeTest;
			if(activeTest->started)
			{
				activeTest->frameUpdate(renderer.task(), win, params);
			}
			else
			{
				activeTest->started = true;
			}
			activeTest->lastFramePresentTime.timestamp = params.timestamp();
			activeTest->lastFramePresentTime.atOnFrame = atOnFrame;
			if(activeTest->frames == framesToRun || activeTest->shouldEndTest)
			{
				finishTest(win, params.timestamp());
				return false;
			}
			else
			{
				win.postDraw();
				return true;
			}
		});
	win.setOnDraw(
		[this, &task = renderer.task()](Base::Window &win, Base::Window::DrawParams params)
		{
			auto &winData = windowData(win);
			auto xIndent = viewManager.tableXIndent();
			return task.draw(win, params, {}, winData.proj.plane().viewport(), winData.proj.matrix(),
				[rect = winData.testRectWin, &activeTest = windowData(win).activeTest, xIndent]
				(Base::Window &win, Gfx::RendererCommands &cmds)
				{
					activeTest->draw(cmds, cmds.renderer().makeClipRect(win, rect), xIndent);
					activeTest->lastFramePresentTime.atWinPresent = IG::steadyClockTimestamp();
					activeTest->presentFence = cmds.clientWaitSyncReset(activeTest->presentFence);
					cmds.present();
				});
		});
}

void FrameRateTestApplication::placeElements(const Base::Window &win)
{
	auto &winData = windowData(win);
	auto &picker = winData.picker;
	auto projP = winData.proj.plane();
	auto &activeTest = winData.activeTest;
	viewManager.setTableXIndentToDefault(win, projP);
	if(!activeTest)
	{
		picker.setViewRect(projP);
		picker.place();
	}
	else
	{
		activeTest->place(renderer, projP, winData.testRect);
	}
}

void FrameRateTestApplication::finishTest(Base::Window &win, IG::FrameTime frameTime)
{
	auto app = win.appContext();
	auto &activeTest = windowData(win).activeTest;
	if(activeTest)
	{
		activeTest->finish(renderer.task(), frameTime);
	}
	renderer.task().awaitPending();
	activeTest.reset();
	deinitCPUFreqStatus();
	deinitCPULoadStatus();
	app.setIdleDisplayPowerSave(true);
	#ifdef __ANDROID__
	if(cpuFreq)
		cpuFreq->setDefaults();
	app.setSustainedPerformanceMode(false);
	#endif
	placeElements(win);
	setPickerHandlers(win);
	win.postDraw();
}

TestFramework *FrameRateTestApplication::startTest(Base::Window &win, const TestParams &t)
{
	auto &face = viewManager.defaultFace();
	auto app = win.appContext();
	#ifdef __ANDROID__
	if(cpuFreq)
		cpuFreq->setLowLatency();
	app.setSustainedPerformanceMode(true);
	#endif
	auto &activeTest = windowData(win).activeTest;
	switch(t.test)
	{
		bcase TEST_CLEAR:
			activeTest = std::make_unique<ClearTest>();
		bcase TEST_DRAW:
			activeTest = std::make_unique<DrawTest>();
		bcase TEST_WRITE:
			activeTest = std::make_unique<WriteTest>();
	}
	activeTest->init(app, renderer, face, t.pixmapSize, t.bufferMode);
	app.setIdleDisplayPowerSave(false);
	initCPUFreqStatus();
	initCPULoadStatus();
	placeElements(win);
	auto &winData = windowData(win);
	setActiveTestHandlers(win);
	return activeTest.get();
}

namespace Base
{

const char *const ApplicationContext::applicationName{CONFIG_APP_NAME};

void ApplicationContext::onInit(ApplicationInitParams initParams)
{
	Gfx::Error err;
	initApplication<FrameRateTestApplication>(initParams, *this, err);
}

}
