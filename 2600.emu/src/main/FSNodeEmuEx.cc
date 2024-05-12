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

#include <FSNode.hxx>

bool FilesystemNode::exists() const
{
	return true;
}

const string& FilesystemNode::getName() const
{
	return path;
}

const string& FilesystemNode::getPath() const
{
	return path;
}

bool FilesystemNode::isReadable() const
{
	return true;
}

bool FilesystemNode::isWritable() const
{
	return true;
}

bool FilesystemNode::isDirectory() const
{
	return false;
}

bool FilesystemNode::isFile() const
{
	return true;
}

string FilesystemNode::getNameWithExt(const string& ext) const
{
	size_t pos = getName().find_last_of("/\\");
	string s = pos == string::npos ? getName() : getName().substr(pos+1);
	pos = s.find_last_of('.');
	return (pos != string::npos) ? s.replace(pos, string::npos, ext) : s + ext;
}

string FilesystemNode::getPathWithExt(const string& ext) const
{
	string s = path;
	const size_t pos = s.find_last_of('.');
	return (pos != string::npos) ? s.replace(pos, string::npos, ext) : s + ext;
}
