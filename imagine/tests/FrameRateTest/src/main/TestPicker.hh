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

#include <imagine/gui/MenuItem.hh>
#include <imagine/gui/TableView.hh>
#include "tests.hh"

namespace FrameRateTest
{

using namespace IG;

class TestPicker : public IG::TableView
{
public:
	TestPicker(IG::ViewAttachParams attach);
	void setTests(const TestDesc *testParams, unsigned tests);

private:
	std::vector<DualTextMenuItem> testEntry;
	std::vector<TestParams> testParam;
};

}
