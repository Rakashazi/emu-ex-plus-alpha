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

#include <emuframework/TurboInput.hh>
#include <emuframework/EmuApp.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"TurboInput"};

void TurboInput::addEvent(KeyInfo key)
{
	key.flags.turbo = 0;
	if(keys.empty())
		clock = 0; // Reset the clock so new turbo event takes effect next frame
	if(keys.tryPushBack(key))
	{
		log.info("added event action {}", key.codes[0]);
	}
}

void TurboInput::removeEvent(KeyInfo key)
{
	key.flags.turbo = 0;
	if(erase(keys, key))
	{
		log.info("removed event action {}", key.codes[0]);
	}
}

void TurboInput::updateEvent(EmuApp &app, KeyInfo key, Input::Action act)
{
	key.flags.turbo = 0;
	if(act == Input::Action::PUSHED)
	{
		addEvent(key);
	}
	else
	{
		removeEvent(key);
		app.handleSystemKeyInput(key, Input::Action::RELEASED, 0, {.allowTurboModifier = false});
	}
}

void TurboInput::update(EmuApp &app)
{
	const int turboFrames = 4;
	for(auto k : keys)
	{
		if(clock == 0)
		{
			app.handleSystemKeyInput(k, Input::Action::PUSHED, 0, {.allowTurboModifier = false});
		}
		else if(clock == turboFrames/2)
		{
			app.handleSystemKeyInput(k, Input::Action::RELEASED, 0, {.allowTurboModifier = false});
		}
	}
	clock++;
	if(clock == turboFrames) clock = 0;
}

}
