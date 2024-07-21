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
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppHelper.hh>
#include <emuframework/FilePicker.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/util/container/ArrayList.hh>
#include <imagine/util/concepts.hh>

namespace EmuEx
{

enum class DataPathSelectMode: uint8_t
{
	File, Folder
};

enum class ArchivePathSelectMode: uint8_t
{
	include, exclude
};

template<class T>
concept FileChangeCallable = Callable<T, bool, CStringView, FS::file_type>;

template<DataPathSelectMode mode, ArchivePathSelectMode archiveMode = ArchivePathSelectMode::include>
class DataPathSelectView : public TableView, public EmuAppHelper
{
public:
	enum class Mode: uint8_t
	{
		File, Folder
	};

	DataPathSelectView(UTF16Convertible auto &&name, ViewAttachParams attach, FS::PathString initialDir,
		FileChangeCallable auto &&onFileChange, EmuSystem::NameFilterFunc fsFilter_ = {}):
		TableView{IG_forward(name), attach, item},
		selectFolder
		{
			"Select Folder", attach,
			[=](View &view, const Input::Event &e)
			{
				auto fPicker = view.makeView<FilePicker>(FSPicker::Mode::DIR, EmuSystem::NameFilterFunc{}, e);
				auto &thisView = asThis(view);
				fPicker->setPath(thisView.searchDir, e);
				fPicker->setOnSelectPath(
					[=](FSPicker &picker, CStringView path, [[maybe_unused]] std::string_view displayName, const Input::Event&)
					{
						if(!onFileChange(path, FS::file_type::directory))
							return;
						picker.popTo();
						picker.dismissPrevious();
						picker.dismiss();
					});
				thisView.app().pushAndShowModalView(std::move(fPicker), e);
			}
		},
		selectFile
		{
			mode == DataPathSelectMode::File ? "Select File" : "Select Archive File", attach,
			[=](View &view, const Input::Event &e)
			{
				auto &thisView = asThis(view);
				auto fPicker = view.makeView<FilePicker>(FSPicker::Mode::FILE, thisView.fsFilter, e,
					archiveMode == ArchivePathSelectMode::include);
				fPicker->setPath(thisView.searchDir, e);
				fPicker->setOnSelectPath(
					[=](FSPicker &picker, CStringView path, std::string_view displayName, const Input::Event&)
					{
						if(mode == DataPathSelectMode::Folder && !EmuApp::hasArchiveExtension(displayName))
						{
							picker.applicationAs<EmuApp>().postErrorMessage("File doesn't have a valid extension");
							return;
						}
						if(!onFileChange(path, FS::file_type::regular))
							return;
						picker.popTo();
						picker.dismissPrevious();
						picker.dismiss();
					});
				thisView.app().pushAndShowModalView(std::move(fPicker), e);
			}
		},
		unset
		{
			"Unset", attach,
			[=](View &view)
			{
				onFileChange("", FS::file_type::none);
				view.dismiss();
			}
		},
		fsFilter{fsFilter_},
		searchDir{initialDir}
	{
		if(mode != DataPathSelectMode::File)
		{
			item.emplace_back(&selectFolder);
		}
		item.emplace_back(&selectFile);
		item.emplace_back(&unset);
	};

	void appendItem(TextMenuItem &i) { item.emplace_back(&i); }

protected:
	ConditionalMember<mode == DataPathSelectMode::Folder, TextMenuItem> selectFolder;
	TextMenuItem selectFile;
	TextMenuItem unset;
	ConditionalMember<mode == DataPathSelectMode::File, EmuSystem::NameFilterFunc> fsFilter;
	StaticArrayList<MenuItem*, 4> item;
	FS::PathString searchDir;

	static auto &asThis(View &view) { return static_cast<DataPathSelectView&>(view); }
};

template<ArchivePathSelectMode archiveMode = ArchivePathSelectMode::include>
using DataFileSelectView = DataPathSelectView<DataPathSelectMode::File, archiveMode>;

using DataFolderSelectView = DataPathSelectView<DataPathSelectMode::Folder>;

}
