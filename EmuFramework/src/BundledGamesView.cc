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

BundledGamesView::BundledGamesView(ViewAttachParams attach):
	TableView
	{
		"Bundled Games",
		attach,
		[this](const TableView &)
		{
			return 1;
		},
		[this](const TableView &, uint idx) -> MenuItem&
		{
			return game[0];
		}
	}
{
	auto &info = EmuSystem::bundledGameInfo(0);
	game[0] = {info.displayName,
		[&info](Input::Event e)
		{
			auto file = EmuApp::openAppAssetIO(info.assetName, IO::AccessHint::ALL);
			if(!file)
			{
				logErr("error opening bundled game asset: %s", info.assetName);
				return;
			}
			EmuApp::createSystemWithMedia(file.makeGeneric(), "", info.assetName, e, {},
				[](Input::Event e)
				{
					EmuApp::launchSystemWithResumePrompt(e, false);
				});
		}};
}

[[gnu::weak]] const BundledGameInfo &EmuSystem::bundledGameInfo(uint idx)
{
	static const BundledGameInfo info[]
	{
		{ "test", "test" }
	};

	return info[0];
}
