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

#include <imagine/gui/AlertView.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuSystem.hh>

namespace EmuEx
{

class ResetAlertView : public BaseAlertView
{
public:
	ResetAlertView(ViewAttachParams attach, UTF16Convertible auto &&label, EmuApp &app):
		BaseAlertView{attach, IG_forward(label), items},
		items
		{
			TextMenuItem
			{
				"Soft Reset", attach,
				[&app]()
				{
					app.system().reset(app, EmuSystem::ResetMode::SOFT);
					app.showEmulation();
				}
			},
			TextMenuItem
			{
				"Hard Reset", attach,
				[&app]()
				{
					app.system().reset(app, EmuSystem::ResetMode::HARD);
					app.showEmulation();
				}
			},
			TextMenuItem{"Cancel", attach, [](){}}
		} {}

protected:
	std::array<TextMenuItem, 3> items;
};

inline std::unique_ptr<View> resetAlertView(ViewAttachParams attachParams, EmuApp &app)
{
	if(EmuSystem::hasResetModes)
	{
		return std::make_unique<ResetAlertView>(attachParams, "Really reset?", app);
	}
	else
	{
		return std::make_unique<YesNoAlertView>(attachParams, "Really reset?",
			YesNoAlertView::Delegates
			{
				.onYes = [&app]
				{
					app.system().reset(app, EmuSystem::ResetMode::SOFT);
					app.showEmulation();
				}
			});
	}
}

}
