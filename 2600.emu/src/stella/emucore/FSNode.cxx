//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FSNode.cxx 3131 2015-01-01 03:49:32Z stephena $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <zlib.h>

#include "bspf.hxx"
#include "FSNodeFactory.hxx"
#include "FSNode.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNode::FilesystemNode()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNode::FilesystemNode(AbstractFSNode *realNode) 
  : _realNode(realNode)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNode::FilesystemNode(const string& p)
{
  AbstractFSNode* tmp = nullptr;

  // Is this potentially a ZIP archive?
  if(BSPF_containsIgnoreCase(p, ".zip"))
    tmp = FilesystemNodeFactory::create(p, FilesystemNodeFactory::ZIP);
  else
    tmp = FilesystemNodeFactory::create(p, FilesystemNodeFactory::SYSTEM);

  _realNode = shared_ptr<AbstractFSNode>(tmp);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNode::exists() const
{
  return _realNode ? _realNode->exists() : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNode::getChildren(FSList& fslist, ListMode mode, bool hidden) const
{
  if (!_realNode || !_realNode->isDirectory())
    return false;

  AbstractFSList tmp;
  tmp.reserve(fslist.capacity());

  if (!_realNode->getChildren(tmp, mode, hidden))
    return false;

  for (const auto& i: tmp)
    fslist.emplace_back(FilesystemNode(i));

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& FilesystemNode::getName() const
{
  return _realNode->getName();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& FilesystemNode::getPath() const
{
  return _realNode->getPath();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FilesystemNode::getShortPath() const
{
  return _realNode->getShortPath();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FilesystemNode::getNameWithExt(const string& ext) const
{
  size_t pos = _realNode->getName().find_last_of("/\\");
  string s = pos == string::npos ? _realNode->getName() :
        _realNode->getName().substr(pos+1);

  pos = s.find_last_of(".");
  return (pos != string::npos) ? s.replace(pos, string::npos, ext) : s + ext;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FilesystemNode::getPathWithExt(const string& ext) const
{
  string s = _realNode->getPath();

  size_t pos = s.find_last_of(".");
  return (pos != string::npos) ? s.replace(pos, string::npos, ext) : s + ext;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FilesystemNode::getShortPathWithExt(const string& ext) const
{
  string s = _realNode->getShortPath();

  size_t pos = s.find_last_of(".");
  return (pos != string::npos) ? s.replace(pos, string::npos, ext) : s + ext;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNode::hasParent() const
{
  return _realNode ? (_realNode->getParent() != 0) : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNode FilesystemNode::getParent() const
{
  if (_realNode == 0)
    return *this;

  AbstractFSNode* node = _realNode->getParent();
  return (node == 0) ? *this : FilesystemNode(node);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNode::isDirectory() const
{
  return _realNode ? _realNode->isDirectory() : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNode::isFile() const
{
  return _realNode ? _realNode->isFile() : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNode::isReadable() const
{
  return _realNode ? _realNode->isReadable() : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNode::isWritable() const
{
  return _realNode ? _realNode->isWritable() : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNode::makeDir()
{
  return (_realNode && !_realNode->exists()) ? _realNode->makeDir() : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNode::rename(const string& newfile)
{
  return (_realNode && _realNode->exists()) ? _realNode->rename(newfile) : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FilesystemNode::read(uInt8*& image) const
{
  uInt32 size = 0;

  // First let the private subclass attempt to open the file
  if((size = _realNode->read(image)) > 0)
    return size;

  // File must actually exist
  if(!(exists() && isReadable()))
    throw "File not found/readable";

  // Otherwise, assume the file is either gzip'ed or not compressed at all
  gzFile f = gzopen(getPath().c_str(), "rb");
  if(f)
  {
    image = new uInt8[512 * 1024];
    size = gzread(f, image, 512 * 1024);
    gzclose(f);

    if(size == 0)
    {
      delete[] image;  image = nullptr;
      throw "Zero-byte file";
    }
    return size;
  }
  else
    throw "ZLIB open/read error";
}
