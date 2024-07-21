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
#include <emuframework/Option.hh>
#include <emuframework/EmuOptions.hh>
#include "pathUtils.hh"
#include <imagine/io/MapIO.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"AutosaveMgr"};
constexpr Minutes defaultSaveFreq{0};

AutosaveManager::AutosaveManager(EmuApp &app_):
	app{app_},
	saveTimer
	{
		defaultSaveFreq,
		{.debugLabel = "AutosaveManager::autosaveTimer"},
		[this]
		{
			log.debug("running autosave timer");
			save();
			saveTimer.update();
			return true;
		}
	} {}

bool AutosaveManager::save(AutosaveActionSource src)
{
	if(autoSaveSlot == noAutosaveName)
		return true;
	log.info("saving autosave slot:{}", autoSaveSlot);
	system().flushBackupMemory(app);
	if(saveOnlyBackupMemory && src == AutosaveActionSource::Auto)
		return true;
	return saveState();
}

bool AutosaveManager::load(AutosaveActionSource src, LoadAutosaveMode mode)
{
	if(autoSaveSlot == noAutosaveName)
		return true;
	try
	{
		system().loadBackupMemory(app);
		if(saveOnlyBackupMemory && src == AutosaveActionSource::Auto)
			return true;
		if(!stateIO)
			stateIO = appContext().openFileUri(statePath(), OpenFlags::createFile());
		if(stateIO.getExpected<uint8_t>(0)) // check if state contains data
		{
			if(mode == LoadAutosaveMode::NoState)
			{
				log.info("skipped loading autosave state");
				return true;
			}
			return loadState();
		}
		else
		{
			log.info("autosave state doesn't exist, creating");
			return saveState();
		}
	}
	catch(std::exception &err)
	{
		if(!hasWriteAccessToDir(system().contentSaveDirectory()))
			app.postErrorMessage(8, "Save folder inaccessible, please set it in Options➔File Paths➔Saves");
		else
			app.postErrorMessage(4, err.what());
		return false;
	}
}

bool AutosaveManager::saveState()
{
	log.info("saving autosave state");
	stateIO.truncate(0);
	auto state = app.saveState();
	if(stateIO.write(state.span(), 0).bytes != ssize_t(state.size()))
	{
		app.postErrorMessage(4, "Error writing autosave state");
		return false;
	}
	return true;
}

bool AutosaveManager::loadState()
{
	log.info("loading autosave state");
	try
	{
		app.readState(stateIO.buffer(IOBufferMode::Direct));
		return true;
	}
	catch(std::exception &err)
	{
		app.postErrorMessage(4, std::format("Error loading autosave state:\n{}", err.what()));
		return false;
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
	resetSlot(name);
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
		[ctx](const FS::directory_entry &e)
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

void AutosaveManager::startTimer()
{
	if(!timerFrequency().count())
		return;
	saveTimer.start();
}

void AutosaveManager::pauseTimer()
{
	saveTimer.pause();
}

void AutosaveManager::cancelTimer()
{
	saveTimer.cancel();
}

void AutosaveManager::resetTimer()
{
	saveTimer.reset();
}

SteadyClockTime AutosaveManager::timerFrequency() const
{
	if(autoSaveSlot == noAutosaveName)
		return {};
	return saveTimer.frequency;
}

bool AutosaveManager::readConfig(MapIO &io, unsigned key)
{
	switch(key)
	{
		default: return false;
		case CFGKEY_AUTOSAVE_LAUNCH_MODE: return readOptionValue(io, autosaveLaunchMode, [](auto m){return m <= lastEnum<AutosaveLaunchMode>;});
		case CFGKEY_AUTOSAVE_TIMER_MINS: return readOptionValue<decltype(saveTimer.frequency.count())>(io, [&](auto m)
		{
			Minutes mins = Minutes{m};
			if(mins >= Minutes{0} && mins <= maxAutosaveSaveFreq)
				saveTimer.frequency = mins;
		});
		case CFGKEY_AUTOSAVE_CONTENT: return readOptionValue(io, saveOnlyBackupMemory);
	}
}

void AutosaveManager::writeConfig(FileIO &io) const
{
	writeOptionValueIfNotDefault(io, CFGKEY_AUTOSAVE_LAUNCH_MODE, autosaveLaunchMode, AutosaveLaunchMode::Load);
	writeOptionValueIfNotDefault(io, CFGKEY_AUTOSAVE_TIMER_MINS, saveTimer.frequency.count(), defaultSaveFreq.count());
	writeOptionValueIfNotDefault(io, CFGKEY_AUTOSAVE_CONTENT, saveOnlyBackupMemory, false);
}

ApplicationContext AutosaveManager::appContext() const { return system().appContext(); }

}
