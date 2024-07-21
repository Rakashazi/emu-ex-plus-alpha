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
#include <imagine/gui/ViewManager.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/util/algorithm.h>
#include <imagine/util/format.hh>
#include <imagine/util/string/StaticString.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Screen.hh>
#include <imagine/logger/logger.h>
#include "tests.hh"
#include "cpuUtils.hh"

namespace FrameRateTest
{

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

TestFramework::TestFramework(ViewAttachParams attach):
	statsRectQuads{attach.rendererTask, {.size = 2}},
	cpuStatsText{attach.rendererTask, &attach.viewManager.defaultFace},
	frameStatsText{attach.rendererTask, &attach.viewManager.defaultFace} {}

void TestFramework::setCPUFreqText(std::string_view str)
{
	cpuFreqStr = "CPU Frequency: ";
	cpuFreqStr += str;
}

void TestFramework::setCPUUseText(std::string_view str)
{
	cpuUseStr = "CPU Load (System): ";
	cpuUseStr += str;
}

void TestFramework::placeCPUStatsText()
{
	if(cpuStatsText.compile())
	{
		cpuStatsRect = viewBounds;
		cpuStatsRect.y2 = (cpuStatsRect.y + cpuStatsText.nominalHeight() * cpuStatsText.currentLines())
			+ cpuStatsText.nominalHeight() / 2; // adjust to top
		statsRectQuads.write(0, {.bounds = cpuStatsRect.as<int16_t>()});
	}
}

void TestFramework::placeFrameStatsText()
{
	if(frameStatsText.compile({.maxLineSize = viewBounds.xSize()}))
	{
		frameStatsRect = viewBounds;
		frameStatsRect.y = (frameStatsRect.y2 - frameStatsText.nominalHeight() * frameStatsText.currentLines())
			- cpuStatsText.nominalHeight() / 2; // adjust to bottom
		statsRectQuads.write(1, {.bounds = frameStatsRect.as<int16_t>()});
	}
}

void TestFramework::place(WRect viewBounds_, WRect testRect)
{
	viewBounds = viewBounds_;
	placeCPUStatsText();
	placeFrameStatsText();
	placeTest(testRect);
}

void TestFramework::frameUpdate(Gfx::RendererTask &rTask, IG::Window &win, IG::FrameParams frameParams)
{
	auto timestamp = frameParams.timestamp;
	// CPU stats
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
			IG::StaticString<512> str{cpuUseStr};
			if(cpuUseStr.size() && cpuFreqStr.size())
				str += '\n';
			str += cpuFreqStr;
			cpuStatsText.resetString(str);
			placeCPUStatsText();
		}

		// frame stats
		bool updatedFrameStats = false;
		if(!hasTime(startTime))
		{
			startTime = timestamp;
			//logMsg("start time: %llu", (unsigned long long)startTime);
		}
		else
		{
			auto elapsedScreenFrames = frameParams.elapsedFrames(lastFramePresentTime.timestamp);
			//logMsg("elapsed: %d", screen.elapsedFrames(frameTime));
			if(elapsedScreenFrames > 1)
			{
				lostFrameProcessTime = duration_cast<Milliseconds>(lastFramePresentTime.atWinPresent - lastFramePresentTime.atOnFrame).count();

				droppedFrames++;
				skippedFrameStr.clear();
				IG::formatTo(skippedFrameStr, "Lost {} frame(s) taking {:.3f}s after {} continuous\nat time {:.3f}s",
					elapsedScreenFrames - 1, IG::FloatSeconds(timestamp - lastFramePresentTime.timestamp).count(),
					continuousFrames, IG::FloatSeconds(timestamp.time_since_epoch()).count());
				updatedFrameStats = true;
				continuousFrames = 0;
			}
		}
		if(frames && frames % 4 == 0)
		{
			statsStr.clear();
			IG::formatTo(statsStr, "Total Draw Time: {:02}ms ({:02}ms)\nTimestamp Diff: {:02}ms",
				(unsigned long)duration_cast<Milliseconds>(lastFramePresentTime.atWinPresent - lastFramePresentTime.atOnFrame).count(),
				lostFrameProcessTime,
				(unsigned long)duration_cast<Milliseconds>(timestamp - lastFramePresentTime.timestamp).count());
			updatedFrameStats = true;
		}
		if(updatedFrameStats)
		{
			IG::StaticString<512> str{skippedFrameStr};
			if(skippedFrameStr.size() && statsStr.size())
				str += '\n';
			str += statsStr;
			frameStatsText.resetString(str);
			placeFrameStatsText();
		}
	}
	// run frame
	frameUpdateTest(rTask, *win.screen(), timestamp);
	frames++;
	continuousFrames++;
}

void TestFramework::prepareDraw()
{
	cpuStatsText.makeGlyphs();
	frameStatsText.makeGlyphs();
}

