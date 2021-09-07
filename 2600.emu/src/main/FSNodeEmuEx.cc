/*  This file is part of 2600.emu.

	2600.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	2600.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with 2600.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <FSNodeEmuEx.hh>
#include <imagine/logger/logger.h>
#include <assert.h>

FilesystemNodeEmuEx::FilesystemNodeEmuEx()
{}

FilesystemNodeEmuEx::FilesystemNodeEmuEx(const string& path):name(path), path(path)
{}

bool FilesystemNodeEmuEx::exists() const
{
	return true;
}

const string& FilesystemNodeEmuEx::getName() const
{
	return name;
}

void FilesystemNodeEmuEx::setName(const string& name)
{
	this->name = name;
}

const string& FilesystemNodeEmuEx::getPath() const
{
	return path;
}
bool FilesystemNodeEmuEx::isReadable() const
{
	return true;
}

bool FilesystemNodeEmuEx::isWritable() const
{
	return true;
}

string FilesystemNodeEmuEx::getShortPath() const
{
	return ".";
}

bool FilesystemNodeEmuEx::hasParent() const
{
	return false;
}

bool FilesystemNodeEmuEx::isDirectory() const
{
	return false;
}

bool FilesystemNodeEmuEx::isFile() const
{
	return true;
}

bool FilesystemNodeEmuEx::makeDir()
{
	return false;
}

bool FilesystemNodeEmuEx::rename(const string& newfile)
{
	return false;
}

bool FilesystemNodeEmuEx::getChildren(AbstractFSList& myList, ListMode mode) const
{
	return false;
}

AbstractFSNodePtr FilesystemNodeEmuEx::getParent() const
{
	return nullptr;
}
