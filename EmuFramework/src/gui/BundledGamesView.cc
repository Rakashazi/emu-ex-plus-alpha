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

#include <emuframework/BundledGamesView.hh>
#include <emuframework/EmuApp.hh>
#include <imagine/logger/logger.h>
#include <imagine/io/IO.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/base/ApplicationContext.hh>

namespace EmuEx
{

constexpr SystemLogger log{"BundledGamesView"};

BundledGamesView::BundledGamesView(ViewAttachParams attach):
	TableView
	{
		"Bundled Content",
		attach,
		game
	},
	game
	{
		{
			system().bundledGameInfo(0).displayName, attach,
			[this](const Input::Event &e)
			{
				auto &info = system().bundledGameInfo(0);
				auto file = appContext().openAsset(info.assetName, OpenFlags{.test = true, .accessHint = IOAccessHint::All});
				if(!file)
				{
					log.error("error opening bundled game asset:{}", info.assetName);
					return;
				}
				app().createSystemWithMedia(std::move(file), info.assetName, info.assetName, e, {}, attachParams(),
					[this](const Input::Event &e)
					{
						app().launchSystem(e);
					});
			}
		}
	} {}

[[gnu::weak]] const BundledGameInfo &EmuSystem::bundledGameInfo(int) const
{
	static const BundledGameInfo info[]
	{
		{ "test", "test" }
	};

	return info[0];
}

}
