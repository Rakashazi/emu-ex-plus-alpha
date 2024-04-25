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

#include <emuframework/config.hh>
#include <imagine/base/PausableTimer.hh>
#include <imagine/fs/FSDefs.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/util/enum.hh>
#include <string>
#include <string_view>
#include <functional>

namespace EmuEx
{

using namespace IG;

class EmuApp;
class EmuSystem;

WISE_ENUM_CLASS((AutosaveLaunchMode, uint8_t),
	Load,
	LoadNoState,
	Ask,
	NoSave);

enum class LoadAutosaveMode{Normal, NoState};
enum class AutosaveActionSource{Auto, Manual};

constexpr std::string_view defaultAutosaveFilename = "auto-00";
constexpr std::string_view noAutosaveName = "\a";
constexpr Minutes maxAutosaveSaveFreq{720};

class AutosaveManager
{
public:
	AutosaveManager(EmuApp &);
	bool save(AutosaveActionSource src = AutosaveActionSource::Auto);
	bool load(AutosaveActionSource src, LoadAutosaveMode m);
	bool load(LoadAutosaveMode m) { return load(AutosaveActionSource::Auto, m); }
	bool load(AutosaveActionSource src = AutosaveActionSource::Auto) { return load(src, LoadAutosaveMode::Normal); }
	bool setSlot(std::string_view name);
	void resetSlot(std::string_view name = "")
	{
		autoSaveSlot = name;
		saveTimer.cancel();
		stateIO = {};
	}
	bool renameSlot(std::string_view name, std::string_view newName);
	bool deleteSlot(std::string_view name);
	std::string_view slotName() const { return autoSaveSlot; }
	std::string slotFullName() const;
	std::string stateTimeAsString() const;
	WallClockTimePoint stateTime() const;
	WallClockTimePoint backupMemoryTime() const;
	FS::PathString statePath() const { return statePath(autoSaveSlot); }
	FS::PathString statePath(std::string_view name) const;
	void startTimer();
	void pauseTimer();
	void cancelTimer();
	void resetTimer();
	SteadyClockTime timerFrequency() const;
	bool readConfig(MapIO &, unsigned key);
	void writeConfig(FileIO &) const;
	ApplicationContext appContext() const;
	auto& system(this auto&& self) { return self.app.system(); }

private:
	EmuApp &app;
	std::string autoSaveSlot;
	FileIO stateIO;

	bool saveState();
	bool loadState();

public:
	PausableTimer<Minutes> saveTimer;
	AutosaveLaunchMode autosaveLaunchMode{};
	bool saveOnlyBackupMemory{};
};

}
