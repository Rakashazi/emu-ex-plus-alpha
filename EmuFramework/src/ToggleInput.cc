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

#include <emuframework/ToggleInput.hh>
#include <emuframework/EmuApp.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"ToggleInput"};

void ToggleInput::updateEvent(EmuApp &app, KeyInfo key, Input::Action act)
{
	if(act != Input::Action::PUSHED)
		return;
	key.flags.toggle = 0;
	if(!std::ranges::contains(keys, key))
	{
		if(keys.tryPushBack(key))
		{
			log.info("added event action {}", key.codes[0]);
			app.handleSystemKeyInput(key, Input::Action::PUSHED);
		}
	}
	else if(erase(keys, key))
	{
		log.info("removed event action {}", key.codes[0]);
		app.handleSystemKeyInput(key, Input::Action::RELEASED);
	}
}

}
