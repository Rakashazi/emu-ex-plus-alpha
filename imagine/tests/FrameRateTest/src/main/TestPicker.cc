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

#include "TestPicker.hh"

void TestTableEntry::draw(Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const
{
	BaseTextMenuItem::draw(xPos, yPos, xSize, ySize, align, projP);
	if(t2.str)
	{
		if(redText)
			Gfx::setColor(1., 0., 0.);
		else
			Gfx::setColor(1., 1., 1.);
		draw2ndText(xPos, yPos, xSize, ySize, align, projP);
	}
}

void TestPicker::init(const TestParams *testParams, uint tests)
{
	assert(tests <= MAX_TESTS);
	this->testParamPtr = testParams;
	iterateTimes(tests, i)
	{
		auto &entry = testEntry[i];
		entry.testStr = testParams[i].makeTestName();
		entry.init(entry.testStr.data(), nullptr);
		item[i] = &entry;
		entry.onSelect() =
			[this, i](DualTextMenuItem &, View &, const Input::Event &e)
			{
				auto test = startTest(window(), testParamPtr[i]);
				test->onTestFinished =
					[this, i](TestFramework &test)
					{
						using namespace Base;
						auto diff = test.endTime - test.startTime;
						logMsg("ran from %f to %f, took %f",
							frameTimeBaseToSDec(test.startTime),
							frameTimeBaseToSDec(test.endTime),
							frameTimeBaseToSDec(diff));
						auto &entry = testEntry[i];
						double fps = (double)Base::frameTimeBaseFromS(test.frames-1)/(double)diff;
						string_printf(entry.fpsStr, "%.2f", fps);
						entry.t2.setString(entry.fpsStr.data());
						entry.redText = test.droppedFrames;
					};
			};
	}
	TableView::init(item, tests, Input::keyInputIsPresent());
}
