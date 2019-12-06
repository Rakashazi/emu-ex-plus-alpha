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
#include <imagine/gfx/GfxSprite.hh>
#include <imagine/gfx/ProjectionPlane.hh>
#include <imagine/time/Time.hh>

enum TestID
{
	TEST_CLEAR,
	TEST_DRAW,
	TEST_WRITE,
};

struct FramePresentTime
{
	IG::Time atOnFrame;
	IG::Time atWinPresent;
	IG::Time atWinPresentEnd;

	constexpr FramePresentTime() {}
};

class TestParams
{
public:
	TestID test;
	IG::Point2D<int> pixmapSize;

	constexpr TestParams(TestID test)
		: test{test} {}

	constexpr TestParams(TestID test, IG::Point2D<int> pixmapSize)
		: test{test}, pixmapSize{pixmapSize} {}

	std::array<char, 64> makeTestName() const;
};

class TestFramework
{
public:
	using TestFinishedDelegate = DelegateFunc<void (TestFramework &test)>;
	bool started = false;
	bool shouldEndTest{};
	uint frames{};
	uint droppedFrames{};
	uint continuousFrames{};
	Base::FrameTimeBase startTime{}, endTime{};
	TestFinishedDelegate onTestFinished;
	FramePresentTime lastFramePresentTime;

	constexpr TestFramework() {}
	virtual ~TestFramework() {}
	virtual void initTest(Gfx::Renderer &r, IG::Point2D<int> pixmapSize) {}
	virtual void placeTest(const Gfx::GCRect &testRect) {}
	virtual void frameUpdateTest(Gfx::RendererTask &rendererTask, Base::Screen &screen, Base::FrameTimeBase frameTime) = 0;
	virtual void deinitTest() {}
	virtual void drawTest(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds) = 0;
	void init(Gfx::Renderer &r, IG::Point2D<int> pixmapSize);
	void deinit();
	void place(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP, const Gfx::GCRect &testRect);
	void frameUpdate(Gfx::RendererTask &rTask, Base::Window &win, Base::FrameTimeBase timestamp);
	void prepareDraw(Gfx::Renderer &r);
	void draw(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds);
	void finish(Base::FrameTimeBase frameTime);
	void setCPUFreqText(const char *str);
	void setCPUUseText(const char *str);

protected:
	Gfx::Text cpuStatsText;
	Gfx::GCRect cpuStatsRect{};
	std::array<char, 256> cpuFreqStr{};
	std::array<char, 64> cpuUseStr{};
	std::array<char, 256> cpuStatsStr{};
	Gfx::Text frameStatsText;
	Gfx::GCRect frameStatsRect{};
	std::array<char, 256> skippedFrameStr{};
	std::array<char, 256> statsStr{};
	std::array<char, 512> frameStatsStr{};
	Gfx::ProjectionPlane projP;
	uint lostFrameDispatchTime = 0;
	uint lostFrameProcessTime = 0;
	uint lostFramePresentTime = 0;

	void placeCPUStatsText(Gfx::Renderer &r);
	void placeFrameStatsText(Gfx::Renderer &r);
};

class ClearTest : public TestFramework
{
protected:
	bool flash{true};

public:
	ClearTest() {}

	void frameUpdateTest(Gfx::RendererTask &rendererTask, Base::Screen &screen, Base::FrameTimeBase frameTime) override;
	void drawTest(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds) override;
};

class DrawTest : public TestFramework
{
protected:
	int flash{true};
	IG::MemPixmap pixmap;
	Gfx::PixmapTexture texture;
	Gfx::Sprite sprite;

public:
	DrawTest() {}

	void initTest(Gfx::Renderer &r, IG::WP pixmapSize) override;
	void placeTest(const Gfx::GCRect &rect) override;
	void deinitTest() override;
	void frameUpdateTest(Gfx::RendererTask &rendererTask, Base::Screen &screen, Base::FrameTimeBase frameTime) override;
	void drawTest(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds) override;
};

class WriteTest : public DrawTest
{
protected:
	Gfx::SyncFence fence{};

public:
	WriteTest() {}

	void frameUpdateTest(Gfx::RendererTask &rendererTask, Base::Screen &screen, Base::FrameTimeBase frameTime) override;
	void drawTest(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds) override;
	void deinitTest() override;
};

TestFramework *startTest(Base::Window &win, Gfx::Renderer &r, const TestParams &t);
const char *testIDToStr(TestID id);
