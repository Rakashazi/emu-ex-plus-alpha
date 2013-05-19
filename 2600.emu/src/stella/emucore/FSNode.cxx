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
// Copyright (c) 1995-2013 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FSNode.cxx 2608 2013-02-13 23:09:31Z stephena $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <zlib.h>

#include "bspf.hxx"
#include "SharedPtr.hxx"
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
  AbstractFSNode* tmp = 0;

  // Is this potentially a ZIP archive?
  if(BSPF_containsIgnoreCase(p, ".zip"))
    tmp = FilesystemNodeFactory::create(p, FilesystemNodeFactory::ZIP);
  else
    tmp = FilesystemNodeFactory::create(p, FilesystemNodeFactory::SYSTEM);

  _realNode = Common::SharedPtr<AbstractFSNode>(tmp);
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

  for (AbstractFSList::iterator i = tmp.begin(); i != tmp.end(); ++i)
    fslist.push_back(FilesystemNode(*i));

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& FilesystemNode::getName() const
{
  assert(_realNode);
  return _realNode->getName();
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
  if (node == 0)
    return *this;
  else
    return FilesystemNode(node);
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
bool FilesystemNode::isAbsolute() const
{
  return _realNode ? _realNode->isAbsolute() : false;
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
bool FilesystemNode::read(uInt8*& image, uInt32& size) const
{
  // First let the private subclass attempt to open the file
  if(_realNode->read(image, size))
    return true;

  // Otherwise, assume the file is either gzip'ed or not compressed at all
  gzFile f = gzopen(getPath().c_str(), "rb");
  if(f)
  {
    image = new uInt8[512 * 1024];
    size = gzread(f, image, 512 * 1024);
    gzclose(f);

    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FilesystemNode::createAbsolutePath(
    const string& p, const string& startpath, const string& ext)
{
  FilesystemNode node(p);
  string path = node.getShortPath();

  // Is path already absolute, or does it start with the given startpath?
  // If not, we prepend the given startpath
  if(!BSPF_startsWithIgnoreCase(p, startpath+BSPF_PATH_SEPARATOR) &&
     !node.isAbsolute())
    path = startpath + BSPF_PATH_SEPARATOR + p;

  // Does the path have a valid extension?
  // If not, we append the given extension
  string::size_type idx = path.find_last_of('.');
  if(idx != string::npos)
  {
    if(!BSPF_equalsIgnoreCase(path.c_str() + idx + 1, ext))
      path = path.replace(idx+1, ext.length(), ext);
  }
  else
    path += "." + ext;

  return path;
}
