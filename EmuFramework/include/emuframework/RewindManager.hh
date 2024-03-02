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
#include <imagine/util/memory/FlexArray.hh>

namespace IG
{
class MapIO;
class FileIO;
}

namespace EmuEx
{

using namespace IG;

class EmuApp;

class RewindManager
{
public:
	RewindManager(EmuApp &);
	void clear();
	bool reset();
	void rewindState(EmuApp &);
	void startTimer();
	void pauseTimer();
	void resetTimer();
	bool readConfig(MapIO &, unsigned key);
	void writeConfig(FileIO &) const;

	void updateMaxStates(size_t max)
	{
		maxStates = max;
		reset();
	}

	bool reset(size_t stateSize_)
	{
		stateSize = stateSize_;
		return reset();
	}

private:
	struct StateEntry
	{
		size_t size{};
		uint8_t data[];
	};

	FlexArray<StateEntry> stateEntries;
	size_t stateIdx{};
public:
	size_t stateSize{};
	size_t maxStates{};
	PausableTimer<Seconds> saveTimer;

private:
	void saveState(EmuApp &);
};

}
