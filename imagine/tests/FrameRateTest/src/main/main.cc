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
#include <imagine/io/FileIO.hh>
#include "tests.hh"
#include "TestPicker.hh"
#include <unistd.h>

static const uint framesToRun = 60*60;
static Base::Window mainWin;
static Gfx::ProjectionPlane projP;
static Gfx::Mat4 projMat;
static Gfx::GCRect testRect;
static FileIO cpuFreqFile;
static TestFramework *activeTest{};
static TestPicker picker{mainWin};

static TestParams testParam[] =
{
	{TEST_CLEAR},
	{TEST_DRAW, {320, 224}},
	{TEST_WRITE, {320, 224}},
};

static void updateCPUFreq()
{
	if(!activeTest || !cpuFreqFile)
		return;
	char buff[32]{};
	cpuFreqFile.readAtPos(buff, sizeof(buff)-1, 0);
	//logMsg("read CPU freq: %s", buff);
	activeTest->setCPUFreqText(buff);
}

static void setupCPUFreqStatus()
{
	if(!Config::envIsLinux && !Config::envIsAndroid)
		return; // ignore cpufreq monitoring on non-linux systems
	const char *cpuFreqPath = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq";
	if(cpuFreqFile.open(cpuFreqPath) != OK)
	{
		logWarn("can't open %s", cpuFreqPath);
	}
}

static void placeElements()
{
	GuiTable1D::setDefaultXIndent(projP);
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

static void finishTest(Base::Window &win, Base::FrameTimeBase frameTime)
{
	cpuFreqFile.close();
	if(activeTest)
	{
		activeTest->finish(frameTime);
	}
	delete activeTest;
	activeTest = nullptr;
	Gfx::setClearColor(0, 0, 0);
	placeElements();
	win.postDraw();
	Base::setIdleDisplayPowerSave(true);
	Input::setKeyRepeat(true);
}

TestFramework *startTest(Base::Window &win, const TestParams &t)
{
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
	setupCPUFreqStatus();
	placeElements();

	win.screen()->addOnFrame(
		[&win](Base::Screen &screen, Base::Screen::FrameParams params)
		{
			auto frameTime = params.frameTime();

			if(activeTest->frames % 8 == 0)
			{
				updateCPUFreq();
			}

			activeTest->frameUpdate(screen, frameTime);

			if(activeTest->frames == framesToRun || activeTest->shouldEndTest)
			{
				finishTest(win, frameTime);
			}
			else
			{
				win.postDraw();
				screen.addOnFrame(params.thisOnFrame());
			}
		});
	return activeTest;
}

namespace Base
{

CallResult onInit(int argc, char** argv)
{
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
			Gfx::presentWindow(win);
		});

	winConf.setOnInputEvent(
		[](Base::Window &, const Input::Event &e)
		{
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
	picker.init(testParam, sizeofArray(testParam));
	mainWin.show();
	return OK;
}

}
