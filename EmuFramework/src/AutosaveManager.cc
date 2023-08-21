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

#include <emuframework/AutosaveManager.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuSystem.hh>
#include "EmuOptions.hh"
#include "pathUtils.hh"
#include <imagine/io/MapIO.hh>
#include <imagine/io/FileIO.hh>

namespace EmuEx
{

AutosaveManager::AutosaveManager(EmuApp &app_):
	app{app_},
	autoSaveTimer
	{
		"AutosaveManager::autosaveTimer",
		[this]()
		{
			logMsg("running autosave timer");
			app.syncEmulationThread();
			save();
			resetTimer();
			return true;
		}
	} {}

bool AutosaveManager::save(AutosaveActionSource src)
{
	if(autoSaveSlot == noAutosaveName)
		return true;
	logMsg("saving autosave slot:%s", autoSaveSlot.c_str());
	system().flushBackupMemory(app);
	if(saveOnlyBackupMemory && src == AutosaveActionSource::Auto)
		return true;
	return app.saveState(statePath());
}

bool AutosaveManager::load(AutosaveActionSource src, LoadAutosaveMode mode)
{
	if(autoSaveSlot == noAutosaveName)
		return true;
	try
	{
		system().loadBackupMemory(app);
	}
	catch(std::exception &err)
	{
		if(!hasWriteAccessToDir(system().contentSaveDirectory()))
			app.postErrorMessage(8, "Save folder inaccessible, please set it in Options➔File Paths➔Saves");
		else
			app.postErrorMessage(4, err.what());
		return false;
	}
	if(saveOnlyBackupMemory && src == AutosaveActionSource::Auto)
		return true;
	auto path = statePath();
	if(appContext().fileUriExists(path))
	{
		if(mode == LoadAutosaveMode::NoState)
		{
			logMsg("skipped loading autosave state");
			return true;
		}
		logMsg("loading autosave state");
		return app.loadState(path);
	}
	else
	{
		logMsg("autosave state doesn't exist, creating");
		return app.saveState(path);
	}
}

bool AutosaveManager::setSlot(std::string_view name)
{
	if(autoSaveSlot == name)
		return true;
	if(!save())
		return false;
	if(name.size() && name != noAutosaveName)
	{
		if(!system().createContentLocalSaveDirectory(name))
			return false;
	}
	autoSaveSlot = name;
	autoSaveTimerElapsedTime = {};
	return load();
}

bool AutosaveManager::renameSlot(std::string_view name, std::string_view newName)
{
	if(!appContext().renameFileUri(system().contentLocalSaveDirectory(name),
		system().contentLocalSaveDirectory(newName)))
	{
		return false;
	}
	if(name == autoSaveSlot)
		autoSaveSlot = newName;
	return true;
}

bool AutosaveManager::deleteSlot(std::string_view name)
{
	if(name == autoSaveSlot)
		return false;
	auto ctx = appContext();
	if(!ctx.forEachInDirectoryUri(system().contentLocalSaveDirectory(name),
			[this, ctx](const FS::directory_entry &e)
		{
			ctx.removeFileUri(e.path());
			return true;
		}, {.test = true}))
	{
		return false;
	}
	if(!ctx.removeFileUri(system().contentLocalSaveDirectory(name)))
	{
		return false;
	}
	return true;
}

std::string AutosaveManager::slotFullName() const
{
	if(autoSaveSlot == noAutosaveName)
		return "No Save";
	else if(autoSaveSlot.empty())
		return "Main";
	else
		return autoSaveSlot;
}

std::string AutosaveManager::stateTimeAsString() const
{
	if(autoSaveSlot == noAutosaveName)
		return "";
	return appContext().fileUriFormatLastWriteTimeLocal(statePath());
}

WallClockTimePoint AutosaveManager::stateTime() const
{
	if(autoSaveSlot == noAutosaveName)
		return {};
	return appContext().fileUriLastWriteTime(statePath());
}

WallClockTimePoint AutosaveManager::backupMemoryTime() const
{
	if(!system().usesBackupMemory() || autoSaveSlot == noAutosaveName)
		return {};
	return system().backupMemoryLastWriteTime(app);
}

FS::PathString AutosaveManager::statePath(std::string_view name) const
{
	if(name == noAutosaveName)
		return "";
	if(name.empty())
		return system().statePath(-1);
	return system().contentLocalSaveDirectory(name, system().stateFilename(defaultAutosaveFilename));
}

void AutosaveManager::pauseTimer()
{
	autoSaveTimerElapsedTime = SteadyClock::now() - autoSaveTimerStartTime;
	autoSaveTimer.cancel();
}

void AutosaveManager::cancelTimer()
{
	autoSaveTimerElapsedTime = {};
	autoSaveTimer.cancel();
}

void AutosaveManager::resetTimer()
{
	autoSaveTimerStartTime = SteadyClock::now();
}

void AutosaveManager::startTimer()
{
	if(!timerFrequency().count())
		return;
	autoSaveTimer.run(nextTimerFireTime(), timerFrequency());
	autoSaveTimerStartTime = SteadyClock::now();
}

SteadyClockTime AutosaveManager::nextTimerFireTime() const
{
	auto timerFreq = timerFrequency();
	if(autoSaveTimerElapsedTime < timerFreq)
		return timerFreq - autoSaveTimerElapsedTime;
	return {};
}

SteadyClockTime AutosaveManager::timerFrequency() const
{
	if(autoSaveSlot == noAutosaveName)
		return {};
	return autosaveTimerMins;
}

bool AutosaveManager::readConfig(MapIO &io, unsigned key, size_t size)
{
	switch(key)
	{
		default: return false;
		case CFGKEY_AUTOSAVE_LAUNCH_MODE: return readOptionValue(io, size, autosaveLaunchMode, [](auto m){return m <= lastEnum<AutosaveLaunchMode>;});
		case CFGKEY_AUTOSAVE_TIMER_MINS: return readOptionValue<decltype(autosaveTimerMins.count())>(io, size, [&](auto m)
		{
			if(m >= 0 && m <= 15)
				autosaveTimerMins = IG::Minutes{m};
		});
		case CFGKEY_AUTOSAVE_CONTENT: return readOptionValue(io, size, saveOnlyBackupMemory);
	}
}

void AutosaveManager::writeConfig(FileIO &io) const
{
	writeOptionValueIfNotDefault(io, CFGKEY_AUTOSAVE_LAUNCH_MODE, autosaveLaunchMode, AutosaveLaunchMode::Load);
	writeOptionValueIfNotDefault(io, CFGKEY_AUTOSAVE_TIMER_MINS, autosaveTimerMins.count(), 5);
	writeOptionValueIfNotDefault(io, CFGKEY_AUTOSAVE_CONTENT, saveOnlyBackupMemory, false);
}

EmuSystem &AutosaveManager::system() { return app.system(); }
const EmuSystem &AutosaveManager::system() const { return app.system(); }
ApplicationContext AutosaveManager::appContext() const { return system().appContext(); }

}
