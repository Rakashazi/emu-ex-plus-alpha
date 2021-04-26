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
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/util/algorithm.h>
#include <imagine/util/string.h>
#include <imagine/base/Window.hh>
#include <imagine/base/Screen.hh>
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

void TestFramework::init(Base::ApplicationContext app, Gfx::Renderer &r,
	Gfx::GlyphTextureSet &face, IG::WP pixmapSize, Gfx::TextureBufferMode bufferMode)
{
	cpuStatsText = {&face};
	frameStatsText = {&face};
	initTest(app, r, pixmapSize, bufferMode);
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
	if(cpuStatsText.compile(r, projP))
	{
		cpuStatsRect = projP.bounds();
		cpuStatsRect.y = (cpuStatsRect.y2 - cpuStatsText.nominalHeight() * cpuStatsText.currentLines())
			- cpuStatsText.nominalHeight() * .5f; // adjust to top
	}
}

void TestFramework::placeFrameStatsText(Gfx::Renderer &r)
{
	if(frameStatsText.compile(r, projP))
	{
		frameStatsRect = projP.bounds();
		frameStatsRect.y2 = (frameStatsRect.y + frameStatsText.nominalHeight() * frameStatsText.currentLines())
			+ cpuStatsText.nominalHeight() * .5f; // adjust to bottom
	}
}

void TestFramework::place(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP, const Gfx::GCRect &testRect)
{
	this->projP = projP;
	frameStatsText.setMaxLineSize(projP.bounds().xSize());
	placeCPUStatsText(r);
	placeFrameStatsText(r);
	placeTest(testRect);
}

void TestFramework::frameUpdate(Gfx::RendererTask &rTask, Base::Window &win, Base::FrameParams frameParams)
{
	auto timestamp = frameParams.timestamp();
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
			cpuStatsText.setString(
				string_makePrintf<256>("%s%s%s",
				strlen(cpuUseStr.data()) ? cpuUseStr.data() : "",
				strlen(cpuUseStr.data()) && strlen(cpuFreqStr.data()) ? "\n" : "",
				strlen(cpuFreqStr.data()) ? cpuFreqStr.data() : "").data()
			);
			placeCPUStatsText(rTask.renderer());
		}

		// frame stats
		bool updatedFrameStats = false;
		if(!startTime.count())
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
				lostFrameProcessTime = std::chrono::duration_cast<IG::Milliseconds>(lastFramePresentTime.atWinPresent - lastFramePresentTime.atOnFrame).count();

				droppedFrames++;
				string_printf(skippedFrameStr, "Lost %u frame(s) taking %.3fs after %u continuous\nat time %.3fs",
					elapsedScreenFrames - 1, IG::FloatSeconds(timestamp - lastFramePresentTime.timestamp).count(),
					continuousFrames, IG::FloatSeconds(timestamp).count());
				updatedFrameStats = true;
				continuousFrames = 0;
			}
		}
		if(frames && frames % 4 == 0)
		{
			string_printf(statsStr, "Total Draw Time: %02lums (%02ums)\nTimestamp Diff: %02lums",
				(unsigned long)std::chrono::duration_cast<IG::Milliseconds>(lastFramePresentTime.atWinPresent - lastFramePresentTime.atOnFrame).count(),
				lostFrameProcessTime,
				(unsigned long)std::chrono::duration_cast<IG::Milliseconds>(timestamp - lastFramePresentTime.timestamp).count());
			updatedFrameStats = true;
		}
		if(updatedFrameStats)
		{
			frameStatsText.setString(
				string_makePrintf<512>("%s%s%s",
				strlen(skippedFrameStr.data()) ? skippedFrameStr.data() : "",
				strlen(skippedFrameStr.data()) && strlen(statsStr.data()) ? "\n" : "",
				strlen(statsStr.data()) ? statsStr.data() : "").data()
			);
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

void TestFramework::draw(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds, Gfx::GC xIndent)
{
	using namespace Gfx;
	cmds.loadTransform(projP.makeTranslate());
	drawTest(cmds, bounds);
	cmds.setClipTest(false);
	if(cpuStatsText.isVisible())
	{
		cmds.setCommonProgram(CommonProgram::NO_TEX);
		cmds.setBlendMode(BLEND_MODE_ALPHA);
		cmds.setColor(0., 0., 0., .7);
		GeomRect::draw(cmds, cpuStatsRect);
		cmds.setColor(1., 1., 1., 1.);
		cmds.setCommonProgram(CommonProgram::TEX_ALPHA);
		cpuStatsText.draw(cmds, projP.alignXToPixel(cpuStatsRect.x + xIndent),
			projP.alignYToPixel(cpuStatsRect.yCenter()), LC2DO, projP);
	}
	if(frameStatsText.isVisible())
	{
		cmds.setCommonProgram(CommonProgram::NO_TEX);
		cmds.setBlendMode(BLEND_MODE_ALPHA);
		cmds.setColor(0., 0., 0., .7);
		GeomRect::draw(cmds, frameStatsRect);
		cmds.setColor(1., 1., 1., 1.);
		cmds.setCommonProgram(CommonProgram::TEX_ALPHA);
		frameStatsText.draw(cmds, projP.alignXToPixel(frameStatsRect.x + xIndent),
			projP.alignYToPixel(frameStatsRect.yCenter()), LC2DO, projP);
	}
}

void TestFramework::finish(Gfx::RendererTask &task, IG::FrameTime frameTime)
{
	endTime = frameTime;
	task.deleteSyncFence(presentFence);
	presentFence = {};
	if(onTestFinished)
		onTestFinished(*this);
}

void ClearTest::frameUpdateTest(Gfx::RendererTask &, Base::Screen &, IG::FrameTime)
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
		cmds.clear();
		cmds.setClearColor(0, 0, 0);
	}
	else
	{
		cmds.clear();
	}
}

