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

class EmuFilePicker : public FSPicker
{
public:
	EmuFilePicker(ViewAttachParams attach, const char *startingPath, bool pickingDir,
		EmuSystem::NameFilterFunc filter, FS::RootPathInfo rootInfo,
		Input::Event e, bool singleDir = false, bool includeArchives = true);
	static std::unique_ptr<EmuFilePicker> makeForBenchmarking(ViewAttachParams attach, Input::Event e, bool singleDir = false);
	static std::unique_ptr<EmuFilePicker> makeForLoading(ViewAttachParams attach, Input::Event e, bool singleDir = false);
	static std::unique_ptr<EmuFilePicker> makeForMediaChange(ViewAttachParams attach, Input::Event e, const char *path,
		EmuSystem::NameFilterFunc filter, FSPicker::OnSelectFileDelegate onSelect);
	static std::unique_ptr<EmuFilePicker> makeForMediaCreation(ViewAttachParams attach, Input::Event e, bool singleDir = false);
	bool inputEvent(Input::Event e) final;
	void setIncludeArchives(bool on);
};
