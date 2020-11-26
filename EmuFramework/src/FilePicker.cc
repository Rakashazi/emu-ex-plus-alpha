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
#include <emuframework/EmuApp.hh>
#include <imagine/base/Base.hh>
#include <imagine/gui/FSPicker.hh>
#include <imagine/logger/logger.h>
#include <string>
#include "private.hh"

EmuFilePicker::EmuFilePicker(ViewAttachParams attach, const char *startingPath, bool pickingDir,
	EmuSystem::NameFilterFunc filter, FS::RootPathInfo rootInfo,
	Input::Event e, bool singleDir, bool includeArchives):
	FSPicker
	{
		attach,
		needsUpDirControl ? &getAsset(attach.renderer(), ASSET_ARROW) : nullptr,
		pickingDir ? &getAsset(attach.renderer(), ASSET_ACCEPT) : View::needsBackControl ? &getAsset(attach.renderer(), ASSET_CLOSE) : nullptr,
		pickingDir ?
		FSPicker::FilterFunc{[](FS::directory_entry &entry)
		{
			return entry.type() == FS::file_type::directory;
		}}:
		FSPicker::FilterFunc{[filter, singleDir, includeArchives](FS::directory_entry &entry)
		{
			if(!singleDir && entry.type() == FS::file_type::directory)
				return true;
			else if(!EmuSystem::handlesArchiveFiles && includeArchives && EmuApp::hasArchiveExtension(entry.name()))
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
				EmuApp::printfMessage(4, true, "Can't open last saved directory: %s", ec.message().c_str());
			});
		if(auto ec = setPath(startingPath, false, rootInfo, e);
			!ec)
		{
			setDefaultPath = false;
		}
	}
	setOnPathReadError(
		[](FSPicker &, std::error_code ec)
		{
			EmuApp::printfMessage(3, true, "Can't open directory: %s", ec.message().c_str());
		});
	if(setDefaultPath)
	{
		setPath(Base::sharedStoragePathLocation(), true, e);
	}
}

std::unique_ptr<EmuFilePicker> EmuFilePicker::makeForBenchmarking(ViewAttachParams attach, Input::Event e, bool singleDir)
{
	auto searchPath = EmuApp::mediaSearchPath();
	auto rootInfo = Base::nearestRootPath(searchPath.data());
	auto picker = std::make_unique<EmuFilePicker>(attach, searchPath.data(), false, EmuSystem::defaultBenchmarkFsFilter, rootInfo, e, singleDir);
	picker->setOnChangePath(
		[](FSPicker &picker, FS::PathString, Input::Event)
		{
			EmuApp::setMediaSearchPath(picker.path());
		});
	picker->setOnSelectFile(
		[](FSPicker &picker, const char* name, Input::Event e)
		{
			EmuApp::postMessage("Running benchmark...");
			EmuApp::createSystemWithMedia({}, picker.makePathString(name).data(), "", e,
				[](Input::Event e)
				{
					runBenchmarkOneShot();
				});
		});
	return picker;
}

std::unique_ptr<EmuFilePicker> EmuFilePicker::makeForLoading(ViewAttachParams attach, Input::Event e, bool singleDir)
{
	auto searchPath = EmuApp::mediaSearchPath();
	auto rootInfo = Base::nearestRootPath(searchPath.data());
	auto picker = std::make_unique<EmuFilePicker>(attach, searchPath.data(), false, EmuSystem::defaultFsFilter, rootInfo, e, singleDir);
	picker->setOnChangePath(
		[](FSPicker &picker, FS::PathString, Input::Event)
		{
			EmuApp::setMediaSearchPath(picker.path());
		});
	picker->setOnSelectFile(
		[](FSPicker &picker, const char *name, Input::Event e)
		{
			onSelectFileFromPicker(picker.makePathString(name).data(), e);
		});
	return picker;
}

std::unique_ptr<EmuFilePicker> EmuFilePicker::makeForMediaChange(ViewAttachParams attach, Input::Event e, const char *path,
	EmuSystem::NameFilterFunc filter, FSPicker::OnSelectFileDelegate onSelect)
{
	auto picker = std::make_unique<EmuFilePicker>(attach, path, false, filter,
		FS::RootPathInfo{FS::makeFileString("Media Path"), strlen(path)}, e, true);
	picker->setOnSelectFile(onSelect);
	return picker;
}

std::unique_ptr<EmuFilePicker> EmuFilePicker::makeForMediaCreation(ViewAttachParams attach, Input::Event e, bool singleDir)
{
	auto rootInfo = Base::nearestRootPath(EmuSystem::baseSavePath().data());
	auto picker = std::make_unique<EmuFilePicker>(attach, EmuSystem::baseSavePath().data(), true, EmuSystem::NameFilterFunc{}, rootInfo, e, singleDir);
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
				emuViewController().showEmulation();
				return true;
			}
		}
	}
	return FSPicker::inputEvent(e);
}
