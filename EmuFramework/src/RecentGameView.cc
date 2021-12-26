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

#include "RecentGameView.hh"
#include <imagine/gui/AlertView.hh>
#include <imagine/fs/FS.hh>

RecentGameView::RecentGameView(ViewAttachParams attach, EmuApp::RecentContentList &list):
	TableView
	{
		"Recent Content",
		attach,
		[this](const TableView &)
		{
			return 1 + recentGame.size();
		},
		[this](const TableView &, size_t idx) -> MenuItem&
		{
			return idx < recentGame.size() ? recentGame[idx] : clear;
		}
	},
	clear
	{
		"Clear List", &defaultFace(),
		[this](Input::Event e)
		{
			auto ynAlertView = makeView<YesNoAlertView>("Really clear the list?");
			ynAlertView->setOnYes(
				[this]()
				{
					this->list.clear();
					dismiss();
				});
			pushAndShowModal(std::move(ynAlertView), e);
		}
	},
	list{list}
{
	recentGame.reserve(list.size());
	for(auto &entry : list)
	{
		auto &recentItem = recentGame.emplace_back(entry.name, &defaultFace(),
			[this, &entry](Input::Event e)
			{
				app().createSystemWithMedia({}, entry.path, appContext().fileUriDisplayName(entry.path), e, {}, attachParams(),
					[this](Input::Event e)
					{
						app().launchSystemWithResumePrompt(e);
					});
			});
	}
	clear.setActive(list.size());
}