void TestFramework::draw(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds, int xIndent)
{
	using namespace IG::Gfx;
	drawTest(cmds, bounds);
	cmds.setClipTest(false);
	auto &basicEffect = cmds.basicEffect();
	if(cpuStatsText.isVisible())
	{
		basicEffect.disableTexture(cmds);
		cmds.set(BlendMode::ALPHA);
		cmds.setColor({0., 0., 0., .7});
		cmds.drawQuad(statsRectQuads, 0);
		basicEffect.enableAlphaTexture(cmds);
		cpuStatsText.draw(cmds, {cpuStatsRect.x + xIndent,
			cpuStatsRect.yCenter()}, LC2DO, ColorName::WHITE);
	}
	if(frameStatsText.isVisible())
	{
		basicEffect.disableTexture(cmds);
		cmds.set(BlendMode::ALPHA);
		cmds.setColor({0., 0., 0., .7});
		cmds.drawQuad(statsRectQuads, 1);
		basicEffect.enableAlphaTexture(cmds);
		frameStatsText.draw(cmds, {frameStatsRect.x + xIndent,
			frameStatsRect.yCenter()}, LC2DO, ColorName::WHITE);
	}
}

void TestFramework::finish(SteadyClockTimePoint frameTime)
{
	endTime = frameTime;
	if(onTestFinished)
		onTestFinished(*this);
}

void ClearTest::frameUpdateTest(Gfx::RendererTask &, Screen &, SteadyClockTimePoint)
{
	flash ^= true;
}

void ClearTest::drawTest(Gfx::RendererCommands &cmds, Gfx::ClipRect)
{
	if(flash)
	{
		if(!droppedFrames)
			cmds.setClearColor({.7, .7, .7});
		else if(droppedFrames % 2 == 0)
			cmds.setClearColor({.7, .7, .0});
		else
			cmds.setClearColor({.7, .0, .0});
		cmds.clear();
		cmds.setClearColor(0);
	}
	else
	{
		cmds.clear();
	}
}

DrawTest::DrawTest(IG::ApplicationContext ctx, ViewAttachParams attach, WSize pixmapSize, Gfx::TextureBufferMode bufferMode):
	TestFramework{attach},
	quad{attach.rendererTask, {.size = 1}}
{
	using namespace IG::Gfx;
	auto &r = attach.renderer();
	PixmapDesc pixmapDesc = {pixmapSize, PixelFmtRGB565};
	TextureConfig texConf{pixmapDesc, SamplerConfigs::noMipClamp};
	texture = r.makePixmapBufferTexture(texConf, bufferMode);
	if(!texture) [[unlikely]]
	{
		ctx.exitWithMessage(-1, "Can't init test texture");
		return;
	}
	auto lockedBuff = texture.lock();
	assert(lockedBuff);
	memset(lockedBuff.pixmap().data(), 0xFF, lockedBuff.pixmap().bytes());
	texture.unlock(lockedBuff);
}

void DrawTest::placeTest(WRect rect)
{
	quad.write(0, {.bounds = rect.as<int16_t>()});
}

void DrawTest::frameUpdateTest(Gfx::RendererTask &, Screen &, SteadyClockTimePoint)
{
	flash ^= true;
}

void DrawTest::drawTest(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds)
{
	using namespace IG::Gfx;
	cmds.clear();
	cmds.setClipTest(true);
	cmds.setClipRect(bounds);
	cmds.set(BlendMode::OFF);
	if(flash)
	{
		if(!droppedFrames)
			cmds.setColor({.7, .7, .7});
		else if(droppedFrames % 2 == 0)
			cmds.setColor({.7, .7, .0});
		else
			cmds.setColor({.7, .0, .0});
	}
	else
		cmds.setColor(0);
	cmds.basicEffect().drawSprite(cmds, quad, 0, texture);
}

void WriteTest::frameUpdateTest(Gfx::RendererTask &rendererTask, Screen &screen, SteadyClockTimePoint frameTime)
{
	DrawTest::frameUpdateTest(rendererTask, screen, frameTime);
	auto lockedBuff = texture.lock();
	auto pix = lockedBuff.pixmap();
	if(flash)
	{
		uint16_t writeColor;
		if(!droppedFrames)
			writeColor = IG::PixelDescRGB565.build(.7, .7, .7, 1.);
		else if(droppedFrames % 2 == 0)
			writeColor = IG::PixelDescRGB565.build(.7, .7, .0, 1.);
		else
			writeColor = IG::PixelDescRGB565.build(.7, .0, .0, 1.);
		for(auto i : iotaCount(pix.w() * pix.h()))
		{
			((uint16_t*)pix.data())[i] = writeColor;
		}
	}
	else
	{
		memset(pix.data(), 0, pix.pitchBytes() * pix.h());
	}
	texture.unlock(lockedBuff);
}

void WriteTest::drawTest(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds)
{
	using namespace IG::Gfx;
	cmds.clear();
	cmds.setClipTest(true);
	cmds.setClipRect(bounds);
	cmds.set(BlendMode::OFF);
	cmds.setColor(ColorName::WHITE);
	cmds.basicEffect().drawSprite(cmds, quad, 0, texture);
}

}
