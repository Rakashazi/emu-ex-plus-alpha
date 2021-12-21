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
#include "private.hh"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gui/FSPicker.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>
#include <string>

EmuFilePicker::EmuFilePicker(ViewAttachParams attach,
	FSPicker::Mode mode, EmuSystem::NameFilterFunc filter, Input::Event e, bool includeArchives):
	EmuFilePicker
	{
		attach, EmuApp::get(attach.appContext()), mode, filter, e, includeArchives
	} {}

EmuFilePicker::EmuFilePicker(ViewAttachParams attach, EmuApp &app,
	FSPicker::Mode mode, EmuSystem::NameFilterFunc filter, Input::Event e, bool includeArchives):
	FSPicker
	{
		attach,
		&app.asset(EmuApp::AssetID::ARROW),
		mode == FSPicker::Mode::DIR ? &app.asset(EmuApp::AssetID::ACCEPT) : &app.asset(EmuApp::AssetID::CLOSE),
		mode == FSPicker::Mode::DIR ?
		FSPicker::FilterFunc{} :
		FSPicker::FilterFunc{[filter, includeArchives](auto &entry)
		{
			if(entry.type() == FS::file_type::directory)
				return true;
			else if(!EmuSystem::handlesArchiveFiles && includeArchives && EmuApp::hasArchiveExtension(entry.name()))
				return true;
			else if(filter)
				return filter(entry.name());
			else
				return false;
		}},
		mode
	} {}

std::unique_ptr<EmuFilePicker> EmuFilePicker::makeForBenchmarking(ViewAttachParams attach, Input::Event e, bool singleDir)
{
	auto &app = EmuApp::get(attach.appContext());
	auto mode = singleDir ? FSPicker::Mode::FILE_IN_DIR : FSPicker::Mode::FILE;
	auto picker = std::make_unique<EmuFilePicker>(attach, app, mode, EmuSystem::defaultBenchmarkFsFilter, e);
	picker->setPath(app.contentSearchPath(), e);
	picker->setOnChangePath(
		[&app](FSPicker &picker, FS::PathString prevPath, Input::Event)
		{
			app.setContentSearchPath(picker.path());
		});
	picker->setOnSelectFile(
		[&app](FSPicker &picker, std::string_view path, std::string_view displayName, Input::Event e)
		{
			app.postMessage("Running benchmark...");
			app.createSystemWithMedia({}, path, displayName, e, {}, picker.attachParams(),
				[&app](Input::Event e)
				{
					runBenchmarkOneShot(app, app.video());
				});
		});
	return picker;
}

std::unique_ptr<EmuFilePicker> EmuFilePicker::makeForLoading(ViewAttachParams attach, Input::Event e,
	bool singleDir, EmuSystemCreateParams params)
{
	auto &app = EmuApp::get(attach.appContext());
	auto mode = singleDir ? FSPicker::Mode::FILE_IN_DIR : FSPicker::Mode::FILE;
	auto picker = std::make_unique<EmuFilePicker>(attach, app, mode, EmuSystem::defaultFsFilter, e);
	picker->setPath(app.contentSearchPath(), e);
	picker->setOnChangePath(
		[&app](FSPicker &picker, FS::PathString prevPath, Input::Event)
		{
			app.setContentSearchPath(picker.path());
		});
	picker->setOnSelectFile(
		[=, &app](FSPicker &picker, std::string_view path, std::string_view displayName, Input::Event e)
		{
			onSelectFileFromPicker(app, {}, path, displayName, e, params, picker.attachParams());
		});
	return picker;
}

std::unique_ptr<EmuFilePicker> EmuFilePicker::makeForMediaChange(ViewAttachParams attach, Input::Event e, EmuSystem::NameFilterFunc filter, FSPicker::OnSelectFileDelegate onSelect)
{
	auto picker = std::make_unique<EmuFilePicker>(attach, FSPicker::Mode::FILE_IN_DIR, filter, e);
	picker->setPath(EmuSystem::contentDirectory(), e);
	picker->setOnSelectFile(onSelect);
	return picker;
}

std::unique_ptr<EmuFilePicker> EmuFilePicker::makeForMediaCreation(ViewAttachParams attach, Input::Event e)
{
	auto &app = EmuApp::get(attach.appContext());
	auto mode = FSPicker::Mode::DIR;
	auto picker = std::make_unique<EmuFilePicker>(attach, app, mode, EmuSystem::NameFilterFunc{}, e);
	picker->setPath(app.contentSearchPath(), e);
	return picker;
}

std::unique_ptr<EmuFilePicker> EmuFilePicker::makeForMediaCreation(ViewAttachParams attach)
{
	return makeForMediaCreation(attach, attach.appContext().defaultInputEvent());
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
	}
	return FSPicker::inputEvent(e);
}
