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

#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuAppHelper.hh>
#include <imagine/gui/FSPicker.hh>

class EmuFilePicker : public FSPicker, public EmuAppHelper<EmuFilePicker>
{
public:
	EmuFilePicker(ViewAttachParams, const char *startingPath, bool pickingDir,
		EmuSystem::NameFilterFunc, FS::RootPathInfo,
		Input::Event, bool singleDir = false, bool includeArchives = true);
	EmuFilePicker(ViewAttachParams, EmuApp &, const char *startingPath, bool pickingDir,
		EmuSystem::NameFilterFunc, FS::RootPathInfo,
		Input::Event, bool singleDir = false, bool includeArchives = true);
	static std::unique_ptr<EmuFilePicker> makeForBenchmarking(ViewAttachParams, Input::Event, bool singleDir = false);
	static std::unique_ptr<EmuFilePicker> makeForLoading(ViewAttachParams, Input::Event, bool singleDir = false, EmuSystemCreateParams params = {});
	static std::unique_ptr<EmuFilePicker> makeForMediaChange(ViewAttachParams, Input::Event, const char *path,
		EmuSystem::NameFilterFunc filter, FSPicker::OnSelectFileDelegate);
	static std::unique_ptr<EmuFilePicker> makeForMediaCreation(ViewAttachParams, Input::Event, bool singleDir = false);
	static std::unique_ptr<EmuFilePicker> makeForMediaCreation(ViewAttachParams, bool singleDir = false);
	bool inputEvent(Input::Event) final;
	void setIncludeArchives(bool on);
};
