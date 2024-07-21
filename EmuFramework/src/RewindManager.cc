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

#include <emuframework/RewindManager.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/Option.hh>
#include <emuframework/EmuOptions.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"RewindMgr"};
constexpr Seconds defaultSaveFreq{1};

RewindManager::RewindManager(EmuApp &app):
	saveTimer
	{
		defaultSaveFreq,
		{.debugLabel = "RewindManager::saveStateTimer"},
		[this, &app]
		{
			//log.debug("running rewind save state timer");
			saveState(app);
			saveTimer.update();
			return true;
		}
	} {}

void RewindManager::clear()
{
	saveTimer.cancel();
	stateEntries = {};
	stateIdx = 0;
	stateSize = 0;
}

bool RewindManager::reset()
{
	if(!stateSize)
		return true;
	try
	{
		if(maxStates)
			log.info("allocating {} states of size:{}", maxStates, stateSize);
		stateEntries.reset(maxStates, stateSize);
		stateIdx = 0;
		return true;
	}
	catch(...)
	{
		return false;
	}
}

void RewindManager::saveState(EmuApp &app)
{
	assumeExpr(maxStates);
	assumeExpr(stateIdx < maxStates);
	//log.debug("saving rewind state index:{}", stateIdx);
	auto &entry = stateEntries[stateIdx];
	stateIdx = stateIdx + 1 == maxStates ? 0 : stateIdx + 1;
	entry.size = app.writeState({entry.data, stateSize}, {.uncompressed = true});
}

void RewindManager::rewindState(EmuApp &app)
{
	if(!maxStates)
		return;
	assumeExpr(stateIdx < maxStates);
	auto prevIdx = stateIdx ? stateIdx - 1 : maxStates - 1;
	auto &entry = stateEntries[prevIdx];
	if(!entry.size)
		return;
	log.info("rewinding to state index:{}", prevIdx);
	app.readState({entry.data, std::exchange(entry.size, 0)});
	stateIdx = prevIdx;
	saveTimer.reset();
}

void RewindManager::startTimer()
{
	if(!stateEntries.size())
		return;
	saveTimer.start();
}

void RewindManager::pauseTimer()
{
	saveTimer.pause();
}

bool RewindManager::readConfig(MapIO &io, unsigned key)
{
	switch(key)
	{
		default: return false;
		case CFGKEY_REWIND_STATES: return readOptionValue<uint32_t>(io, [&](auto m){ maxStates = m; });
		case CFGKEY_REWIND_TIMER_SECS: return readOptionValue<int16_t>(io, [&](auto s)
		{
			if(s > 0)
				saveTimer.frequency = Seconds{s};
		});
	}
}

void RewindManager::writeConfig(FileIO &io) const
{
	writeOptionValueIfNotDefault(io, CFGKEY_REWIND_STATES, uint32_t(maxStates), 0u);
	writeOptionValueIfNotDefault(io, CFGKEY_REWIND_TIMER_SECS, int16_t(saveTimer.frequency.count()), defaultSaveFreq.count());
}


}
