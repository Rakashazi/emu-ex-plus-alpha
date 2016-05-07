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

class AutoStateConfirmAlertView : public YesNoAlertView
{
	std::array<char, 96> msg{};

public:
	AutoStateConfirmAlertView(Base::Window &win, const char *dateStr, bool addToRecent):
		YesNoAlertView{win, "", "Continue", "Restart Game"}
	{
		string_printf(msg, "Auto-save state exists from:\n%s", dateStr);
		setLabel(msg.data());
		setOnYes(
			[addToRecent](TextMenuItem &, View &view, Input::Event e)
			{
				view.dismiss();
				loadGameComplete(true, addToRecent);
			});
		setOnNo(
			[addToRecent](TextMenuItem &, View &view, Input::Event e)
			{
				view.dismiss();
				loadGameComplete(false, addToRecent);
			});
	}
};

void loadGameCompleteFromBenchmarkFilePicker(uint result, Input::Event e);

bool hasArchiveExtension(const char *name)
{
	return string_hasDotExtension(name, "7z") ||
		string_hasDotExtension(name, "rar") ||
		string_hasDotExtension(name, "zip");
}

EmuFilePicker::EmuFilePicker(Base::Window &win, bool pickingDir, EmuSystem::NameFilterFunc filter, bool singleDir):
	FSPicker
	{
		win,
		needsUpDirControl ? &getAsset(ASSET_ARROW) : nullptr,
		pickingDir ? &getAsset(ASSET_ACCEPT) : View::needsBackControl ? &getAsset(ASSET_CLOSE) : nullptr,
		[filter, singleDir](FS::directory_entry &entry)
		{
			logMsg("%s %d", entry.name(), (int)entry.type());
			if(!singleDir && entry.type() == FS::file_type::directory)
				return true;
			else if(!EmuSystem::handlesArchiveFiles && hasArchiveExtension(entry.name()))
				return true;
			else if(filter)
				return filter(entry.name());
			else
				return false;
		},
		singleDir
	}
{
	if(setPath(FS::current_path().data(), false) != OK)
	{
		setPath(Base::storagePath(), true);
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

EmuFilePicker *EmuFilePicker::makeForBenchmarking(Base::Window &win, bool singleDir)
{
	auto picker = new EmuFilePicker{win, false, EmuSystem::defaultBenchmarkFsFilter, singleDir};
	picker->setOnSelectFile(
		[](FSPicker &picker, const char* name, Input::Event e)
		{
			EmuSystem::onLoadGameComplete() =
				[](uint result, Input::Event e)
				{
					loadGameCompleteFromBenchmarkFilePicker(result, e);
				};
			auto res = EmuSystem::loadGameFromPath(FS::makePathString(name));
			if(res == 1)
			{
				loadGameCompleteFromBenchmarkFilePicker(1, e);
			}
			else if(res == 0)
			{
				EmuSystem::clearGamePaths();
			}
		});
	return picker;
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
		auto mTime = FS::status(saveStr).lastWriteTimeLocal();
		char dateStr[64]{};
		std::strftime(dateStr, sizeof(dateStr), strftimeFormat, &mTime);
		auto &ynAlertView = *new AutoStateConfirmAlertView{mainWin.win, dateStr, addToRecent};
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
	auto res = EmuSystem::loadGameFromPath(FS::makePathString(name));
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

void EmuFilePicker::inputEvent(Input::Event e)
{
	if(e.state == Input::PUSHED)
	{
		if(e.isDefaultCancelButton())
		{
			onCloseD.callCopy(*this, e);
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
