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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Serializer.cxx 2318 2011-12-31 21:56:36Z stephena $
//============================================================================

#include <fstream>
#include <sstream>

#include "FSNode.hxx"
#include "Serializer.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Serializer::Serializer(const string& filename, bool readonly)
  : myStream(NULL),
    myUseFilestream(true)
{
  if(readonly)
  {
    /*FilesystemNode node(filename);
    if(!node.isDirectory() && node.isReadable())*/
    {
      fstream* str = new fstream(filename.c_str(), ios::in | ios::binary);
      if(str && str->is_open())
      {
        myStream = str;
        reset();
      }
      else
        delete str;
    }
  }
  else
  {
    // When using fstreams, we need to manually create the file first
    // if we want to use it in read/write mode, since it won't be created
    // if it doesn't already exist
    // However, if it *does* exist, we don't want to overwrite it
    // So we open in write and append mode - the write creates the file
    // when necessary, and the append doesn't delete any data if it
    // already exists
    fstream temp(filename.c_str(), ios::out | ios::app);
    temp.close();

    fstream* str = new fstream(filename.c_str(), ios::in | ios::out | ios::binary);
    if(str && str->is_open())
    {
      myStream = str;
      reset();
    }
    else
      delete str;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Serializer::Serializer(void)
  : myStream(NULL),
    myUseFilestream(false)
{
  myStream = new stringstream(ios::in | ios::out | ios::binary);
  
  // For some reason, Windows and possibly OSX needs to store something in
  // the stream before it is used for the first time
  if(myStream)
  {
    putBool(true);
    reset();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Serializer::~Serializer(void)
{
  if(myStream != NULL)
  {
    if(myUseFilestream)
      ((fstream*)myStream)->close();

    delete myStream;
    myStream = NULL;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Serializer::isValid(void)
{
  return myStream != NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::reset(void)
{
  myStream->seekg(ios_base::beg);
  myStream->seekp(ios_base::beg);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char Serializer::getByte(void)
{
  if(myStream->eof())
    throw "Serializer::getByte() end of file";

  char buf;
  myStream->read(&buf, 1);

  return buf;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Serializer::getInt(void)
{
  if(myStream->eof())
    throw "Serializer::getInt() end of file";

  int val = 0;
  unsigned char buf[4];
  myStream->read((char*)buf, 4);
  for(int i = 0; i < 4; ++i)
    val += (int)(buf[i]) << (i<<3);

  return val;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Serializer::getString(void)
{
  int len = getInt();
  string str;
  str.resize((string::size_type)len);
  myStream->read(&str[0], (streamsize)len);

  if(myStream->bad())
    throw "Serializer::getString() file read failed";

  return str;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Serializer::getBool(void)
{
  char b = getByte();
  if(b == (char)TruePattern)
    return true;
  else if(b == (char)FalsePattern)
    return false;
  else
    throw "Serializer::getBool() data corruption";

  return false;  // to stop compiler from complaining
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putByte(char value)
{
  myStream->write(&value, 1);
  if(myStream->bad())
    throw "Serializer::putByte() file write failed";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putInt(int value)
{
  unsigned char buf[4];
  for(int i = 0; i < 4; ++i)
    buf[i] = (value >> (i<<3)) & 0xff;

  myStream->write((char*)buf, 4);
  if(myStream->bad())
    throw "Serializer::putInt() file write failed";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putString(const string& str)
{
  int len = str.length();
  putInt(len);
  myStream->write(str.data(), (streamsize)len);

  if(myStream->bad())
    throw "Serializer::putString() file write failed";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putBool(bool b)
{
  putByte(b ? TruePattern: FalsePattern);
}
