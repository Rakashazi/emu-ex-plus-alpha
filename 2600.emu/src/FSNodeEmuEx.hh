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

#include <stella/emucore/FSNode.hxx>

class FilesystemNodeEmuEx : public AbstractFSNode
{
public:
	FilesystemNodeEmuEx();
	FilesystemNodeEmuEx(const string& path);
	bool exists() const final;
	const string& getName() const final;
	void setName(const string& name) final;
	const string& getPath() const final;
	string getShortPath() const final;
	bool hasParent() const final;
	bool isDirectory() const final;
	bool isFile() const final;
	bool isReadable() const final;
	bool isWritable() const final;
	bool makeDir() final;
	bool rename(const string& newfile) final;
	bool getChildren(AbstractFSList& list, ListMode mode) const final;
	AbstractFSNodePtr getParent() const final;
	size_t read(ByteBuffer& image) const final;

protected:
	string name{};
	string path{};
};
