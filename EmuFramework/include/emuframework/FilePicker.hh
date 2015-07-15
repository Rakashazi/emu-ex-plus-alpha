#pragma once

/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/gui/FSPicker.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuApp.hh>

using EmuNameFilterFunc = bool(*)(const char *name);

class EmuFilePicker : public FSPicker
{
public:
	static EmuNameFilterFunc defaultFsFilter;
	static EmuNameFilterFunc defaultBenchmarkFsFilter;

	EmuFilePicker(Base::Window &win): FSPicker(win) {}
	void init(bool highlightFirst, bool pickingDir, EmuNameFilterFunc filter = defaultFsFilter, bool singleDir = false);
	void initForBenchmark(bool highlightFirst, bool singleDir = 0);
	void inputEvent(const Input::Event &e) override;
};

class GameFilePicker
{
public:
	static void onSelectFile(const char* name, const Input::Event &e);
};

void loadGameCompleteFromFilePicker(uint result, const Input::Event &e);
