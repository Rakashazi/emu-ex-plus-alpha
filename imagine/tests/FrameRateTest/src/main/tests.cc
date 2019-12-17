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

#define LOGTAG "test"
#include <imagine/gui/TableView.hh>
#include <imagine/util/algorithm.h>
#include <imagine/logger/logger.h>
#include "tests.hh"
#include "cpuUtils.hh"

const char *testIDToStr(TestID id)
{
	switch(id)
	{
		case TEST_CLEAR: return "Clear";
		case TEST_DRAW: return "Draw";
		case TEST_WRITE: return "Write";
		default: return "Unknown";
	}
}

std::array<char, 64> TestParams::makeTestName() const
{
	std::array<char, 64> name;
	if(pixmapSize.x)
		string_printf(name, "%s RGB565 %ux%u", testIDToStr(test), pixmapSize.x, pixmapSize.y);
	else
		string_printf(name, "%s", testIDToStr(test));
	return name;
}

void TestFramework::init(Gfx::Renderer &r, IG::Point2D<int> pixmapSize)
{
	cpuStatsText = {cpuStatsStr.data(), &View::defaultFace};
	frameStatsText = {frameStatsStr.data(), &View::defaultFace};
	initTest(r, pixmapSize);
}

void TestFramework::deinit()
{
	deinitTest();
}

void TestFramework::setCPUFreqText(const char *str)
{
	string_printf(cpuFreqStr, "CPU Frequency: %s", str);
}

void TestFramework::setCPUUseText(const char *str)
{
	string_printf(cpuUseStr, "CPU Load (System): %s", str);
}

void TestFramework::placeCPUStatsText(Gfx::Renderer &r)
{
	if(strlen(cpuStatsStr.data()))
	{
		cpuStatsText.compile(r, projP);
		cpuStatsRect = projP.bounds();
		cpuStatsRect.y = (cpuStatsRect.y2 - cpuStatsText.nominalHeight * cpuStatsText.lines)
			- cpuStatsText.nominalHeight * .5f; // adjust to top
	}
}

void TestFramework::placeFrameStatsText(Gfx::Renderer &r)
{
	if(strlen(frameStatsStr.data()))
	{
		frameStatsText.compile(r, projP);
		frameStatsRect = projP.bounds();
		frameStatsRect.y2 = (frameStatsRect.y + frameStatsText.nominalHeight * frameStatsText.lines)
			+ cpuStatsText.nominalHeight * .5f; // adjust to bottom
	}
}

void TestFramework::place(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP, const Gfx::GCRect &testRect)
{
	this->projP = projP;
	frameStatsText.maxLineSize = projP.bounds().xSize();
	placeCPUStatsText(r);
	placeFrameStatsText(r);
	placeTest(testRect);
}

