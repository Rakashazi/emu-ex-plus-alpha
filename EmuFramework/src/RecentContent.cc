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

#include "EmuOptions.hh"
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

bool RecentContent::readConfig(MapIO &io, unsigned key, size_t size, const EmuSystem &system)
{
	if(key == CFGKEY_MAX_RECENT_CONTENT)
	{
		readOptionValue(io, size, maxRecentContent);
		return true;
	}
	else if(key == CFGKEY_RECENT_CONTENT_V2)
	{
		auto len = io.get<uint16_t>();
		FS::PathString path;
		auto bytesRead = io.readSized(path, len);
		if(bytesRead == -1)
		{
			log.error("error reading string option");
			return true;
		}
		if(!bytesRead)
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

bool RecentContent::readLegacyConfig(MapIO &io, const EmuSystem &system)
{
	log.info("reading legacy config");
	auto readSize = io.size();
	while(readSize)
	{
		if(readSize < 2)
		{
			log.info("expected string length but only {} bytes left", readSize);
			return false;
		}

		auto len = io.get<uint16_t>();
		readSize -= 2;

		if(len > readSize)
		{
			log.info("string length {} longer than {} bytes left", len, readSize);
			return false;
		}

		FS::PathString path;
		auto bytesRead = io.readSized(path, len);
		if(bytesRead == -1)
		{
			log.error("error reading string option");
			return false;
		}
		if(!bytesRead)
			continue; // don't add empty paths
		readSize -= len;
		auto displayName = system.contentDisplayNameForPath(path);
		if(displayName.empty())
		{
			log.info("skipping missing recent content:{}", path);
			continue;
		}
		RecentContentInfo info{path, displayName};
		const auto &added = recentContentList.emplace_back(info);
		log.info("added game to recent list:{}, name:{}", added.path, added.name);
	}
	return true;
}

}
