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
#include <emuframework/EmuSystem.hh>
#include <emuframework/FilePicker.hh>
#include <imagine/logger/logger.h>
#include <imagine/io/FileIO.hh>
#include <imagine/io/BufferMapIO.hh>
#include <imagine/fs/ArchiveFS.hh>

void loadGameCompleteFromRecentItem(uint result, Input::Event e);

BundledGamesView::BundledGamesView(Base::Window &win):
	TableView
	{
		"Bundled Games",
		win,
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
		[&info](TextMenuItem &t, View &, Input::Event ev)
		{
			auto file = openAppAssetIO(info.assetName);
			if(!file)
			{
				logErr("error opening bundled game asset: %s", info.assetName);
				return;
			}
			int loadGameRes;
			if(string_hasDotExtension(info.assetName, "lzma") || hasArchiveExtension(info.assetName))
			{
				ArchiveIO io{};
				std::error_code ec{};
				for(auto &entry : FS::ArchiveIterator{GenericIO{std::move(file)}, ec})
				{
					if(entry.type() == FS::file_type::directory)
					{
						continue;
					}
					auto name = entry.name();
					logMsg("archive file entry:%s", name);
					if(EmuSystem::defaultFsFilter(name))
					{
						io = entry.moveIO();
						break;
					}
				}
				if(ec)
				{
					logErr("error opening asset archive:%s", info.assetName);
					return;
				}
				if(!io)
				{
					logErr("no recognized file extensions in asset archive:%s", info.assetName);
					return;
				}
				loadGameRes = EmuSystem::loadGameFromIO(io, info.assetName, io.name());
			}
			else
			{
				loadGameRes = EmuSystem::loadGameFromIO(file, info.assetName, info.assetName);
			}
			file.close();
			if(loadGameRes == 1)
			{
				loadGameCompleteFromRecentItem(1, ev); // has same behavior as if loading from recent item
			}
			else if(loadGameRes == 0)
			{
				EmuSystem::clearGamePaths();
			}
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
