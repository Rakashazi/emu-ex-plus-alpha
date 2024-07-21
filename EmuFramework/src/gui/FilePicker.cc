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

#include <emuframework/FilePicker.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuApp.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gui/FSPicker.hh>
#include <imagine/fs/FS.hh>
#include <imagine/io/IO.hh>
#include <string>

namespace EmuEx
{

FilePicker::FilePicker(ViewAttachParams attach,
	FSPicker::Mode mode, EmuSystem::NameFilterFunc filter, const Input::Event &e, bool includeArchives):
	FilePicker
	{
		attach, EmuApp::get(attach.appContext()), mode, filter, e, includeArchives
	} {}

FilePicker::FilePicker(ViewAttachParams attach, EmuApp &app,
	FSPicker::Mode mode, EmuSystem::NameFilterFunc filter, const Input::Event&, bool includeArchives):
	FSPicker
	{
		attach,
		app.asset(AssetID::arrow),
		mode == FSPicker::Mode::DIR ? app.asset(AssetID::accept) : app.asset(AssetID::close),
		mode == FSPicker::Mode::DIR ?
		FSPicker::FilterFunc{} :
		FSPicker::FilterFunc{[filter, includeArchives](auto &entry)
		{
			if(entry.type() == FS::file_type::directory)
				return true;
			else if(includeArchives && EmuApp::hasArchiveExtension(entry.name()))
				return true;
			else if(filter)
				return filter(entry.name());
			else
				return false;
		}},
		mode
	}
{
	if(app.showHiddenFilesInPicker)
		setShowHiddenFiles(true);
}

std::unique_ptr<FilePicker> FilePicker::forBenchmarking(ViewAttachParams attach, const Input::Event &e, bool singleDir)
{
	auto &app = EmuApp::get(attach.appContext());
	auto mode = singleDir ? FSPicker::Mode::FILE_IN_DIR : FSPicker::Mode::FILE;
	auto picker = std::make_unique<FilePicker>(attach, app, mode, EmuSystem::defaultFsFilter, e);
	picker->setPath(app.contentSearchPath, e);
	picker->setOnChangePath(
		[&app](FSPicker &picker, const Input::Event &)
		{
			app.contentSearchPath = picker.path();
		});
	picker->setOnSelectPath(
		[&app](FSPicker &picker, CStringView path, std::string_view displayName, const Input::Event &e)
		{
			app.postMessage("Running benchmark...");
			app.createSystemWithMedia({}, path, displayName, e, {}, picker.attachParams(),
				[&app](const Input::Event &)
				{
					app.runBenchmarkOneShot(app.video);
				});
		});
	return picker;
}

std::unique_ptr<FilePicker> FilePicker::forLoading(ViewAttachParams attach, const Input::Event &e,
	bool singleDir, EmuSystemCreateParams params)
{
	auto &app = EmuApp::get(attach.appContext());
	auto mode = singleDir ? FSPicker::Mode::FILE_IN_DIR : FSPicker::Mode::FILE;
	auto picker = std::make_unique<FilePicker>(attach, app, mode, EmuSystem::defaultFsFilter, e);
	picker->setPath(app.contentSearchPath, e);
	picker->setOnChangePath(
		[&app](FSPicker &picker, const Input::Event &)
		{
			app.contentSearchPath = picker.path();
		});
	picker->setOnSelectPath(
		[=, &app](FSPicker &picker, CStringView path, std::string_view displayName, const Input::Event &e)
		{
			app.onSelectFileFromPicker({}, path, displayName, e, params, picker.attachParams());
		});
	return picker;
}

std::unique_ptr<FilePicker> FilePicker::forMediaChange(ViewAttachParams attach, const Input::Event &e,
	EmuSystem::NameFilterFunc filter, FSPicker::OnSelectPathDelegate onSelect, bool singleDir)
{
	auto &app = EmuApp::get(attach.appContext());
	auto mode = singleDir ? FSPicker::Mode::FILE_IN_DIR : FSPicker::Mode::FILE;
	auto picker = std::make_unique<FilePicker>(attach, mode, filter, e);
	picker->setPath(app.system().contentDirectory(), e);
	picker->setOnSelectPath(onSelect);
	return picker;
}

std::unique_ptr<FilePicker> FilePicker::forMediaCreation(ViewAttachParams attach, const Input::Event &e)
{
	auto &app = EmuApp::get(attach.appContext());
	auto mode = FSPicker::Mode::DIR;
	auto picker = std::make_unique<FilePicker>(attach, app, mode, EmuSystem::NameFilterFunc{}, e);
	picker->setPath(app.contentSearchPath, e);
	return picker;
}

std::unique_ptr<FilePicker> FilePicker::forMediaCreation(ViewAttachParams attach)
{
	return forMediaCreation(attach, attach.appContext().defaultInputEvent());
}

}
