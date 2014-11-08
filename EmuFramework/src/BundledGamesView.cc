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
#include <imagine/io/FileIO.hh>
#include <imagine/io/BufferMapIO.hh>
#include <unzip.h>

void loadGameCompleteFromRecentItem(uint result, const Input::Event &e);

void BundledGamesView::init(bool highlightFirst)
{
	uint i = 0;
	auto &info = EmuSystem::bundledGameInfo(0);
	game[0].init(info.displayName); item[i++] = &game[0];
	game[0].onSelect() =
		[&info](TextMenuItem &t, View &, const Input::Event &ev)
		{
			#if defined __ANDROID__ || defined CONFIG_MACHINE_PANDORA
			auto file = openAppAssetIO(info.assetName);
			if(!file)
			{
				logErr("error opening bundled game asset: %s", info.assetName);
				return;
			}
			auto res = EmuSystem::loadGameFromIO(file, info.assetName);
			file.close();
			#else
			auto zipPath = makeFSPathStringPrintf("%s/%s", Base::assetPath(), info.assetName);
			auto zip = unzOpen(zipPath.data());
			if(!zip)
			{
				logErr("error opening bundled game asset: %s", info.assetName);
				return;
			}
			unzGoToFirstFile(zip);
			unz_file_info zipInfo;
			unzGetCurrentFileInfo(zip, &zipInfo, 0, 0, 0, 0, 0, 0);
			unzOpenCurrentFile(zip);
			auto size = zipInfo.uncompressed_size;
			auto buf = mem_alloc(size);
			unzReadCurrentFile(zip, buf, size);
			unzCloseCurrentFile(zip);
			unzClose(zip);
			BufferMapIO io;
			io.open(buf, size, [buf](BufferMapIO &) { mem_free(buf); });
			auto res = EmuSystem::loadGameFromIO(io, info.assetName);
			#endif
			if(res == 1)
			{
				loadGameCompleteFromRecentItem(1, ev); // has same behavior as if loading from recent item
			}
			else if(res == 0)
			{
				EmuSystem::clearGamePaths();
			}
		};
	assert(i <= sizeofArray(item));
	TableView::init(item, i, highlightFirst);
}

[[gnu::weak]] const BundledGameInfo &EmuSystem::bundledGameInfo(uint idx)
{
	static const BundledGameInfo info[]
	{
		{ "test", "test" }
	};

	return info[0];
}