void TestFramework::frameUpdate(Gfx::RendererTask &rTask, Base::Window &win, Base::FrameTimeBase timestamp)
{
	// CPU stats
	auto &screen = *win.screen();
	bool updatedCPUStats = false;
	if(frames % 8 == 0)
	{
		updateCPUFreq(*this);
		updatedCPUStats = true;
	}
	if(frames % 120 == 0)
	{
		updateCPULoad(*this);
		updatedCPUStats = true;
	}
	{
		if(updatedCPUStats)
		{
			string_printf(cpuStatsStr, "%s%s%s",
				strlen(cpuUseStr.data()) ? cpuUseStr.data() : "",
				strlen(cpuUseStr.data()) && strlen(cpuFreqStr.data()) ? "\n" : "",
				strlen(cpuFreqStr.data()) ? cpuFreqStr.data() : "");
			placeCPUStatsText(rTask.renderer());
		}

		// frame stats
		bool updatedFrameStats = false;
		if(!frames)
		{
			startTime = timestamp;
			//logMsg("start time: %llu", (unsigned long long)startTime);
		}
		else
		{
			auto elapsedScreenFrames = screen.elapsedFrames(timestamp);
			//logMsg("elapsed: %d", screen.elapsedFrames(frameTime));
			if(elapsedScreenFrames > 1)
			{
				lostFrameProcessTime = (lastFramePresentTime.atWinPresent - lastFramePresentTime.atOnFrame).mSecs();
				lostFramePresentTime = (lastFramePresentTime.atWinPresentEnd - lastFramePresentTime.atWinPresent).mSecs();

				droppedFrames++;
				string_printf(skippedFrameStr, "Lost %u frame(s) taking %.3fs after %u continuous\nat time %.3fs",
					elapsedScreenFrames - 1, Base::frameTimeBaseToSecsDec(timestamp - screen.lastFrameTimestamp()),
					continuousFrames, Base::frameTimeBaseToSecsDec(timestamp));
				updatedFrameStats = true;
				continuousFrames = 0;
			}
		}
		if(frames && frames % 4 == 0)
		{
			string_printf(statsStr, "Process: %02ums (%02ums)\nPresent: %02ums (%02ums)",
				(uint)(lastFramePresentTime.atWinPresent - lastFramePresentTime.atOnFrame).mSecs(), lostFrameProcessTime,
				(uint)(lastFramePresentTime.atWinPresentEnd - lastFramePresentTime.atWinPresent).mSecs(), lostFramePresentTime);
			updatedFrameStats = true;
		}
		if(updatedFrameStats)
		{
			string_printf(frameStatsStr, "%s%s%s",
				strlen(skippedFrameStr.data()) ? skippedFrameStr.data() : "",
				strlen(skippedFrameStr.data()) && strlen(statsStr.data()) ? "\n" : "",
				strlen(statsStr.data()) ? statsStr.data() : "");
			placeFrameStatsText(rTask.renderer());
		}
	}
	// run frame
	frameUpdateTest(rTask, *win.screen(), timestamp);
	frames++;
	continuousFrames++;
}

void TestFramework::prepareDraw(Gfx::Renderer &r)
{
	cpuStatsText.makeGlyphs(r);
	frameStatsText.makeGlyphs(r);
}

void TestFramework::draw(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds)
{
	using namespace Gfx;
	cmds.loadTransform(projP.makeTranslate());
	drawTest(cmds, bounds);
	cmds.setClipTest(false);
	if(strlen(cpuStatsStr.data()))
	{
		cmds.setCommonProgram(CommonProgram::NO_TEX);
		cmds.setBlendMode(BLEND_MODE_ALPHA);
		cmds.setColor(0., 0., 0., .7);
		GeomRect::draw(cmds, cpuStatsRect);
		cmds.setColor(1., 1., 1., 1.);
		cmds.setCommonProgram(CommonProgram::TEX_ALPHA);
		cpuStatsText.draw(cmds, projP.alignXToPixel(cpuStatsRect.x + TableView::globalXIndent),
			projP.alignYToPixel(cpuStatsRect.yCenter()), LC2DO, projP);
	}
	if(strlen(frameStatsStr.data()))
	{
		cmds.setCommonProgram(CommonProgram::NO_TEX);
		cmds.setBlendMode(BLEND_MODE_ALPHA);
		cmds.setColor(0., 0., 0., .7);
		GeomRect::draw(cmds, frameStatsRect);
		cmds.setColor(1., 1., 1., 1.);
		cmds.setCommonProgram(CommonProgram::TEX_ALPHA);
		frameStatsText.draw(cmds, projP.alignXToPixel(frameStatsRect.x + TableView::globalXIndent),
			projP.alignYToPixel(frameStatsRect.yCenter()), LC2DO, projP);
	}
}

void TestFramework::finish(Base::FrameTimeBase frameTime)
{
	endTime = frameTime;
	if(onTestFinished)
		onTestFinished(*this);
}

void ClearTest::frameUpdateTest(Gfx::RendererTask &, Base::Screen &, Base::FrameTimeBase)
{
	flash ^= true;
}

void ClearTest::drawTest(Gfx::RendererCommands &cmds, Gfx::ClipRect)
{
	if(flash)
	{
		if(!droppedFrames)
			cmds.setClearColor(.7, .7, .7);
		else if(droppedFrames % 2 == 0)
			cmds.setClearColor(.7, .7, .0);
		else
			cmds.setClearColor(.7, .0, .0);
	}
	else
	{
		cmds.setClearColor(0, 0, 0);
	}
	cmds.clear();
}

