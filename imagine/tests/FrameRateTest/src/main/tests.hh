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
#include <imagine/gfx/PixmapBufferTexture.hh>
#include <imagine/gfx/SyncFence.hh>
#include <imagine/time/Time.hh>
#include <imagine/thread/Semaphore.hh>
#include <imagine/base/ApplicationContext.hh>

class ViewAttachParams;

enum TestID
{
	TEST_CLEAR,
	TEST_DRAW,
	TEST_WRITE,
};

struct FramePresentTime
{
	IG::FrameTime timestamp{};
	IG::Time atOnFrame{};
	IG::Time atWinPresent{};

	constexpr FramePresentTime() {}
};

struct TestParams
{
	TestID test{};
	IG::WP pixmapSize{};
	Gfx::TextureBufferMode bufferMode{};

	constexpr TestParams(TestID test)
		: test{test} {}

	constexpr TestParams(TestID test, IG::WP pixmapSize, Gfx::TextureBufferMode bufferMode)
		: test{test}, pixmapSize{pixmapSize}, bufferMode{bufferMode} {}
};

struct TestDesc
{
	TestParams params;
	std::string name;

	TestDesc(TestID test, const char *name, IG::WP pixmapSize = {},
		Gfx::TextureBufferMode bufferMode = {})
		: params{test, pixmapSize, bufferMode}, name{name} {}
};

class TestFramework
{
public:
	using TestFinishedDelegate = DelegateFunc<void (TestFramework &test)>;
	bool started{};
	bool shouldEndTest{};
	unsigned frames{};
	unsigned droppedFrames{};
	unsigned continuousFrames{};
	IG::FrameTime startTime{}, endTime{};
	TestFinishedDelegate onTestFinished;
	FramePresentTime lastFramePresentTime;
	Gfx::SyncFence presentFence{};

	TestFramework() {}
	virtual ~TestFramework() {}
	virtual void initTest(Base::ApplicationContext, Gfx::Renderer &, IG::WP pixmapSize, Gfx::TextureBufferMode) {}
	virtual void placeTest(const Gfx::GCRect &testRect) {}
	virtual void frameUpdateTest(Gfx::RendererTask &rendererTask, Base::Screen &screen, IG::FrameTime frameTime) = 0;
	virtual void drawTest(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds) = 0;
	virtual void presentedTest(Gfx::RendererCommands &cmds) {}
	void init(Base::ApplicationContext, Gfx::Renderer &, Gfx::GlyphTextureSet &face, IG::WP pixmapSize, Gfx::TextureBufferMode);
	void place(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP, const Gfx::GCRect &testRect);
	void frameUpdate(Gfx::RendererTask &rTask, Base::Window &win, Base::FrameParams frameParams);
	void prepareDraw(Gfx::Renderer &r);
	void draw(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds, Gfx::GC xIndent);
	void finish(Gfx::RendererTask &task, IG::FrameTime frameTime);
	void setCPUFreqText(const char *str);
	void setCPUUseText(const char *str);

protected:
	Gfx::Text cpuStatsText;
	Gfx::GCRect cpuStatsRect{};
	std::array<char, 256> cpuFreqStr{};
	std::array<char, 64> cpuUseStr{};
	Gfx::Text frameStatsText;
	Gfx::GCRect frameStatsRect{};
	std::array<char, 256> skippedFrameStr{};
	std::array<char, 256> statsStr{};
	Gfx::ProjectionPlane projP;
	unsigned lostFrameProcessTime = 0;

	void placeCPUStatsText(Gfx::Renderer &r);
	void placeFrameStatsText(Gfx::Renderer &r);
};

class ClearTest : public TestFramework
{
protected:
	bool flash{true};

public:
	ClearTest() {}

	void frameUpdateTest(Gfx::RendererTask &rendererTask, Base::Screen &screen, IG::FrameTime frameTime) override;
	void drawTest(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds) override;
};

class DrawTest : public TestFramework
{
protected:
	int flash{true};
	Gfx::PixmapBufferTexture texture;
	Gfx::Sprite sprite;

public:
	DrawTest() {}

	void initTest(Base::ApplicationContext, Gfx::Renderer &, IG::WP pixmapSize, Gfx::TextureBufferMode) override;
	void placeTest(const Gfx::GCRect &rect) override;
	void frameUpdateTest(Gfx::RendererTask &rendererTask, Base::Screen &screen, IG::FrameTime frameTime) override;
	void drawTest(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds) override;
};

class WriteTest : public DrawTest
{
public:
	WriteTest() {}
	~WriteTest() override;

	void frameUpdateTest(Gfx::RendererTask &rendererTask, Base::Screen &screen, IG::FrameTime frameTime) override;
	void drawTest(Gfx::RendererCommands &cmds, Gfx::ClipRect bounds) override;
};

const char *testIDToStr(TestID id);
