/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#define thisModuleName "filePicker"
#include <FilePicker.hh>
#include <MsgPopup.hh>
#include <EmuSystem.hh>
#include <Recent.hh>
#include <resource2/image/png/ResourceImagePng.h>
#include <util/gui/ViewStack.hh>
#include <gui/FSPicker/FSPicker.hh>

extern ViewStack viewStack;
void startGameFromMenu();
bool isMenuDismissKey(const InputEvent &e);
extern MsgPopup popup;
extern DLList<RecentGameInfo> recentGameList;
ResourceImage *getArrowAsset();
ResourceImage *getXAsset();

void EmuFilePicker::init(bool highlightFirst, FsDirFilterFunc filter, bool singleDir)
{
	FSPicker::init(".", needsUpDirControl ? getArrowAsset() : 0,
			View::needsBackControl ? getXAsset() : 0, filter, singleDir);
	onSelectFileDelegate().bind<&GameFilePicker::onSelectFile>();
	onCloseDelegate().bind<&GameFilePicker::onClose>();
	if(highlightFirst && tbl.cells)
	{
		tbl.selected = 0;
	}
}

void EmuFilePicker::initForBenchmark(bool highlightFirst, bool singleDir)
{
	EmuFilePicker::init(highlightFirst, defaultBenchmarkFsFilter, singleDir);
	onSelectFileDelegate().bind<&BenchmarkFilePicker::onSelectFile>();
	onCloseDelegate().bind<&BenchmarkFilePicker::onClose>();
}

void loadGameCompleteFromFilePicker(uint result = 1)
{
	if(result)
	{
		recent_addGame();
		startGameFromMenu();
	}
}

void GameFilePicker::onSelectFile(const char* name, const InputEvent &e)
{
	EmuSystem::loadGameCompleteDelegate().bind<&loadGameCompleteFromFilePicker>();
	if(EmuSystem::loadGame(name))
	{
		loadGameCompleteFromFilePicker();
	}
}

void GameFilePicker::onClose(const InputEvent &e)
{
	viewStack.popAndShow();
}

void loadGameCompleteFromBenchmarkFilePicker(uint result = 1)
{
	if(result)
	{
		logMsg("starting benchmark");
		TimeSys time = EmuSystem::benchmark();
		EmuSystem::closeGame(0);
		logMsg("done in: %f", double(time));
		popup.printf(2, 0, "%.2f fps", double(180.)/double(time));
	}
}

void BenchmarkFilePicker::onSelectFile(const char* name, const InputEvent &e)
{
	EmuSystem::loadGameCompleteDelegate().bind<&loadGameCompleteFromBenchmarkFilePicker>();
	if(!EmuSystem::loadGame(name, 0))
		return;
	loadGameCompleteFromBenchmarkFilePicker();
}

void BenchmarkFilePicker::onClose(const InputEvent &e)
{
	View::removeModalView();
}

#undef thisModuleName