void DrawTest::initTest(Base::ApplicationContext app, Gfx::Renderer &r, IG::WP pixmapSize, Gfx::TextureBufferMode bufferMode)
{
	IG::PixmapDesc pixmapDesc = {pixmapSize, IG::PIXEL_FMT_RGB565};
	Gfx::TextureConfig texConf{pixmapDesc};
	texConf.setCompatSampler(&r.make(Gfx::CommonTextureSampler::NO_MIP_CLAMP));
	const bool canSingleBuffer = r.maxSwapChainImages() < 3 || r.supportsSyncFences();
	texture = r.makePixmapBufferTexture(texConf, bufferMode, canSingleBuffer);
	if(!texture)
	{
		app.exitWithErrorMessagePrintf(-1, "Can't init test texture");
		return;
	}
	auto lockedBuff = texture.lock();
	assert(lockedBuff);
	memset(lockedBuff.pixmap().data(), 0xFF, lockedBuff.pixmap().bytes());
	texture.unlock(lockedBuff);
	texture.compileDefaultProgram(Gfx::IMG_MODE_REPLACE);
	texture.compileDefaultProgram(Gfx::IMG_MODE_MODULATE);
	sprite = {{}, texture};
}

void DrawTest::placeTest(const Gfx::GCRect &rect)
{
	sprite.setPos(rect);
}

void DrawTest::frameUpdateTest(Gfx::RendererTask &, Base::Screen &, IG::FrameTime)
{
	flash ^= true;
}

void DrawTest::drawTest(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds)
{
	cmds.clear();
	cmds.setClipTest(true);
	cmds.setClipRect(bounds);
	cmds.setBlendMode(Gfx::BLEND_MODE_OFF);
	cmds.set(Gfx::CommonTextureSampler::NO_MIP_CLAMP);
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

void WriteTest::frameUpdateTest(Gfx::RendererTask &rendererTask, Base::Screen &screen, IG::FrameTime frameTime)
{
	DrawTest::frameUpdateTest(rendererTask, screen, frameTime);
	rendererTask.clientWaitSync(std::exchange(presentFence, {}));
	auto lockedBuff = texture.lock();
	IG::Pixmap pix = lockedBuff.pixmap();
	if(flash)
	{
		uint16_t writeColor;
		if(!droppedFrames)
			writeColor = IG::PIXEL_DESC_RGB565.build(.7, .7, .7, 1.);
		else if(droppedFrames % 2 == 0)
			writeColor = IG::PIXEL_DESC_RGB565.build(.7, .7, .0, 1.);
		else
			writeColor = IG::PIXEL_DESC_RGB565.build(.7, .0, .0, 1.);
		iterateTimes(pix.w() * pix.h(), i)
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
	cmds.clear();
	cmds.setClipTest(true);
	cmds.setClipRect(bounds);
	cmds.setBlendMode(Gfx::BLEND_MODE_OFF);
	cmds.set(Gfx::CommonTextureSampler::NO_MIP_CLAMP);
	sprite.setCommonProgram(cmds, Gfx::IMG_MODE_REPLACE);
	sprite.draw(cmds);
}

WriteTest::~WriteTest() {}
