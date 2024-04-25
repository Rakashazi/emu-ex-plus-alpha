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

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppHelper.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/util/container/ArrayList.hh>
#include <string_view>

namespace EmuEx
{

class FilePathOptionView : public TableView, public EmuAppHelper
{
public:
	FilePathOptionView(ViewAttachParams attach, bool customMenu = false);
	void loadStockItems();

protected:
	TextMenuItem savePath;
	TextMenuItem screenshotPath;
	StaticArrayList<MenuItem*, 7> item;

	void onSavePathChange(std::string_view path);
};

}
