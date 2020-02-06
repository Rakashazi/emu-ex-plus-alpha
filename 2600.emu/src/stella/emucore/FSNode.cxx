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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "FSNodeFactory.hxx"
#include "FSNode.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNode::FilesystemNode(const AbstractFSNodePtr& realNode)
  : _realNode(realNode)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNode::FilesystemNode(const string& p)
{
  // Is this potentially a ZIP archive?
#if defined(ZIP_SUPPORT)
  if (BSPF::containsIgnoreCase(p, ".zip"))
    _realNode = FilesystemNodeFactory::create(p, FilesystemNodeFactory::Type::ZIP);
  else
#endif
    _realNode = FilesystemNodeFactory::create(p, FilesystemNodeFactory::Type::SYSTEM);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNode::exists() const
{
  return _realNode ? _realNode->exists() : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNode::getChildren(FSList& fslist, ListMode mode,
                                 const NameFilter& filter) const
{
  if (!_realNode || !_realNode->isDirectory())
    return false;

  AbstractFSList tmp;
  tmp.reserve(fslist.capacity());

  if (!_realNode->getChildren(tmp, mode))
    return false;

  std::sort(tmp.begin(), tmp.end(),
    [](const AbstractFSNodePtr& node1, const AbstractFSNodePtr& node2)
    {
      if (node1->isDirectory() != node2->isDirectory())
        return node1->isDirectory();
      else
        return BSPF::compareIgnoreCase(node1->getName(), node2->getName()) < 0;
    }
  );

  // Add parent node, if it is valid to do so
  if (hasParent())
  {
    FilesystemNode parent = getParent();
    parent.setName(" [..]");
    fslist.emplace_back(parent);
  }

  // And now add the rest of the entries
  for (const auto& i: tmp)
  {
  #if defined(ZIP_SUPPORT)
    if (BSPF::endsWithIgnoreCase(i->getPath(), ".zip"))
    {
      // Force ZIP c'tor to be called
      AbstractFSNodePtr ptr = FilesystemNodeFactory::create(i->getPath(),
          FilesystemNodeFactory::Type::ZIP);
      FilesystemNode node(ptr);
      if (filter(node))
        fslist.emplace_back(node);
    }
    else
  #endif
    {
      // Make directories stand out
      if (i->isDirectory())
        i->setName(" [" + i->getName() + "]");

      FilesystemNode node(i);
      if (filter(node))
        fslist.emplace_back(node);
    }
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& FilesystemNode::getName() const
{
  return _realNode ? _realNode->getName() : EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilesystemNode::setName(const string& name)
{
  if (_realNode)
    _realNode->setName(name);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& FilesystemNode::getPath() const
{
  return _realNode ? _realNode->getPath() : EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FilesystemNode::getShortPath() const
{
  return _realNode ? _realNode->getShortPath() : EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FilesystemNode::getNameWithExt(const string& ext) const
{
  if (!_realNode)
    return EmptyString;

  size_t pos = _realNode->getName().find_last_of("/\\");
  string s = pos == string::npos ? _realNode->getName() :
        _realNode->getName().substr(pos+1);

  pos = s.find_last_of('.');
  return (pos != string::npos) ? s.replace(pos, string::npos, ext) : s + ext;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FilesystemNode::getPathWithExt(const string& ext) const
{
  if (!_realNode)
    return EmptyString;

  string s = _realNode->getPath();

  size_t pos = s.find_last_of('.');
  return (pos != string::npos) ? s.replace(pos, string::npos, ext) : s + ext;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNode::hasParent() const
{
  return _realNode ? _realNode->hasParent() : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNode FilesystemNode::getParent() const
{
  if (!_realNode)
    return *this;

  AbstractFSNodePtr node = _realNode->getParent();
  return node ? FilesystemNode(node) : *this;
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
size_t FilesystemNode::read(ByteBuffer& image) const
{
  size_t size = 0;

  // File must actually exist
  if (!(exists() && isReadable()))
    throw runtime_error("File not found/readable");

  // First let the private subclass attempt to open the file
  if (_realNode && (size = _realNode->read(image)) > 0)
    return size;

  // Otherwise, the default behaviour is to read from a normal C++ ifstream
  image = make_unique<uInt8[]>(512 * 1024);
  ifstream in(getPath(), std::ios::binary);
  if (in)
  {
    in.seekg(0, std::ios::end);
    std::streampos length = in.tellg();
    in.seekg(0, std::ios::beg);

    if (length == 0)
      throw runtime_error("Zero-byte file");

    size = std::min<size_t>(length, 512 * 1024);
    in.read(reinterpret_cast<char*>(image.get()), size);
  }
  else
    throw runtime_error("File open/read error");

  return size;
}
