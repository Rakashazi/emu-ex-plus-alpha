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
#include <imagine/logger/logger.h>
#include <imagine/util/algorithm.h>

void TestTableEntry::draw(Gfx::Renderer &r, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const
{
	BaseTextMenuItem::draw(r, xPos, yPos, xSize, ySize, align, projP);
	if(t2.str)
	{
		if(redText)
			r.setColor(1., 0., 0.);
		else
			r.setColor(1., 1., 1.);
		draw2ndText(r, xPos, yPos, xSize, ySize, align, projP);
	}
}

TestPicker::TestPicker(ViewAttachParams attach):
	TableView
	{
		attach,
		[this](const TableView &)
		{
			return testEntry.size();
		},
		[this](const TableView &, int idx) -> MenuItem&
		{
			return testEntry[idx];
		}
	}
{}

void TestPicker::setTests(const TestParams *testParams, uint tests)
{
	testParamPtr = testParams;
	testEntry.clear();
	testEntry.reserve(tests);
	iterateTimes(tests, i)
	{
		testEntry.emplace_back(
			[this, i](DualTextMenuItem &, View &, Input::Event e)
			{
				auto test = startTest(window(), renderer(), testParamPtr[i]);
				test->onTestFinished =
					[this, i](TestFramework &test)
					{
						using namespace Base;
						auto diff = test.endTime - test.startTime;
						logMsg("ran from %f to %f, took %f",
							frameTimeBaseToSecsDec(test.startTime),
							frameTimeBaseToSecsDec(test.endTime),
							frameTimeBaseToSecsDec(diff));
						auto &entry = testEntry[i];
						double fps = (double)Base::frameTimeBaseFromSecs(test.frames-1)/(double)diff;
						string_printf(entry.fpsStr, "%.2f", fps);
						entry.t2.setString(entry.fpsStr.data());
						entry.redText = test.droppedFrames;
					};
			});
		testEntry[i].testStr = testParams[i].makeTestName();
	}
	if(Input::keyInputIsPresent())
		highlightCell(0);
}
