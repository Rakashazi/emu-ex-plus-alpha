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

void EmuFilePicker::init(bool pickingDir, EmuNameFilterFunc filter, bool singleDir)
{
	FSPicker::init(needsUpDirControl ? &getAsset(ASSET_ARROW) : nullptr,
		pickingDir ? &getAsset(ASSET_ACCEPT) : View::needsBackControl ? &getAsset(ASSET_CLOSE) : nullptr,
				[filter, singleDir](FS::directory_entry &entry)
				{
					logMsg("%s %d", entry.name(), (int)entry.type());
					if(!singleDir && entry.type() == FS::file_type::directory)
						return true;
					else if(filter)
						return filter(entry.name());
					else
						return false;
				}, singleDir);
	if(setPath(FS::current_path().data()) != OK)
	{
		setPath(Base::storagePath());
	}
	setOnPathReadError(
		[](FSPicker &, CallResult res)
		{
			switch(res)
			{
				bcase PERMISSION_DENIED: popup.postError("Permission denied reading directory");
				bcase NOT_FOUND: popup.postError("Directory not found");
				bcase INVALID_PARAMETER: popup.postError("Not a directory");
				bdefualt: popup.postError("Unknown error reading directory");
			}
		});
	setOnSelectFile(
		[this](FSPicker &, const char *name, Input::Event e)
		{
			GameFilePicker::onSelectFile(name, e);
		});
}

void loadGameComplete(bool tryAutoState, bool addToRecent)
{
	if(tryAutoState)
		EmuSystem::loadAutoState();
	if(addToRecent)
		recent_addGame();
	startGameFromMenu();
}

bool showAutoStateConfirm(Input::Event e, bool addToRecent)
{
	if(!(optionConfirmAutoLoadState && optionAutoSaveState))
	{
		return 0;
	}
	auto saveStr = EmuSystem::sprintStateFilename(-1);
	if(FS::exists(saveStr))
	{
		auto mTime = FS::status(saveStr).last_write_time_local();
		char dateStr[64]{};
		std::strftime(dateStr, sizeof(dateStr), strftimeFormat, &mTime);
		static char msg[96] = "";
		snprintf(msg, sizeof(msg), "Auto-save state exists from:\n%s", dateStr);
		auto &ynAlertView = *new YesNoAlertView{mainWin.win, msg, "Continue", "Restart Game"};
		ynAlertView.onYes() =
			[addToRecent](Input::Event e)
			{
				loadGameComplete(true, addToRecent);
			};
		ynAlertView.onNo() =
			[addToRecent](Input::Event e)
			{
				loadGameComplete(false, addToRecent);
			};
		modalViewController.pushAndShow(ynAlertView, e);
		return 1;
	}
	return 0;
}

void loadGameCompleteFromFilePicker(uint result, Input::Event e)
{
	if(!result)
		return;

	if(!showAutoStateConfirm(e, true))
	{
		loadGameComplete(1, 1);
	}
}

void GameFilePicker::onSelectFile(const char* name, Input::Event e)
{
	EmuSystem::onLoadGameComplete() =
		[](uint result, Input::Event e)
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

void loadGameCompleteFromBenchmarkFilePicker(uint result, Input::Event e)
{
	if(result)
	{
		logMsg("starting benchmark");
		IG::Time time = EmuSystem::benchmark();
		EmuSystem::closeGame(0);
		logMsg("done in: %f", double(time));
		popup.printf(2, 0, "%.2f fps", double(180.)/double(time));
	}
}

void EmuFilePicker::initForBenchmark(bool singleDir)
{
	EmuFilePicker::init(false, defaultBenchmarkFsFilter, singleDir);
	setOnSelectFile(
		[this](FSPicker &picker, const char* name, Input::Event e)
		{
			EmuSystem::onLoadGameComplete() =
				[](uint result, Input::Event e)
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
		});
}

void EmuFilePicker::inputEvent(Input::Event e)
{
	if(e.state == Input::PUSHED)
	{
		if(e.isDefaultCancelButton())
		{
			onCloseD(*this, e);
			return;
		}

		if(isMenuDismissKey(e))
		{
			if(EmuSystem::gameIsRunning())
			{
				dismiss();
				startGameFromMenu();
				return;
			}
		}
	}

	FSPicker::inputEvent(e);
}
