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
#include <cerrno>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>
#include <system_error>

namespace IG::FS
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

DirectoryStream::DirectoryStream(CStringView path, DirOpenFlags flags):
	dir{opendir(path)}
{
	if(!dir)
	{
		if(Config::DEBUG_BUILD)
			logErr("opendir(%s) error:%s", path.data(), strerror(errno));
		if(flags.test)
			return;
		else
			throw std::system_error{errno, std::generic_category(), path};
	}
	logMsg("opened directory:%s", path.data());
	basePath = path;
	readNextDir(); // go to first entry
}

bool DirectoryStream::readNextDir()
{
	if(!dir) [[unlikely]]
		return false;
	errno = 0;
	struct dirent *ent{};
	while((ent = readdir(dir.get())))
	{
		//logMsg("reading entry:%s", dirent.d_name);
		if(!isDotName(ent->d_name))
		{
			#ifdef __APPLE__
			// Precompose all strings for text renderer
			// TODO: make optional when renderer supports decomposed unicode
			precomposeUnicodeString(ent->d_name, ent->d_name, NAME_MAX + 1);
			#endif
			entry_ = {pathString(basePath, ent->d_name), ent};
			return true; // got an entry
		}
	}
	// handle error or end of directory
	if(Config::DEBUG_BUILD && errno)
		logErr("readdir error: %s", strerror(errno));
	entry_ = {};
	return false;
}

bool DirectoryStream::hasEntry() const
{
	return (bool)entry_;
}

void DirectoryStream::closeDirectoryStream(DIR *dir)
{
	//logDMsg("closing dir:%p", dir);
	auto dirAddr = (size_t)dir;
	if(::closedir(dir) == -1 && Config::DEBUG_BUILD) [[unlikely]]
	{
		logErr("closedir(0x%zX) error: %s", dirAddr, strerror(errno));
	}
}

std::string_view directory_entry::name() const
{
	if(name_.size())
	{
		return name_;
	}
	else
	{
		assumeExpr(dirent_);
		return dirent_->d_name;
	}
}

file_type directory_entry::type() const
{
	if(type_ == file_type::none)
	{
		assumeExpr(dirent_);
		type_ = makeDirType(dirent_->d_type);
		if(type_ == file_type::unknown || type_ == file_type::symlink)
		{
			type_ = status(path_).type();
		}
	}
	return type_;
}

file_type directory_entry::symlink_type() const
{
	if(linkType_ == file_type::none)
	{
		linkType_ = type();
		if(linkType_ == file_type::symlink)
		{
			logMsg("checking symlink type for:%s", path_.data());
			linkType_ = symlink_status(path_).type();
		}
	}
	return linkType_;
}

static std::shared_ptr<DirectoryStream> makeDirectoryStream(CStringView path)
{
	auto streamPtr = std::make_shared<DirectoryStream>(path);
	return streamPtr->hasEntry() ? streamPtr : nullptr;
}

directory_iterator::directory_iterator(CStringView path):
	impl{makeDirectoryStream(path)} {}

directory_entry& directory_iterator::operator*()
{
	return impl->entry();
}

directory_entry* directory_iterator::operator->()
{
	return &impl->entry();
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

void current_path(CStringView path)
{
	if(chdir(path) == -1) [[unlikely]]
	{
		if(Config::DEBUG_BUILD)
			logErr("chdir(%s) error:%s", path.data(), strerror(errno));
	}
}

bool exists(CStringView path)
{
	return access(path, acc::e);
}

std::uintmax_t file_size(CStringView path)
{
	auto s = status(path);
	if(s.type() != file_type::regular)
	{
		return -1;
	}
	return s.size();
}

file_status status(CStringView path)
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
	return {makeFileType(s), (std::uintmax_t)s.st_size, file_time_type{std::chrono::seconds{s.st_mtime}}};
}

file_status symlink_status(CStringView path)
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
	return {makeFileType(s), (std::uintmax_t)s.st_size, file_time_type{std::chrono::seconds{s.st_mtime}}};
}

void chown(CStringView path, uid_t owner, gid_t group)
{
	if(::chown(path, owner, group) == -1) [[unlikely]]
	{
		if(Config::DEBUG_BUILD)
			logErr("chown(%s) error:%s", path.data(), strerror(errno));
		return;
	}
}

bool access(CStringView path, acc type)
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

bool remove(CStringView path)
{
	if(::remove(path) == -1) [[unlikely]]
	{
		if(Config::DEBUG_BUILD)
			logErr("remove(%s) error:%s", path.data(), strerror(errno));
		return false;
	}
	logErr("removed:%s", path.data());
	return true;
}

bool create_directory(CStringView path)
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
			throw std::system_error(err, std::generic_category(), path);
		}
	}
	logMsg("made directory:%s", path.data());
	return true;
}

bool rename(CStringView oldPath, CStringView newPath)
{
	if(::rename(oldPath, newPath) == -1) [[unlikely]]
	{
		if(Config::DEBUG_BUILD)
			logErr("rename(%s, %s) error:%s", oldPath.data(), newPath.data(), strerror(errno));
		return false;
	}
	logMsg("renamed:%s -> %s", oldPath.data(), newPath.data());
	return true;
}

}