void DrawTest::initTest(Gfx::Renderer &r, IG::WP pixmapSize)
{
	pixmap = {{pixmapSize, IG::PIXEL_FMT_RGB565}};
	memset(pixmap.pixel({}), 0xFF, pixmap.pixelBytes());
	Gfx::TextureConfig texConf{pixmap};
	texConf.setWillWriteOften(true);
	texture = r.makePixmapTexture(texConf);
	if(!texture)
	{
		Base::exitWithErrorMessagePrintf(-1, "Can't init test texture");
		return;
	}
	texture.write(0, pixmap, {});
	texture.compileDefaultProgram(Gfx::IMG_MODE_REPLACE);
	texture.compileDefaultProgram(Gfx::IMG_MODE_MODULATE);
	sprite.init({}, texture);
	Gfx::TextureSampler::initDefaultNoMipClampSampler(r);
}

void DrawTest::placeTest(const Gfx::GCRect &rect)
{
	sprite.setPos(rect);
}

void DrawTest::deinitTest()
{
	sprite.deinit();
	texture.deinit();
}

void DrawTest::frameUpdateTest(Gfx::RendererTask &, Base::Screen &, Base::FrameTimeBase)
{
	flash ^= true;
}

void DrawTest::drawTest(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds)
{
	cmds.setClearColor(0, 0, 0);
	cmds.clear();
	cmds.setClipTest(true);
	cmds.setClipRect(bounds);
	cmds.setBlendMode(Gfx::BLEND_MODE_OFF);
	cmds.setCommonTextureSampler(Gfx::CommonTextureSampler::NO_MIP_CLAMP);
	sprite.setCommonProgram(cmds, Gfx::IMG_MODE_MODULATE);
	if(flash)
	{
		if(!droppedFrames)
			cmds.setColor(.7, .7, .7, 1.);
		else if(droppedFrames % 2 == 0)
			cmds.setColor(.7, .7, .0, 1.);
		else
			cmds.setColor(.7, .0, .0, 1.);
	}
	else
		cmds.setColor(0., 0., 0., 1.);
	sprite.draw(cmds);
}

void WriteTest::frameUpdateTest(Gfx::RendererTask &rendererTask, Base::Screen &screen, Base::FrameTimeBase frameTime)
{
	DrawTest::frameUpdateTest(rendererTask, screen, frameTime);
	auto lockedBuff = texture.lock(0);
	IG::Pixmap pix;
	if(lockedBuff)
	{
		pix = lockedBuff.pixmap();
	}
	else
	{
		pix = pixmap;
	}
	if(flash)
	{
		uint writeColor;
		if(!droppedFrames)
			writeColor = IG::PIXEL_DESC_RGB565.build(.7, .7, .7, 1.);
		else if(droppedFrames % 2 == 0)
			writeColor = IG::PIXEL_DESC_RGB565.build(.7, .7, .0, 1.);
		else
			writeColor = IG::PIXEL_DESC_RGB565.build(.7, .0, .0, 1.);
		iterateTimes(pix.w() * pix.h(), i)
		{
			((uint16*)pix.pixel({}))[i] = writeColor;
		}
	}
	else
	{
		memset(pix.pixel({}), 0, pix.pitchBytes() * pix.h());
	}
	rendererTask.acquireFenceAndWait(fence);
	if(lockedBuff)
	{
		texture.unlock(lockedBuff);
	}
	else
	{
		texture.write(0, pix, {});
	}
}

void WriteTest::drawTest(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds)
{
	cmds.setClearColor(0, 0, 0);
	cmds.clear();
	cmds.setClipTest(true);
	cmds.setClipRect(bounds);
	cmds.setBlendMode(Gfx::BLEND_MODE_OFF);
	cmds.setCommonTextureSampler(Gfx::CommonTextureSampler::NO_MIP_CLAMP);
	sprite.setCommonProgram(cmds, Gfx::IMG_MODE_REPLACE);
	sprite.draw(cmds);
	fence = cmds.replaceSyncFence(fence);
}

void WriteTest::deinitTest()
{
	DrawTest::deinitTest();
	texture.renderer().deleteSyncFence(fence);
}
