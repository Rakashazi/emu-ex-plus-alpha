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

#include <imagine/gfx/GfxText.hh>
#include <imagine/gfx/Quads.hh>
#include <imagine/gfx/PixmapBufferTexture.hh>
#include <imagine/gfx/SyncFence.hh>
#include <imagine/time/Time.hh>
#include <imagine/thread/Semaphore.hh>
#include <imagine/base/ApplicationContext.hh>

namespace IG
{
struct ViewAttachParams;
}

namespace FrameRateTest
{

using namespace IG;

enum TestID
{
	TEST_CLEAR,
	TEST_DRAW,
	TEST_WRITE,
};

struct FramePresentTime
{
	SteadyClockTimePoint timestamp{};
	SteadyClockTimePoint atOnFrame{};
	SteadyClockTimePoint atWinPresent{};

	constexpr FramePresentTime() {}
};

struct TestParams
{
	TestID test{};
	WSize pixmapSize{};
	Gfx::TextureBufferMode bufferMode{};

	constexpr TestParams(TestID test)
		: test{test} {}

	constexpr TestParams(TestID test, WSize pixmapSize, Gfx::TextureBufferMode bufferMode)
		: test{test}, pixmapSize{pixmapSize}, bufferMode{bufferMode} {}
};

struct TestDesc
{
	TestParams params;
	std::string name;

	TestDesc(TestID test, std::string name, WSize pixmapSize = {},
		Gfx::TextureBufferMode bufferMode = {})
		: params{test, pixmapSize, bufferMode}, name{name} {}
};

class TestFramework
{
public:
	using TestFinishedDelegate = IG::DelegateFunc<void (TestFramework &test)>;
	bool started{};
	bool shouldEndTest{};
	unsigned frames{};
	unsigned droppedFrames{};
	unsigned continuousFrames{};
	SteadyClockTimePoint presentTime{};
	SteadyClockTimePoint startTime{}, endTime{};
	TestFinishedDelegate onTestFinished;
	FramePresentTime lastFramePresentTime;
	Gfx::IQuads statsRectQuads;

	TestFramework(ViewAttachParams);
	virtual ~TestFramework() {}
	virtual void placeTest(WRect) {}
	virtual void frameUpdateTest(Gfx::RendererTask &, Screen &, SteadyClockTimePoint) = 0;
	virtual void drawTest(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds) = 0;
	virtual void presentedTest(Gfx::RendererCommands&) {}
	void place(WRect viewBounds, WRect testRect);
	void frameUpdate(Gfx::RendererTask &rTask, IG::Window &win, IG::FrameParams frameParams);
	void prepareDraw();
	void draw(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds, int xIndent);
	void finish(SteadyClockTimePoint);
	void setCPUFreqText(std::string_view str);
	void setCPUUseText(std::string_view str);

protected:
	Gfx::Text cpuStatsText;
	Gfx::Text frameStatsText;
	std::string cpuFreqStr;
	std::string cpuUseStr;
	std::string skippedFrameStr;
	std::string statsStr;
	WRect viewBounds{};
	WRect cpuStatsRect{};
	WRect frameStatsRect{};
	unsigned lostFrameProcessTime{};

	void placeCPUStatsText();
	void placeFrameStatsText();
};

class ClearTest : public TestFramework
{
protected:
	bool flash{true};

public:
	using TestFramework::TestFramework;
	void frameUpdateTest(Gfx::RendererTask &, Screen &, SteadyClockTimePoint) override;
	void drawTest(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds) override;
};

class DrawTest : public TestFramework
{
protected:
	int flash{true};
	Gfx::ITexQuads quad;
	Gfx::PixmapBufferTexture texture;

public:
	DrawTest(IG::ApplicationContext, ViewAttachParams attach, WSize pixmapSize, Gfx::TextureBufferMode);
	void placeTest(WRect testRect) override;
	void frameUpdateTest(Gfx::RendererTask &, Screen &, SteadyClockTimePoint) override;
	void drawTest(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds) override;
};

class WriteTest : public DrawTest
{
public:
	using DrawTest::DrawTest;
	void frameUpdateTest(Gfx::RendererTask &, Screen &, SteadyClockTimePoint) override;
	void drawTest(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds) override;
};

const char *testIDToStr(TestID id);

}
