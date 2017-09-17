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

#define LOGTAG "FilePicker"
#include <emuframework/FilePicker.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuOptions.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/Recent.hh>
#include <imagine/gui/FSPicker.hh>
#include <imagine/gui/AlertView.hh>
#include <string>
#include "private.hh"

EmuFilePicker::EmuFilePicker(ViewAttachParams attach, const char *startingPath, bool pickingDir, EmuSystem::NameFilterFunc filter, bool singleDir):
	FSPicker
	{
		attach,
		needsUpDirControl ? &getAsset(attach.renderer, ASSET_ARROW) : nullptr,
		pickingDir ? &getAsset(attach.renderer, ASSET_ACCEPT) : View::needsBackControl ? &getAsset(attach.renderer, ASSET_CLOSE) : nullptr,
		pickingDir ?
		FSPicker::FilterFunc{[](FS::directory_entry &entry)
		{
			return entry.type() == FS::file_type::directory;
		}}:
		FSPicker::FilterFunc{[filter, singleDir](FS::directory_entry &entry)
		{
			logMsg("%s %d", entry.name(), (int)entry.type());
			if(!singleDir && entry.type() == FS::file_type::directory)
				return true;
			else if(!EmuSystem::handlesArchiveFiles && EmuApp::hasArchiveExtension(entry.name()))
				return true;
			else if(filter)
				return filter(entry.name());
			else
				return false;
		}},
		singleDir
	}
{
	bool setDefaultPath = true;
	if(strlen(startingPath))
	{
		setOnPathReadError(
			[](FSPicker &, std::error_code ec)
			{
				popup.printf(4, true, "Can't open last saved directory: %s", ec.message().c_str());
			});
		auto ec = setPath(startingPath, false);
		if(!ec)
		{
			setDefaultPath = false;
		}
	}
	setOnPathReadError(
		[](FSPicker &, std::error_code ec)
		{
			popup.printf(3, true, "Can't open directory: %s", ec.message().c_str());
		});
	if(setDefaultPath)
		setPath(Base::storagePath(), true);
}

EmuFilePicker *EmuFilePicker::makeForBenchmarking(ViewAttachParams attach, bool singleDir)
{
	auto picker = new EmuFilePicker{attach, lastLoadPath.data(), false, EmuSystem::defaultBenchmarkFsFilter, singleDir};
	picker->setOnChangePath(
		[](FSPicker &picker, FS::PathString, Input::Event)
		{
			lastLoadPath = picker.path();
		});
	picker->setOnSelectFile(
		[](FSPicker &picker, const char* name, Input::Event e)
		{
			EmuApp::createSystemWithMedia({}, name, "", e,
				[](Input::Event e)
				{
					runBenchmarkOneShot();
				});
		});
	return picker;
}

EmuFilePicker *EmuFilePicker::makeForLoading(ViewAttachParams attach, bool singleDir)
{
	auto picker = new EmuFilePicker{attach, lastLoadPath.data(), false, EmuSystem::defaultFsFilter, singleDir};
	picker->setOnChangePath(
		[](FSPicker &picker, FS::PathString, Input::Event)
		{
			lastLoadPath = picker.path();
		});
	picker->setOnSelectFile(
		[](FSPicker &picker, const char *name, Input::Event e)
		{
			onSelectFileFromPicker(picker.renderer(), picker.makePathString(name).data(), e);
		});
	return picker;
}

bool EmuFilePicker::inputEvent(Input::Event e)
{
	if(e.pushed())
	{
		if(e.isDefaultCancelButton())
		{
			onClose_.callCopy(*this, e);
			return true;
		}
		if(isMenuDismissKey(e))
		{
			if(EmuSystem::gameIsRunning())
			{
				dismiss();
				startGameFromMenu();
				return true;
			}
		}
	}
	return FSPicker::inputEvent(e);
}
