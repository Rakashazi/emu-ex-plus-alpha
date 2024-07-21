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

#include "RecentContentView.hh"
#include <emuframework/EmuApp.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/fs/FS.hh>
#include <imagine/io/IO.hh>

namespace EmuEx
{

RecentContentView::RecentContentView(ViewAttachParams attach, RecentContent &recentContent_):
	TableView
	{
		"Recent Content", attach,
		[this](TableView::ItemMessage msg)
		{
			return msg.visit(overloaded
			{
				[&](const ItemsMessage&) -> ItemReply { return 1 + recentItems.size(); },
				[&](const GetItemMessage& m) -> ItemReply { return m.idx < recentItems.size() ? &recentItems[m.idx] : &clear; },
			});
		}
	},
	clear
	{
		"Clear List", attach,
		[this](const Input::Event &e)
		{
			pushAndShowModal(makeView<YesNoAlertView>("Really clear the list?",
				YesNoAlertView::Delegates
				{
					.onYes = [this]
					{
						recentContent.clear();
						dismiss();
					}
				}), e);
		}
	},
	recentContent{recentContent_}
{
	recentItems.reserve(recentContent_.size());
	for(auto &entry : recentContent_)
	{
		recentItems.emplace_back(entry.name, attach,
			[this, &entry](const Input::Event &e)
			{
				app().createSystemWithMedia({}, entry.path, appContext().fileUriDisplayName(entry.path), e, {}, attachParams(),
					[this](const Input::Event &e)
					{
						app().launchSystem(e);
					});
			});
	}
	clear.setActive(recentContent_.size());
}

}
