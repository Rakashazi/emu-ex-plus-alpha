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

enum TestID
{
	TEST_CLEAR,
	TEST_DRAW,
	TEST_WRITE,
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
	bool shouldEndTest{};
	uint frames{};
	uint droppedFrames{};
	uint continuousFrames{};
	Base::FrameTimeBase startTime{}, endTime{};
	TestFinishedDelegate onTestFinished;

	constexpr TestFramework() {}
	virtual ~TestFramework() {}
	virtual void initTest(IG::Point2D<int> pixmapSize) {}
	virtual void placeTest(const Gfx::GCRect &testRect) {}
	virtual void frameUpdateTest(Base::Screen &screen, Base::FrameTimeBase frameTime) = 0;
	virtual void deinitTest() {}
	virtual void drawTest() = 0;
	void init(IG::Point2D<int> pixmapSize);
	void deinit();
	void place(const Gfx::ProjectionPlane &projP, const Gfx::GCRect &testRect);
	void frameUpdate(Base::Screen &screen, Base::FrameTimeBase frameTime);
	void draw();
	void finish(Base::FrameTimeBase frameTime);
	void setCPUFreqText(const char *str);

protected:
	Gfx::Text cpuFreqText;
	Gfx::GCRect cpuFreqRect{};
	std::array<char, 256> cpuFreqStr{};
	Gfx::Text skippedFrameText;
	Gfx::GCRect skippedFrameRect{};
	std::array<char, 256> skippedFrameStr{};
	Gfx::ProjectionPlane projP;

	void placeCPUFreqText();
	void placeSkippedFrameText();
};

class ClearTest : public TestFramework
{
protected:
	bool flash{true};

public:
	ClearTest() {}

	void frameUpdateTest(Base::Screen &screen, Base::FrameTimeBase frameTime) override;
	void drawTest() override;
};

class DrawTest : public TestFramework
{
protected:
	int flash{true};
	IG::Pixmap pixmap{PixelFormatRGB565};
	Gfx::PixmapTexture texture;
	Gfx::Sprite sprite;
	char *pixBuff{};

public:
	DrawTest() {}

	void initTest(IG::Point2D<int> pixmapSize) override;
	void placeTest(const Gfx::GCRect &rect) override;
	void deinitTest() override;
	void frameUpdateTest(Base::Screen &screen, Base::FrameTimeBase frameTime) override;
	void drawTest() override;
};

class WriteTest : public DrawTest
{
public:
	WriteTest() {}

	void drawTest() override;
};

TestFramework *startTest(Base::Window &win, const TestParams &t);
const char *testIDToStr(TestID id);
