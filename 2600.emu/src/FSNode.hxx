#pragma once

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

#include "bspf.hxx"
#include <utility>

class FilesystemNode
{
public:
	constexpr FilesystemNode() = default;
	template<class T>
	FilesystemNode(T&& path): path{std::forward<T>(path)} {}

  friend ostream& operator<<(ostream& os, const FilesystemNode& node)
  {
    return os << node.getPath();
  }

	bool exists() const;
	const string& getName() const;
	const string& getPath() const;
	bool isDirectory() const;
	bool isFile() const;
	bool isReadable() const;
	bool isWritable() const;
	size_t read(ByteBuffer& buffer, size_t size = 0) const { return 0; }
	size_t read(stringstream& buffer) const { return 0; }
	size_t write(const ByteBuffer& buffer, size_t size) const { return 0; }
	size_t write(const stringstream& buffer) const { return 0; }
	string getNameWithExt(const string& ext) const;
	string getPathWithExt(const string& ext) const;

protected:
	string path;
};
