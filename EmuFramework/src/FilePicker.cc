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

void EmuFilePicker::init(bool highlightFirst, bool pickingDir, FsDirFilterFunc filter, bool singleDir)
{
	FSPicker::init(".", needsUpDirControl ? &getAsset(ASSET_ARROW) : nullptr,
		pickingDir ? &getAsset(ASSET_ACCEPT) : View::needsBackControl ? &getAsset(ASSET_CLOSE) : nullptr, filter, singleDir);
	onSelectFile() = [this](FSPicker &picker, const char* name, const Input::Event &e){GameFilePicker::onSelectFile(name, e);};
	if(highlightFirst)
	{
		tbl.highlightFirstCell();
	}
}

void loadGameComplete(bool tryAutoState, bool addToRecent)
{
	if(tryAutoState)
		EmuSystem::loadAutoState();
	if(addToRecent)
		recent_addGame();
	startGameFromMenu();
}

bool showAutoStateConfirm(const Input::Event &e, bool addToRecent)
{
	if(!(optionConfirmAutoLoadState && optionAutoSaveState))
	{
		return 0;
	}
	auto saveStr = EmuSystem::sprintStateFilename(-1);
	if(FsSys::fileExists(saveStr.data()))
	{
		FsSys::timeStr date = "";
		FsSys::mTimeAsStr(saveStr.data(), date);
		static char msg[96] = "";
		snprintf(msg, sizeof(msg), "Auto-save state exists from:\n%s", date);
		auto &ynAlertView = *new YesNoAlertView{mainWin.win};
		ynAlertView.init(msg, !e.isPointer(), "Continue", "Restart Game");
		ynAlertView.onYes() =
			[addToRecent](const Input::Event &e)
			{
				loadGameComplete(true, addToRecent);
			};
		ynAlertView.onNo() =
			[addToRecent](const Input::Event &e)
			{
				loadGameComplete(false, addToRecent);
			};
		modalViewController.pushAndShow(ynAlertView);
		return 1;
	}
	return 0;
}

void loadGameCompleteFromFilePicker(uint result, const Input::Event &e)
{
	if(!result)
		return;

	if(!showAutoStateConfirm(e, true))
	{
		loadGameComplete(1, 1);
	}
}

void GameFilePicker::onSelectFile(const char* name, const Input::Event &e)
{
	EmuSystem::onLoadGameComplete() =
		[](uint result, const Input::Event &e)
		{
			loadGameCompleteFromFilePicker(result, e);
		};
	auto res = EmuSystem::loadGame(name);
	if(res == 1)
	{
		loadGameCompleteFromFilePicker(1, e);
	}
	else if(res == 0)
	{
		EmuSystem::clearGamePaths();
	}
}

void loadGameCompleteFromBenchmarkFilePicker(uint result, const Input::Event &e)
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

void EmuFilePicker::initForBenchmark(bool highlightFirst, bool singleDir)
{
	EmuFilePicker::init(highlightFirst, false, defaultBenchmarkFsFilter, singleDir);
	onSelectFile() =
		[this](FSPicker &picker, const char* name, const Input::Event &e)
		{
			EmuSystem::onLoadGameComplete() =
				[](uint result, const Input::Event &e)
				{
					loadGameCompleteFromBenchmarkFilePicker(result, e);
				};
			auto res = EmuSystem::loadGame(name);
			if(res == 1)
			{
				loadGameCompleteFromBenchmarkFilePicker(1, e);
			}
			else if(res == 0)
			{
				EmuSystem::clearGamePaths();
			}
		};
}
