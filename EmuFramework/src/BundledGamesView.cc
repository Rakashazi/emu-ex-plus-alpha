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
#include <emuframework/FilePicker.hh>
#include <imagine/logger/logger.h>
#include <imagine/io/FileIO.hh>
#include <imagine/io/BufferMapIO.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/base/ApplicationContext.hh>

BundledGamesView::BundledGamesView(ViewAttachParams attach):
	TableView
	{
		"Bundled Games",
		attach,
		[this](const TableView &)
		{
			return 1;
		},
		[this](const TableView &, unsigned idx) -> MenuItem&
		{
			return game[0];
		}
	}
{
	auto &info = EmuSystem::bundledGameInfo(0);
	game[0] = {info.displayName, &defaultFace(),
		[this, &info](Input::Event e)
		{
			auto file = app().openAppAssetIO(appContext(), info.assetName, IO::AccessHint::ALL);
			if(!file)
			{
				logErr("error opening bundled game asset: %s", info.assetName);
				return;
			}
			app().createSystemWithMedia(file.makeGeneric(), "", info.assetName, e, {}, attachParams(),
				[this](Input::Event e)
				{
					app().launchSystemWithResumePrompt(e, false);
				});
		}};
}

[[gnu::weak]] const BundledGameInfo &EmuSystem::bundledGameInfo(unsigned idx)
{
	static const BundledGameInfo info[]
	{
		{ "test", "test" }
	};

	return info[0];
}
