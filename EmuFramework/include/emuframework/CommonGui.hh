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

#include <meta.h>
#include <emuframework/EmuApp.hh>
#include <emuframework/FileUtils.hh>
#include <main/EmuMenuViews.hh>

static SystemMenuView mMenu(mainWin.win);

void initMainMenu(Base::Window &win)
{
	mMenu.win = &win;
	mMenu.init(Input::keyInputIsPresent());
}

View &mainMenu()
{
	return mMenu;
}

View *makeOptionCategoryMenu(Base::Window &win, const Input::Event &e, uint idx)
{
	auto oCategoryMenu = new SystemOptionView{win};
	oCategoryMenu->init(idx, !e.isPointer());
	return oCategoryMenu;
}

const char *appViewTitle()
{
	return CONFIG_APP_NAME " " IMAGINE_VERSION;
}

const char *appName()
{
	return CONFIG_APP_NAME;
}

const char *appID()
{
	return CONFIG_APP_ID;
}
