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

#include <emuframework/EmuOptions.hh>
#include <emuframework/RecentContent.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/Option.hh>
#include <imagine/io/MapIO.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"RecentContent"};

void RecentContent::add(std::string_view fullPath, std::string_view name)
{
	if(fullPath.empty())
		return;
	log.info("adding {} @ {} to recent list, current size:{}", name, fullPath, recentContentList.size());
	RecentContentInfo recent{FS::PathString{fullPath}, FS::FileString{name}};
	eraseFirst(recentContentList, recent); // remove existing entry so it's added to the front
	recentContentList.insert(recentContentList.begin(), recent);
	if(recentContentList.size() > maxRecentContent)
		recentContentList.resize(maxRecentContent);

	/*log.info("list contents:");
	for(auto &e : recentContentList)
	{
		log.info("path: {} name: {}", e.path, e.name);
	}*/
}

void RecentContent::add(const EmuSystem &system)
{
	add(system.contentLocation(), system.contentDisplayName());
}

void RecentContent::writeConfig(FileIO &io) const
{
	writeOptionValueIfNotDefault(io, CFGKEY_MAX_RECENT_CONTENT, maxRecentContent, defaultMaxRecentContent);
	log.info("writing recent content list");
	for(const auto &e : recentContentList)
	{
		auto size = 2 + e.path.size();
		writeOptionValueHeader(io, CFGKEY_RECENT_CONTENT_V2, size);
		io.put(uint16_t(e.path.size()));
		io.write(e.path.data(), e.path.size());
	}
}

bool RecentContent::readConfig(MapIO &io, unsigned key, const EmuSystem &system)
{
	if(key == CFGKEY_MAX_RECENT_CONTENT)
	{
		readOptionValue(io, maxRecentContent);
		return true;
	}
	else if(key == CFGKEY_RECENT_CONTENT_V2)
	{
		FS::PathString path;
		if(readSizedData<uint16_t>(io, path) == -1)
		{
			log.error("error reading string option");
			return true;
		}
		if(path.empty())
			return true; // don't add empty paths
		auto displayName = system.contentDisplayNameForPath(path);
		if(displayName.empty())
		{
			log.info("skipping missing recent content:{}", path);
			return true;
		}
		RecentContentInfo info{path, displayName};
		const auto &added = recentContentList.emplace_back(info);
		log.info("added game to recent list:{}, name:{}", added.path, added.name);
		return true;
	}
	return false;
}

}
