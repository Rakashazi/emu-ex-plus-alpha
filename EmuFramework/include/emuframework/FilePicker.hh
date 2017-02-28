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

class EmuFilePicker : public FSPicker
{
public:
	EmuFilePicker(ViewAttachParams attach, const char *startingPath, bool pickingDir, EmuSystem::NameFilterFunc filter, bool singleDir = false);
	static EmuFilePicker *makeForBenchmarking(ViewAttachParams attach, bool singleDir = false);
	static EmuFilePicker *makeForLoading(ViewAttachParams attach, bool singleDir = false);
	void inputEvent(Input::Event e) override;
};

class GameFilePicker
{
public:
	static void onSelectFile(Gfx::Renderer &r, const char* name, Input::Event e);
};

void loadGameComplete(bool tryAutoState, bool addToRecent);
void loadGameCompleteFromFilePicker(Gfx::Renderer &r, uint result, Input::Event e);
bool hasArchiveExtension(const char *name);
