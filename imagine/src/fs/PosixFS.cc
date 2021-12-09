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
#ifdef __APPLE__
#include <imagine/util/string/apple.h>
#endif
#include <errno.h>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>
#include <system_error>

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

static bool isDotName(std::string_view name)
{
	return name == "." || name == "..";
}

DirectoryEntryImpl::DirectoryEntryImpl(IG::CStringView path):
	dir{opendir(path)}
{
	if(!dir) [[unlikely]]
	{
		if(Config::DEBUG_BUILD)
			logErr("opendir(%s) error:%s", path.data(), strerror(errno));
		throw std::system_error{errno, std::system_category(), path};
	}
	logMsg("opened directory:%s", path.data());
	basePath = path;
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

std::string_view DirectoryEntryImpl::name() const
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

PathString DirectoryEntryImpl::path() const
{
	return FS::pathString(basePath, name());
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
	//logDMsg("closing dir:%p", dir);
	if(::closedir(dir) == -1 && Config::DEBUG_BUILD) [[unlikely]]
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

directory_iterator::directory_iterator(IG::CStringView path):
	impl{makeDirEntryPtr(path)}
{}

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

static std::string formatDateAndTime(std::tm time)
{
	std::array<char, 64> str{};
	static constexpr const char *strftimeFormat = "%x  %r";
	std::strftime(str.data(), str.size(), strftimeFormat, &time);
	return str.data();
}

std::tm file_status::lastWriteTimeLocal() const
{
	std::tm localMTime;
	std::time_t mTime = lastWriteTime();
	if(!localtime_r(&mTime, &localMTime)) [[unlikely]]
	{
		logErr("localtime_r failed");
		return {};
	}
	return localMTime;
}

std::string formatLastWriteTimeLocal(IG::CStringView path)
{
	return formatDateAndTime(status(path).lastWriteTimeLocal());
}

PathString current_path()
{
	PathStringArray wDir;
	if(!getcwd(wDir.data(), sizeof(wDir))) [[unlikely]]
	{
		if(Config::DEBUG_BUILD)
			logErr("getcwd error:%s", strerror(errno));
		return {};
	}
	#ifdef __APPLE__
	// Precompose all strings for text renderer
	// TODO: make optional when renderer supports decomposed unicode
	precomposeUnicodeString(wDir.data(), wDir.data(), sizeof(wDir));
	#endif
	return wDir.data();
}

void current_path(IG::CStringView path)
{
	if(chdir(path) == -1) [[unlikely]]
	{
		if(Config::DEBUG_BUILD)
			logErr("chdir(%s) error:%s", path.data(), strerror(errno));
	}
}

bool exists(IG::CStringView path)
{
	return access(path, acc::e);
}

std::uintmax_t file_size(IG::CStringView path)
{
	auto s = status(path);
	if(s.type() != file_type::regular)
	{
		return -1;
	}
	return s.size();
}

file_status status(IG::CStringView path)
{
	struct stat s;
	if(stat(path, &s) == -1)
	{
		if(Config::DEBUG_BUILD)
			logErr("stat(%s) error:%s", path.data(), strerror(errno));
		if(errno == ENOENT)
			return {file_type::not_found, {}, {}};
		else
			return {};
	}
	return {makeFileType(s), (std::uintmax_t)s.st_size, (file_time_type)s.st_mtime};
}

file_status symlink_status(IG::CStringView path)
{
	struct stat s;
	if(lstat(path, &s) == -1)
	{
		if(Config::DEBUG_BUILD)
			logErr("lstat(%s) error:%s", path.data(), strerror(errno));
		if(errno == ENOENT)
			return {file_type::not_found, {}, {}};
		else
			return {};
	}
	return {makeFileType(s), (std::uintmax_t)s.st_size, (file_time_type)s.st_mtime};
}

void chown(IG::CStringView path, uid_t owner, gid_t group)
{
	if(::chown(path, owner, group) == -1) [[unlikely]]
	{
		if(Config::DEBUG_BUILD)
			logErr("chown(%s) error:%s", path.data(), strerror(errno));
		return;
	}
}

bool access(IG::CStringView path, acc type)
{
	if(::access(path, (int)type) == -1)
	{
		if(errno != ENOENT) [[unlikely]]
		{
			if(Config::DEBUG_BUILD)
				logErr("access(%s) error:%s", path.data(), strerror(errno));
		}
		return false;
	}
	logMsg("file exists:%s", path.data());
	return true;
}

bool remove(IG::CStringView path)
{
	logErr("removing:%s", path.data());
	if(::unlink(path) == -1) [[unlikely]]
	{
		if(Config::DEBUG_BUILD)
			logErr("unlink(%s) error:%s", path.data(), strerror(errno));
		return false;
	}
	return true;
}

bool create_directory(IG::CStringView path)
{
	const mode_t defaultOpenMode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
	if(::mkdir(path, defaultOpenMode) == -1)
	{
		auto err = errno;
		if(err == EEXIST)
		{
			return false;
		}
		else [[unlikely]]
		{
			if(Config::DEBUG_BUILD)
				logErr("mkdir(%s) error:%s", path.data(), strerror(err));
			throw std::system_error(err, std::system_category(), path);
		}
	}
	return true;
}

void rename(IG::CStringView oldPath, IG::CStringView newPath)
{
	if(::rename(oldPath, newPath) == -1) [[unlikely]]
	{
		if(Config::DEBUG_BUILD)
			logErr("rename(%s, %s) error:%s", oldPath.data(), newPath.data(), strerror(errno));
	}
}

}
