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
#include <cstdlib>
#include <imagine/fs/FS.hh>
#include <imagine/logger/logger.h>
#include <imagine/base/Base.hh>
#include <imagine/util/strings.h>
#include <imagine/util/string/generic.h>
#include <imagine/util/assume.h>
#include <errno.h>

#ifdef __APPLE__
#include <imagine/util/string/apple.h>
#endif

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

const char *DirectoryEntryImpl::name() const
{
	assumeExpr(dirent_);
	return dirent_->d_name;
}

file_type DirectoryEntryImpl::type()
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

file_type DirectoryEntryImpl::symlink_type()
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

static bool isDotName(const char *name)
{
	return string_equal(name, ".") || string_equal(name, "..");
}

void DirectoryIteratorImpl::init(const char *path, CallResult &result)
{
	DIR *d = opendir(path);
	if(!d)
	{
		logErr("opendir(%s) error: %s", path, strerror(errno));
		result = BAD_STATE;
		switch(errno)
		{
			bcase EACCES: result = PERMISSION_DENIED;
			bcase ENOENT: result = NOT_FOUND;
			bcase ENOMEM: result = OUT_OF_MEMORY;
			bcase ENOTDIR: result = INVALID_PARAMETER;
		}
		return;
	}
	logMsg("opened directory:%s", path);
	result = OK;
	dir = {d,
		[](DIR *dir)
		{
			logMsg("closing directory");
			closedir(dir);
		}
	};
	string_copy(entry.basePath, path);
	++(*static_cast<directory_iterator*>(this)); // go to first entry
}

directory_iterator::directory_iterator(const char *path)
{
	CallResult dummy;
	init(path, dummy);
}

directory_iterator::directory_iterator(const char *path, CallResult &result)
{
	init(path, result);
}

directory_iterator::~directory_iterator() {}

directory_entry& directory_iterator::operator*()
{
	return entry;
}

directory_entry* directory_iterator::operator->()
{
	return &entry;
}

void directory_iterator::operator++()
{
	assumeExpr(dir); // incrementing end-iterator is undefined
	int ret = 0;
	auto &dirent = entry.dirent_;
	errno = 0;
	while((dirent = readdir(dir.get())))
	{
		//logMsg("reading entry:%s", dirent.d_name);
		if(!isDotName(dirent->d_name))
		{
			// clear cached types
			entry.type_ = {};
			entry.linkType_ = {};
			#ifdef __APPLE__
			// Precompose all strings for text renderer
			// TODO: make optional when renderer supports decomposed unicode
			precomposeUnicodeString(dirent->d_name, dirent->d_name, NAME_MAX + 1);
			#endif
			return; // got an entry
		}
	}
	// handle error or end of directory
	if(Config::DEBUG_BUILD && errno)
		logErr("readdir error: %s", strerror(errno));
	dir = nullptr;
}

