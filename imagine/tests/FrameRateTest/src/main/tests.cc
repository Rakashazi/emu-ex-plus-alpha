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

#include <imagine/gui/TableView.hh>
#include "tests.hh"

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

void TestFramework::init(IG::Point2D<int> pixmapSize)
{
	cpuFreqText.init(View::defaultFace);
	cpuFreqText.maxLines = 1;
	cpuFreqText.setString(cpuFreqStr.data());
	skippedFrameText.init(View::defaultFace);
	skippedFrameText.setString(skippedFrameStr.data());
	initTest(pixmapSize);
}

void TestFramework::deinit()
{
	skippedFrameText.deinit();
	deinitTest();
}

void TestFramework::setCPUFreqText(const char *str)
{
	string_printf(cpuFreqStr, "CPU Frequency: %s", str);
	placeCPUFreqText();
}

void TestFramework::placeCPUFreqText()
{
	if(strlen(cpuFreqStr.data()))
	{
		cpuFreqText.compile(projP);
		cpuFreqRect = projP.bounds();
		cpuFreqRect.y = cpuFreqRect.y2 - cpuFreqText.ySize * 1.5f; // adjust to top
	}
}

void TestFramework::placeSkippedFrameText()
{
	if(strlen(skippedFrameStr.data()))
	{
		skippedFrameText.compile(projP);
		skippedFrameRect = projP.bounds();
		skippedFrameRect.y2 = skippedFrameRect.y + skippedFrameText.ySize * 1.5f; // adjust to bottom
	}
}

void TestFramework::place(const Gfx::ProjectionPlane &projP, const Gfx::GCRect &testRect)
{
	var_selfs(projP);
	skippedFrameText.maxLineSize = projP.bounds().xSize();
	placeCPUFreqText();
	placeSkippedFrameText();
	placeTest(testRect);
}

void TestFramework::frameUpdate(Base::Screen &screen, Base::FrameTimeBase frameTime)
{
	if(!frames)
	{
		startTime = frameTime;
		//logMsg("start time: %llu", (unsigned long long)startTime);
	}
	else
	{
		auto elapsedScreenFrames = screen.elapsedFrames(frameTime);
		//logMsg("elapsed: %d", screen.elapsedFrames(frameTime));
		if(elapsedScreenFrames > 1)
		{
			droppedFrames++;
			string_printf(skippedFrameStr, "Lost %u frame(s) after %u continuous\nat time %f",
				elapsedScreenFrames - 1, continuousFrames, Base::frameTimeBaseToSDec(frameTime));
			placeSkippedFrameText();
			continuousFrames = 0;
		}
	}
	frameUpdateTest(screen, frameTime);
	frames++;
	continuousFrames++;
}

void TestFramework::draw()
{
	using namespace Gfx;
	drawTest();
	if(strlen(cpuFreqStr.data()))
	{
		noTexProgram.use();
		setBlendMode(BLEND_MODE_ALPHA);
		setColor(0., 0., 0., .7);
		GeomRect::draw(cpuFreqRect);
		setColor(1., 1., 1., 1.);
		texAlphaProgram.use();
		cpuFreqText.draw(projP.alignXToPixel(cpuFreqRect.x + TableView::globalXIndent),
			projP.alignYToPixel(cpuFreqRect.yCenter()), LC2DO, projP);
	}
	if(strlen(skippedFrameStr.data()))
	{
		noTexProgram.use();
		setBlendMode(BLEND_MODE_ALPHA);
		setColor(0., 0., 0., .7);
		GeomRect::draw(skippedFrameRect);
		setColor(1., 1., 1., 1.);
		texAlphaProgram.use();
		skippedFrameText.draw(projP.alignXToPixel(skippedFrameRect.x + TableView::globalXIndent),
			projP.alignYToPixel(skippedFrameRect.yCenter()), LC2DO, projP);
	}
}

void TestFramework::finish(Base::FrameTimeBase frameTime)
{
	endTime = frameTime;
	if(onTestFinished)
		onTestFinished(*this);
	deinit();
}


void ClearTest::frameUpdateTest(Base::Screen &screen, Base::FrameTimeBase frameTime)
{
	toggle(flash);
}

void ClearTest::drawTest()
{
	using namespace Gfx;
	if(flash)
	{
		if(!droppedFrames)
			setClearColor(.7, .7, .7);
		else if(droppedFrames % 2 == 0)
			setClearColor(.7, .7, .0);
		else
			setClearColor(.7, .0, .0);
	}
	else
	{
		setClearColor(0, 0, 0);
	}
	Gfx::clear();
}

void DrawTest::initTest(IG::Point2D<int> pixmapSize)
{
	pixBuff = new char[pixmap.sizeOfPixels(pixmapSize.x * pixmapSize.y)];
	pixmap.init(pixBuff, pixmapSize.x, pixmapSize.y);
	memset(pixmap.data, 0xFF, pixmap.pitch * pixmap.y);
	Gfx::TextureConfig texConf{pixmap};
	texConf.setWillWriteOften(true);
	doOrAbort(texture.init(texConf));
	texture.write(0, pixmap, {});
	texture.compileDefaultProgram(Gfx::IMG_MODE_REPLACE);
	texture.compileDefaultProgram(Gfx::IMG_MODE_MODULATE);
	sprite.init({}, texture);
	Gfx::TextureSampler::initDefaultNoMipClampSampler();
}

void DrawTest::placeTest(const Gfx::GCRect &rect)
{
	sprite.setPos(rect);
}

void DrawTest::deinitTest()
{
	sprite.deinit();
	texture.deinit();
	delete[] pixBuff;
}

void DrawTest::frameUpdateTest(Base::Screen &screen, Base::FrameTimeBase frameTime)
{
	toggle(flash);
}

void DrawTest::drawTest()
{
	using namespace Gfx;
	Gfx::clear();
	setBlendMode(Gfx::BLEND_MODE_OFF);
	Gfx::TextureSampler::bindDefaultNoMipClampSampler();
	sprite.useDefaultProgram(IMG_MODE_MODULATE);
	if(flash)
	{
		if(!droppedFrames)
			setColor(.7, .7, .7, 1.);
		else if(droppedFrames % 2 == 0)
			setColor(.7, .7, .0, 1.);
		else
			setColor(.7, .0, .0, 1.);
	}
	else
		setColor(0., 0., 0., 1.);
	sprite.draw();
}

void WriteTest::drawTest()
{
	using namespace Gfx;
	Gfx::clear();
	setBlendMode(Gfx::BLEND_MODE_OFF);
	Gfx::TextureSampler::bindDefaultNoMipClampSampler();
	sprite.useDefaultProgram(IMG_MODE_REPLACE);
	if(flash)
	{
		uint writeColor;
		if(!droppedFrames)
			writeColor = PixelFormatRGB565.build(.7, .7, .7, 1.);
		else if(droppedFrames % 2 == 0)
			writeColor = PixelFormatRGB565.build(.7, .7, .0, 1.);
		else
			writeColor = PixelFormatRGB565.build(.7, .0, .0, 1.);
		iterateTimes(pixmap.x * pixmap.y, i)
		{
			((uint16*)pixmap.data)[i] = writeColor;
		}
	}
	else
	{
		memset(pixmap.data, 0, pixmap.pitch * pixmap.y);
	}
	texture.write(0, pixmap, {});
	sprite.draw();
}
