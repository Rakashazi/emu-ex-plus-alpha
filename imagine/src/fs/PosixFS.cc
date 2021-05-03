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

#define LOGTAG "PosixFS"
#include <imagine/fs/FS.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/utility.h>
#include <imagine/util/string.h>
#ifdef __APPLE__
#include <imagine/util/string/apple.h>
#endif
#include <errno.h>
#include <sys/stat.h>
#include <cstdlib>

namespace FS
{

static file_type makeDirType(int type)
{
	switch(type)
	{
		case DT_REG: return file_type::regular;
		case DT_DIR: return file_type::directory;
		case DT_LNK: return file_type::symlink;
		case DT_BLK: return file_type::block;
		case DT_CHR: return file_type::character;
		case DT_FIFO: return file_type::fifo;
		case DT_SOCK: return file_type::socket;
	}
	return file_type::unknown;
}

static bool isDotName(const char *name)
{
	return string_equal(name, ".") || string_equal(name, "..");
}

DirectoryEntryImpl::DirectoryEntryImpl(const char *path, std::error_code &ec):
	DirectoryEntryImpl{path, &ec}
{}

DirectoryEntryImpl::DirectoryEntryImpl(const char *path):
	DirectoryEntryImpl{path, nullptr}
{}

DirectoryEntryImpl::DirectoryEntryImpl(const char *path, std::error_code *ecPtr):
	dir{opendir(path)}
{
	if(!dir)
	{
		if(Config::DEBUG_BUILD)
			logErr("opendir(%s) error: %s", path, strerror(errno));
		if(ecPtr)
			*ecPtr = {errno, std::system_category()};
		return;
	}
	logMsg("opened directory:%s", path);
	if(ecPtr)
		ecPtr->clear();
	string_copy(basePath, path);
	readNextDir(); // go to first entry
}

bool DirectoryEntryImpl::readNextDir()
{
	if(!dir) [[unlikely]]
		return false;
	errno = 0;
	// clear cached types
	type_ = {};
	linkType_ = {};
	while((dirent_ = readdir(dir.get())))
	{
		//logMsg("reading entry:%s", dirent.d_name);
		if(!isDotName(dirent_->d_name))
		{
			#ifdef __APPLE__
			// Precompose all strings for text renderer
			// TODO: make optional when renderer supports decomposed unicode
			precomposeUnicodeString(dirent_->d_name, dirent_->d_name, NAME_MAX + 1);
			#endif
			return true; // got an entry
		}
	}
	// handle error or end of directory
	if(Config::DEBUG_BUILD && errno)
		logErr("readdir error: %s", strerror(errno));
	return false;
}

bool DirectoryEntryImpl::hasEntry() const
{
	return dirent_;
}

const char *DirectoryEntryImpl::name() const
{
	assumeExpr(dirent_);
	return dirent_->d_name;
}

file_type DirectoryEntryImpl::type() const
{
	assumeExpr(dirent_);
	if(type_ == file_type::none)
	{
		type_ = makeDirType(dirent_->d_type);
		if(type_ == file_type::unknown || type_ == file_type::symlink)
		{
			type_ = status(path()).type();
		}
	}
	return type_;
}

file_type DirectoryEntryImpl::symlink_type() const
{
	assumeExpr(dirent_);
	if(linkType_ == file_type::none)
	{
		linkType_ = makeDirType(dirent_->d_type);
		if(linkType_ == file_type::unknown)
		{
			logMsg("dir entry doesn't provide file type");
			linkType_ = symlink_status(path()).type();
		}
	}
	return linkType_;
}

PathStringImpl DirectoryEntryImpl::path() const
{
	return makePathStringPrintf("%s/%s", basePath.data(), name());
}

void DirectoryEntryImpl::close()
{
	if(dir)
	{
		logMsg("closing directory:%s", basePath.data());
	}
	dir.reset();
}

void DirectoryEntryImpl::closeDirectoryStream(DIR *dir)
{
	if(!dir)
		return;
	//logDMsg("closing dir:%p", dir);
	if(::closedir(dir) == -1 && Config::DEBUG_BUILD)
	{
		logErr("closedir(%p) error: %s", dir, strerror(errno));
	}
}

template <class... Args>
static std::shared_ptr<DirectoryEntryImpl> makeDirEntryPtr(Args&& ...args)
{
	DirectoryEntryImpl dirEntry{std::forward<Args>(args)...};
	if(dirEntry.hasEntry())
	{
		return std::make_shared<DirectoryEntryImpl>(std::move(dirEntry));
	}
	else
	{
		// empty directory
		return {};
	}
}

directory_iterator::directory_iterator(const char *path):
	impl{makeDirEntryPtr(path)}
{}

directory_iterator::directory_iterator(const char *path, std::error_code &ec):
	impl{makeDirEntryPtr(path, ec)}
{}

directory_iterator::~directory_iterator() {}

directory_entry& directory_iterator::operator*()
{
	return *impl;
}

directory_entry* directory_iterator::operator->()
{
	return impl.get();
}

void directory_iterator::operator++()
{
	assumeExpr(impl); // incrementing end-iterator is undefined
	if(!impl->readNextDir())
		impl.reset();
}

bool directory_iterator::operator==(directory_iterator const &rhs) const
{
	return impl == rhs.impl;
}

static file_type makeFileType(struct stat s)
{
	if(S_ISREG(s.st_mode))
		return file_type::regular;
	if(S_ISDIR(s.st_mode))
		return file_type::directory;
	if(S_ISBLK(s.st_mode))
		return file_type::block;
	if(S_ISCHR(s.st_mode))
		return file_type::character;
	if(S_ISFIFO(s.st_mode))
		return file_type::fifo;
	if(S_ISSOCK(s.st_mode))
		return file_type::socket;
	if(S_ISLNK(s.st_mode))
		return file_type::symlink;
	return file_type::unknown;
}

std::tm file_status::lastWriteTimeLocal() const
{
	std::tm localMTime;
	std::time_t mTime = lastWriteTime();
	if(!localtime_r(&mTime, &localMTime))
	{
		logErr("localtime_r failed");
		return {};
	}
	return localMTime;
}

PathString current_path()
{
	std::error_code dummy;
	return current_path(dummy);
}

PathString current_path(std::error_code &result)
{
	PathString wDir{};
	if(!getcwd(wDir.data(), sizeof(wDir)))
	{
		if(Config::DEBUG_BUILD)
			logErr("getcwd error: %s", strerror(errno));
		result = {errno, std::system_category()};
		return {};
	}
	#ifdef __APPLE__
	// Precompose all strings for text renderer
	// TODO: make optional when renderer supports decomposed unicode
	precomposeUnicodeString(wDir.data(), wDir.data(), sizeof(wDir));
	#endif
	result.clear();
	return wDir;
}

void current_path(const char *path)
{
	std::error_code dummy;
	current_path(path, dummy);
}

void current_path(const char *path, std::error_code &result)
{
	if(chdir(path) == -1)
	{
		if(Config::DEBUG_BUILD)
			logErr("chdir(%s) error: %s", path, strerror(errno));
		result = {errno, std::system_category()};
		return;
	}
	result.clear();
}

bool exists(const char *path)
{
	std::error_code dummy;
	return exists(path, dummy);
}

bool exists(const char *path, std::error_code &result)
{
	return access(path, acc::e, result);
}

std::uintmax_t file_size(const char *path)
{
	std::error_code dummy;
	return file_size(path, dummy);
}

std::uintmax_t file_size(const char *path, std::error_code &ec)
{
	auto s = status(path, ec);
	if(ec)
	{
		return -1;
	}
	if(s.type() != file_type::regular)
	{
		ec = {EINVAL, std::system_category()};
		return -1;
	}
	ec.clear();
	return s.size();
}

file_status status(const char *path)
{
	std::error_code dummy;
	return status(path, dummy);
}

file_status status(const char *path, std::error_code &result)
{
	struct stat s;
	if(stat(path, &s) == -1)
	{
		if(Config::DEBUG_BUILD)
			logErr("stat(%s) error: %s", path, strerror(errno));
		result = {errno, std::system_category()};
		if(result.value() == ENOENT)
			return {file_type::not_found, {}, {}};
		else
			return {};
	}
	result.clear();
	return {makeFileType(s), (std::uintmax_t)s.st_size, (file_time_type)s.st_mtime};
}

file_status symlink_status(const char *path)
{
	std::error_code dummy;
	return symlink_status(path, dummy);
}

file_status symlink_status(const char *path, std::error_code &result)
{
	struct stat s;
	if(lstat(path, &s) == -1)
	{
		if(Config::DEBUG_BUILD)
			logErr("lstat(%s) error: %s", path, strerror(errno));
		result = {errno, std::system_category()};
		if(result.value() == ENOENT)
			return {file_type::not_found, {}, {}};
		else
			return {};
	}
	result.clear();
	return {makeFileType(s), (std::uintmax_t)s.st_size, (file_time_type)s.st_mtime};
}

void chown(const char *path, uid_t owner, gid_t group)
{
	std::error_code dummy;
	return chown(path, owner, group, dummy);
}

void chown(const char *path, uid_t owner, gid_t group, std::error_code &result)
{
	if(::chown(path, owner, group) == -1)
	{
		if(Config::DEBUG_BUILD)
			logErr("chown(%s) error: %s", path, strerror(errno));
		result = {errno, std::system_category()};
		return;
	}
	result.clear();
}

bool access(const char *path, acc type)
{
	std::error_code dummy;
	return access(path, type, dummy);
}

bool access(const char *path, acc type, std::error_code &result)
{
	result.clear();
	if(::access(path, (int)type) == -1)
	{
		if(errno != ENOENT)
		{
			if(Config::DEBUG_BUILD)
				logErr("access(%s) error: %s", path, strerror(errno));
			result = {errno, std::system_category()};
		}
		return false;
	}
	logMsg("file exists:%s", path);
	return true;
}

bool remove(const char *path)
{
	std::error_code dummy;
	return remove(path, dummy);
}

bool remove(const char *path, std::error_code &result)
{
	logErr("removing: %s", path);
	if(::unlink(path) == -1)
	{
		if(Config::DEBUG_BUILD)
			logErr("unlink(%s) error: %s", path, strerror(errno));
		result = {errno, std::system_category()};
		return false;
	}
	result.clear();
	return true;
}

bool create_directory(const char *path)
{
	std::error_code dummy;
	return create_directory(path, dummy);
}

bool create_directory(const char *path, std::error_code &result)
{
	const mode_t defaultOpenMode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
	if(::mkdir(path, defaultOpenMode) == -1)
	{
		if(Config::DEBUG_BUILD)
			logErr("mkdir(%s) error: %s", path, strerror(errno));
		result = {errno, std::system_category()};
		return false;
	}
	result.clear();
	return true;
}

void rename(const char *oldPath, const char *newPath)
{
	std::error_code dummy;
	return rename(oldPath, newPath, dummy);
}

void rename(const char *oldPath, const char *newPath, std::error_code &result)
{
	if(::rename(oldPath, newPath) == -1)
	{
		if(Config::DEBUG_BUILD)
			logErr("rename(%s, %s) error: %s", oldPath, newPath, strerror(errno));
		result = {errno, std::system_category()};
	}
	result.clear();
}

}
