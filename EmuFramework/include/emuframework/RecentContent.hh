#pragma once

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

#include <imagine/fs/FSDefs.hh>
#include <vector>
#include <string_view>

namespace IG
{
class ApplicationContext;
class FileIO;
class MapIO;
}

namespace EmuEx
{

class EmuSystem;

using namespace IG;

struct RecentContentInfo
{
	FS::PathString path;
	FS::FileString name;

	constexpr bool operator==(RecentContentInfo const& rhs) const
	{
		return path == rhs.path;
	}
};

class RecentContent
{
public:
	void add(std::string_view path, std::string_view name);
	void add(const EmuSystem &);
	size_t size() const { return recentContentList.size(); }
	auto begin() const { return recentContentList.begin(); }
	auto end() const { return recentContentList.end(); }
	void clear() { recentContentList.clear(); }
	void writeConfig(FileIO &) const;
	bool readConfig(MapIO &, unsigned key, const EmuSystem &);

private:
	std::vector<RecentContentInfo> recentContentList;
public:
	static constexpr uint8_t defaultMaxRecentContent{20};
	uint8_t maxRecentContent{defaultMaxRecentContent};
};

}
