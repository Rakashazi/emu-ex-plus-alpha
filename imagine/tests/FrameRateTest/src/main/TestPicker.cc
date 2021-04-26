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
#include "main.hh"
#include <imagine/util/algorithm.h>
#include <imagine/util/string.h>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/logger/logger.h>

TestTableEntry::TestTableEntry(Gfx::GlyphTextureSet *face, SelectDelegate selectDel):
	DualTextMenuItem{{}, {}, face, selectDel}
{}

void TestTableEntry::draw(Gfx::RendererCommands &cmds, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize,
	Gfx::GC xIndent, _2DOrigin align, const Gfx::ProjectionPlane &projP, Gfx::Color color) const
{
	BaseTextMenuItem::draw(cmds, xPos, yPos, xSize, ySize, xIndent, align, projP, color);
	if(t2.isVisible())
	{
		Gfx::Color color2;
		if(redText)
			color2 = Gfx::color(1.f, 0.f, 0.f);
		else
			color2 = Gfx::color(1.f, 1.f, 1.f);
		draw2ndText(cmds, xPos, yPos, xSize, ySize, xIndent, align, projP, color2);
	}
}

TestPicker::TestPicker(ViewAttachParams attach):
	TableView
	{
		attach,
		testEntry
	}
{}

void TestPicker::setTests(const TestDesc *testDesc, unsigned tests)
{
	testEntry.clear();
	testEntry.reserve(tests);
	testParam.clear();
	testParam.reserve(tests);
	iterateTimes(tests, i)
	{
		testEntry.emplace_back(&defaultFace(),
			[this, i](DualTextMenuItem &, View &, Input::Event e)
			{
				auto &app = mainApp(appContext());
				auto test = app.startTest(window(), testParam[i]);
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
	if(appContext().keyInputIsPresent())
		highlightCell(0);
}