bool directory_iterator::operator==(directory_iterator const &rhs) const
{
	return dir == rhs.dir;
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

std::tm file_status::last_write_time_local() const
{
	std::tm localMTime;
	std::time_t mTime = last_write_time();
	if(!localtime_r(&mTime, &localMTime))
	{
		logErr("localtime_r failed");
		return {};
	}
	return localMTime;
}

PathString current_path()
{
	CallResult dummy;
	return current_path(dummy);
}

PathString current_path(CallResult &result)
{
	PathString wDir{};
	if(!getcwd(wDir.data(), sizeof(wDir)))
	{
		if(Config::DEBUG_BUILD)
			logErr("getcwd error: %s", strerror(errno));
		result = BAD_STATE;
		switch(errno)
		{
			bcase EACCES: result = PERMISSION_DENIED;
			bcase ENOENT: result = NOT_FOUND;
		}
		return {};
	}
	#ifdef __APPLE__
	// Precompose all strings for text renderer
	// TODO: make optional when renderer supports decomposed unicode
	precomposeUnicodeString(wDir.data(), wDir.data(), sizeof(wDir));
	#endif
	result = OK;
	return wDir;
}

void current_path(const char *path)
{
	CallResult dummy;
	current_path(path, dummy);
}

void current_path(const char *path, CallResult &result)
{
	if(chdir(path) == -1)
	{
		if(Config::DEBUG_BUILD)
			logErr("chdir(%s) error: %s", path, strerror(errno));
		result = BAD_STATE;
		switch(errno)
		{
			bcase EACCES: result = PERMISSION_DENIED;
			bcase EFAULT: result = INVALID_PARAMETER;
			bcase EIO: result = IO_ERROR;
			bcase ELOOP: result = INVALID_PARAMETER;
			bcase ENAMETOOLONG: result = INVALID_PARAMETER;
			bcase ENOENT: result = NOT_FOUND;
			bcase ENOMEM: result = OUT_OF_MEMORY;
			bcase ENOTDIR: result = INVALID_PARAMETER;
		}
		return;
	}
	result = OK;
}

bool exists(const char *path)
{
	CallResult dummy;
	return exists(path, dummy);
}

bool exists(const char *path, CallResult &result)
{
	return access(path, acc::e, result);
}

std::uintmax_t file_size(const char *path)
{
	CallResult dummy;
	return file_size(path, dummy);
}

std::uintmax_t file_size(const char *path, CallResult &result)
{
	auto s = status(path, result);
	if(result != OK)
	{
		return 0;
	}
	if(s.type() != file_type::regular)
	{
		result = INVALID_PARAMETER;
		return 0;
	}
	return s.size();
}

file_status status(const char *path)
{
	CallResult dummy;
	return status(path, dummy);
}

file_status status(const char *path, CallResult &result)
{
	struct stat s;
	if(stat(path, &s) == -1)
	{
		if(Config::DEBUG_BUILD)
			logErr("stat(%s) error: %s", path, strerror(errno));
		result = BAD_STATE;
		switch(errno)
		{
			bcase EACCES: result = PERMISSION_DENIED;
			bcase EFAULT: result = INVALID_PARAMETER;
			bcase ELOOP: result = INVALID_PARAMETER;
			bcase ENAMETOOLONG: result = INVALID_PARAMETER;
			bcase ENOENT: result = NOT_FOUND;
			bcase ENOMEM: result = OUT_OF_MEMORY;
			bcase ENOTDIR: result = INVALID_PARAMETER;
		}
		if(result == NOT_FOUND)
			return {file_type::not_found, {}, {}};
		else
			return {};
	}
	result = OK;
	return {makeFileType(s), (std::uintmax_t)s.st_size, (file_time_type)s.st_mtime};
}

file_status symlink_status(const char *path)
{
	CallResult dummy;
	return symlink_status(path, dummy);
}

file_status symlink_status(const char *path, CallResult &result)
{
	struct stat s;
	if(lstat(path, &s) == -1)
	{
		if(Config::DEBUG_BUILD)
			logErr("lstat(%s) error: %s", path, strerror(errno));
		result = BAD_STATE;
		switch(errno)
		{
			bcase EACCES: result = PERMISSION_DENIED;
			bcase EFAULT: result = INVALID_PARAMETER;
			bcase ELOOP: result = INVALID_PARAMETER;
			bcase ENAMETOOLONG: result = INVALID_PARAMETER;
			bcase ENOENT: result = NOT_FOUND;
			bcase ENOMEM: result = OUT_OF_MEMORY;
			bcase ENOTDIR: result = INVALID_PARAMETER;
		}
		if(result == NOT_FOUND)
			return {file_type::not_found, {}, {}};
		else
			return {};
	}
	result = OK;
	return {makeFileType(s), (std::uintmax_t)s.st_size, (file_time_type)s.st_mtime};
}

void chown(const char *path, uid_t owner, gid_t group)
{
	CallResult dummy;
	return chown(path, owner, group, dummy);
}

void chown(const char *path, uid_t owner, gid_t group, CallResult &result)
{
	if(::chown(path, owner, group) == -1)
	{
		if(Config::DEBUG_BUILD)
			logErr("chown(%s) error: %s", path, strerror(errno));
		result = BAD_STATE;
		switch(errno)
		{
			bcase EACCES: result = PERMISSION_DENIED;
			bcase EFAULT: result = INVALID_PARAMETER;
			bcase ELOOP: result = INVALID_PARAMETER;
			bcase ENAMETOOLONG: result = INVALID_PARAMETER;
			bcase ENOENT: result = NOT_FOUND;
			bcase ENOMEM: result = OUT_OF_MEMORY;
			bcase ENOTDIR: result = INVALID_PARAMETER;
			bcase EPERM: result = PERMISSION_DENIED;
			bcase EROFS: result = PERMISSION_DENIED;
		}
		return;
	}
	result = OK;
}

bool access(const char *path, acc type)
{
	CallResult dummy;
	return access(path, type, dummy);
}

bool access(const char *path, acc type, CallResult &result)
{
	if(::access(path, (int)type) == -1)
	{
		if(Config::DEBUG_BUILD)
			logErr("access(%s) error: %s", path, strerror(errno));
		result = BAD_STATE;
		switch(errno)
		{
			bcase EACCES: result = PERMISSION_DENIED;
			bcase EFAULT: result = INVALID_PARAMETER;
			bcase EINVAL: result = INVALID_PARAMETER;
			bcase ELOOP: result = INVALID_PARAMETER;
			bcase ENAMETOOLONG: result = INVALID_PARAMETER;
			bcase ENOENT: result = NOT_FOUND;
			bcase EIO: result = IO_ERROR;
			bcase ENOMEM: result = OUT_OF_MEMORY;
			bcase ENOTDIR: result = INVALID_PARAMETER;
			bcase EROFS: result = PERMISSION_DENIED;
			bcase ETXTBSY: result = PERMISSION_DENIED;
		}
		return false;
	}
	result = OK;
	return true;
}

bool remove(const char *path)
{
	CallResult dummy;
	return remove(path, dummy);
}

bool remove(const char *path, CallResult &result)
{
	logErr("removing: %s", path);
	if(::unlink(path) == -1)
	{
		if(Config::DEBUG_BUILD)
			logErr("unlink(%s) error: %s", path, strerror(errno));
		result = BAD_STATE;
		switch(errno)
		{
			bcase EACCES: result = PERMISSION_DENIED;
			bcase EBUSY: result = IO_ERROR;
			bcase EFAULT: result = INVALID_PARAMETER;
			bcase ELOOP: result = INVALID_PARAMETER;
			bcase ENAMETOOLONG: result = INVALID_PARAMETER;
			bcase ENOENT: result = NOT_FOUND;
			bcase EIO: result = IO_ERROR;
			bcase ENOMEM: result = OUT_OF_MEMORY;
			bcase ENOTDIR: result = INVALID_PARAMETER;
			bcase EPERM: result = PERMISSION_DENIED;
			bcase EROFS: result = PERMISSION_DENIED;
		}
		return false;
	}
	result = OK;
	return true;
}

bool create_directory(const char *path)
{
	CallResult dummy;
	return create_directory(path, dummy);
}

bool create_directory(const char *path, CallResult &result)
{
	const mode_t defaultOpenMode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
	if(::mkdir(path, defaultOpenMode) == -1)
	{
		if(Config::DEBUG_BUILD)
			logErr("mkdir(%s) error: %s", path, strerror(errno));
		result = BAD_STATE;
		switch(errno)
		{
			bcase EACCES: result = PERMISSION_DENIED;
			bcase EEXIST: result = ALREADY_EXISTS;
			bcase ELOOP: result = INVALID_PARAMETER;
			bcase EMLINK: result = INVALID_PARAMETER;
			bcase ENAMETOOLONG: result = INVALID_PARAMETER;
			bcase ENOENT: result = NOT_FOUND;
			bcase ENOSPC: result = NO_FREE_ENTRIES;
			bcase ENOTDIR: result = INVALID_PARAMETER;
			bcase EROFS: result = PERMISSION_DENIED;
		}
		return false;
	}
	result = OK;
	return true;
}

void rename(const char *oldPath, const char *newPath)
{
	CallResult dummy;
	return rename(oldPath, newPath, dummy);
}

void rename(const char *oldPath, const char *newPath, CallResult &result)
{
	if(::rename(oldPath, newPath) == -1)
	{
		if(Config::DEBUG_BUILD)
			logErr("rename(%s, %s) error: %s", oldPath, newPath, strerror(errno));
		switch(errno)
		{
			bcase EACCES: result = PERMISSION_DENIED;
			bcase EBUSY: result = IO_ERROR;
			bcase EDQUOT: result = NO_FREE_ENTRIES;
			bcase EFAULT: result = INVALID_PARAMETER;
			bcase EINVAL: result = INVALID_PARAMETER;
			bcase EISDIR: result = INVALID_PARAMETER;
			bcase ELOOP: result = INVALID_PARAMETER;
			bcase EMLINK: result = INVALID_PARAMETER;
			bcase ENAMETOOLONG: result = INVALID_PARAMETER;
			bcase ENOENT: result = NOT_FOUND;
			bcase ENOSPC: result = NO_FREE_ENTRIES;
			bcase ENOTDIR: result = INVALID_PARAMETER;
			bcase EEXIST: result = ALREADY_EXISTS;
			bcase EROFS: result = PERMISSION_DENIED;
		}
	}
	result = OK;
}

}
