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

#define LOGTAG "AAssetFS"
#include <imagine/fs/AAssetFS.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>
#include <android/asset_manager.h>

namespace IG::FS
{

AAssetDirectory::AAssetDirectory(AAssetManager *mgr, CStringView path):
	dir{AAssetManager_openDir(mgr, path)}
{
	if(!dir) [[unlikely]]
	{
		if(Config::DEBUG_BUILD)
			logErr("AAssetManager_openDir(%p, %s) error", mgr, path.data());
		throw std::runtime_error{std::format("error opening asset directory: {}", path)};
	}
	logMsg("opened asset directory:%s", path.data());
	basePath = path;
	readNextDir(); // go to first entry
}

bool AAssetDirectory::readNextDir()
{
	if(!dir) [[unlikely]]
		return false;
	entryName = AAssetDir_getNextFileName(dir.get());
	if(!entryName)
	{
		logMsg("no more filenames in directory");
		return false;
	}
	return true;
}

bool AAssetDirectory::hasEntry() const
{
	return entryName;
}

std::string_view AAssetDirectory::name() const
{
	assumeExpr(entryName);
	return entryName;
}

PathString AAssetDirectory::path() const
{
	return pathString(basePath, name());
}

void AAssetDirectory::closeAAssetDir(AAssetDir *ptr)
{
	AAssetDir_close(ptr);
}

template <class... Args>
static std::shared_ptr<AAssetDirectory> makeDirEntryPtr(Args&& ...args)
{
	AAssetDirectory dirEntry{std::forward<Args>(args)...};
	if(dirEntry.hasEntry())
	{
		return std::make_shared<AAssetDirectory>(std::move(dirEntry));
	}
	else
	{
		// empty directory
		return {};
	}
}

AAssetIterator::AAssetIterator(AAssetManager *mgr, CStringView path):
	impl{makeDirEntryPtr(mgr, path)} {}

AAssetDirectory& AAssetIterator::operator*()
{
	return *impl;
}

AAssetDirectory* AAssetIterator::operator->()
{
	return impl.get();
}

void AAssetIterator::operator++()
{
	assumeExpr(impl); // incrementing end-iterator is undefined
	if(!impl->readNextDir())
		impl.reset();
}

bool AAssetIterator::operator==(AAssetIterator const &rhs) const
{
	return impl == rhs.impl;
}

}
