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
	IG::CStringView startingPath, bool pickingDir,
	EmuSystem::NameFilterFunc filter, FS::RootPathInfo rootInfo,
	Input::Event e, bool singleDir, bool includeArchives):
	EmuFilePicker
	{
		attach, EmuApp::get(attach.appContext()),
		startingPath, pickingDir, filter, rootInfo,
		e, singleDir, includeArchives
	}
{}

EmuFilePicker::EmuFilePicker(ViewAttachParams attach,
	EmuApp &app, IG::CStringView startingPath, bool pickingDir,
	EmuSystem::NameFilterFunc filter, FS::RootPathInfo rootInfo,
	Input::Event e, bool singleDir, bool includeArchives):
	FSPicker
	{
		attach,
		needsUpDirControl ? &app.asset(EmuApp::AssetID::ARROW) : nullptr,
		pickingDir ? &app.asset(EmuApp::AssetID::ACCEPT) : &app.asset(EmuApp::AssetID::CLOSE),
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
			[this, &app](FSPicker &, std::error_code ec)
			{
				app.postMessage(4, true, fmt::format("Can't open last saved directory: {}", ec.message()));
			});
		if(auto ec = setPath(startingPath, false, rootInfo, e);
			!ec)
		{
			setDefaultPath = false;
		}
	}
	setOnPathReadError(
		[this, &app](FSPicker &, std::error_code ec)
		{
			app.postMessage(3, true, fmt::format("Can't open directory: {}", ec.message()));
		});
	if(setDefaultPath)
	{
		setPath(appContext().sharedStoragePathLocation(), true, e);
	}
}

std::unique_ptr<EmuFilePicker> EmuFilePicker::makeForBenchmarking(ViewAttachParams attach, Input::Event e, bool singleDir)
{
	auto &app = EmuApp::get(attach.appContext());
	auto searchPath = app.mediaSearchPath();
	auto rootInfo = attach.appContext().nearestRootPath(searchPath.data());
	auto picker = std::make_unique<EmuFilePicker>(attach, app, searchPath.data(), false, EmuSystem::defaultBenchmarkFsFilter, rootInfo, e, singleDir);
	picker->setOnChangePath(
		[&app](FSPicker &picker, FS::PathString, Input::Event)
		{
			app.setMediaSearchPath(picker.path());
		});
	picker->setOnSelectFile(
		[&app](FSPicker &picker, const char* name, Input::Event e)
		{
			app.postMessage("Running benchmark...");
			app.createSystemWithMedia({}, picker.makePathString(name), name, e, {}, picker.attachParams(),
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
	auto searchPath = app.mediaSearchPath();
	auto rootInfo = attach.appContext().nearestRootPath(searchPath.data());
	auto picker = std::make_unique<EmuFilePicker>(attach, app, searchPath.data(), false, EmuSystem::defaultFsFilter, rootInfo, e, singleDir);
	picker->setOnChangePath(
		[&app](FSPicker &picker, FS::PathString, Input::Event)
		{
			app.setMediaSearchPath(picker.path());
		});
	picker->setOnSelectFile(
		[=, &app](FSPicker &picker, const char *name, Input::Event e)
		{
			onSelectFileFromPicker(app, {}, picker.makePathString(name).data(), name, false, e, params, picker.attachParams());
		});
	return picker;
}

void EmuFilePicker::browseForLoading(ViewAttachParams attach, EmuSystemCreateParams params)
{
	auto ctx = attach.appContext();
	ctx.showSystemDocumentPicker(
		[=](const char *uri, GenericIO io)
		{
			auto &app = EmuApp::get(ctx);
			onSelectFileFromPicker(app, std::move(io), uri, "", true, ctx.defaultInputEvent(), params, app.attachParams());
		});
}

std::unique_ptr<EmuFilePicker> EmuFilePicker::makeForMediaChange(ViewAttachParams attach, Input::Event e, IG::CStringView path, EmuSystem::NameFilterFunc filter, FSPicker::OnSelectFileDelegate onSelect)
{
	auto picker = std::make_unique<EmuFilePicker>(attach, path, false, filter,
		FS::RootPathInfo{FS::makeFileString("Media Path"), strlen(path)}, e, true);
	picker->setOnSelectFile(onSelect);
	return picker;
}

std::unique_ptr<EmuFilePicker> EmuFilePicker::makeForMediaCreation(ViewAttachParams attach, Input::Event e, bool singleDir)
{
	auto ctx = attach.appContext();
	auto rootInfo = ctx.nearestRootPath(EmuSystem::baseSavePath(ctx).data());
	auto picker = std::make_unique<EmuFilePicker>(attach, EmuSystem::baseSavePath(ctx).data(), true, EmuSystem::NameFilterFunc{}, rootInfo, e, singleDir);
	return picker;
}

std::unique_ptr<EmuFilePicker> EmuFilePicker::makeForMediaCreation(ViewAttachParams attach, bool singleDir)
{
	return makeForMediaCreation(attach, attach.appContext().defaultInputEvent(), singleDir);
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

void EmuApp::requestFilePickerForLoading(View &parentView, ViewAttachParams attach, Input::Event e,
	bool singleDir, EmuSystemCreateParams params)
{
	auto ctx = appContext();
	if(ctx.usesPermission(Base::Permission::WRITE_EXT_STORAGE))
	{
		if(!ctx.requestPermission(Base::Permission::WRITE_EXT_STORAGE))
			return;
	}
	parentView.pushAndShow(EmuFilePicker::makeForLoading(attach, e, singleDir, params), e, false);
}
