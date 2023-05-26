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

namespace EmuEx
{

void TurboInput::addEvent(unsigned action)
{
	Action *slot = std::ranges::find_if(activeAction, [](Action a){ return a == 0; });
	if(slot != activeAction.end())
	{
		*slot = action;
		logMsg("added turbo event action %d", action);
	}
}

void TurboInput::removeEvent(unsigned action)
{
	for(auto &e : activeAction)
	{
		if(e == action)
		{
			e = 0;
			logMsg("removed turbo event action %d", action);
		}
	}
}


void TurboInput::update(EmuApp &app)
{
	static const int turboFrames = 4;

	for(auto e : activeAction)
	{
		if(e)
		{
			if(clock == 0)
			{
				//logMsg("turbo push for player %d, action %d", e.player, e.action);
				app.system().handleInputAction(&app, {e, Input::Action::PUSHED});
			}
			else if(clock == turboFrames/2)
			{
				//logMsg("turbo release for player %d, action %d", e.player, e.action);
				app.system().handleInputAction(&app, {e, Input::Action::RELEASED});
			}
		}
	}
	clock++;
	if(clock == turboFrames) clock = 0;
}

}
