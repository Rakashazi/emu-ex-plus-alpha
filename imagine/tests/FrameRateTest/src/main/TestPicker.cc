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
#include <imagine/util/algorithm.h>
#include <imagine/util/string.h>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/logger/logger.h>

TestTableEntry::TestTableEntry(SelectDelegate selectDel):
	DualTextMenuItem{{}, {}, selectDel}
{}

void TestTableEntry::draw(Gfx::RendererCommands &cmds, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const
{
	BaseTextMenuItem::draw(cmds, xPos, yPos, xSize, ySize, align, projP);
	if(t2.isVisible())
	{
		if(redText)
			cmds.setColor(1., 0., 0.);
		else
			cmds.setColor(1., 1., 1.);
		draw2ndText(cmds, xPos, yPos, xSize, ySize, align, projP);
	}
}

TestPicker::TestPicker(ViewAttachParams attach):
	TableView
	{
		attach,
		testEntry
	}
{}

void TestPicker::setTests(const TestDesc *testDesc, uint tests)
{
	testEntry.clear();
	testEntry.reserve(tests);
	testParam.clear();
	testParam.reserve(tests);
	iterateTimes(tests, i)
	{
		testEntry.emplace_back(
			[this, i](DualTextMenuItem &, View &, Input::Event e)
			{
				auto test = startTest(window(), renderer(), testParam[i]);
				test->onTestFinished =
					[this, i](TestFramework &test)
					{
						IG::FloatSeconds diff = test.endTime - test.startTime;
						logMsg("ran from %f to %f, took %f",
							IG::FloatSeconds(test.startTime).count(),
							IG::FloatSeconds(test.endTime).count(),
							diff.count());
						auto &entry = testEntry[i];
						auto fps = double(test.frames-1) / diff.count();
						entry.set2ndName(string_makePrintf<9>("%.2f", fps).data());
						entry.redText = test.droppedFrames;
					};
			});
		testEntry[i].setName(testDesc[i].name.data());
		testParam.emplace_back(testDesc[i].params);
	}
	if(Input::keyInputIsPresent())
		highlightCell(0);
}
