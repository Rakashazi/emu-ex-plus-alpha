#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/fs/FS.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/util/string/CStringView.hh>
#include <string>

namespace IG::FS
{

inline PathString createDirectoryUriSegments(ApplicationContext ctx, ConvertibleToPathString auto &&base, auto &&...components)
{
	if(!isUri(base))
		return createDirectorySegments(IG_forward(base), IG_forward(components)...);
	PathString uri{IG_forward(base)};
	([&]()
	{
		uri += "%2F";
		uri += encodeUri<PathString>(IG_forward(components));
		ctx.createDirectoryUri(uri);
	}(), ...);
	return uri;
}

size_t directoryItems(CStringView path);
bool forEachInDirectory(CStringView path, DirectoryEntryDelegate del, DirOpenFlags flags = {});
std::string formatLastWriteTimeLocal(ApplicationContext, CStringView path);

}
