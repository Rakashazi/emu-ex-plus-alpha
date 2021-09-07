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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <regex>

#include "bspf.hxx"
#include "PlusROM.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlusROM::initialize(const ByteBuffer& image, size_t size)
{
  // Host and path are stored at the NMI vector
  size_t i = ((image[size - 5] - 16) << 8) | image[size - 6];  // NMI @ $FFFA
  if(i >= size)
    return myIsPlusROM = false;  // Invalid NMI

  // Path stored first, 0-terminated
  string path;
  while(i < size && image[i] != 0)
    path += static_cast<char>(image[i++]);

  // Did we get a valid, 0-terminated path?
  if(i >= size || image[i] != 0 || !isValidPath(path))
    return myIsPlusROM = false;  // Invalid path

  i++;  // advance past 0 terminator

  // Host stored next, 0-terminated
  string host;
  while(i < size && image[i] != 0)
    host += static_cast<char>(image[i++]);

  // Did we get a valid, 0-terminated host?
  if(i >= size || image[i] != 0 || !isValidHost(host))
    return myIsPlusROM = false;  // Invalid host

  myURL = "http://" + host + "/" + path;
  cerr << "URL: " << myURL << endl;
  return myIsPlusROM = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlusROM::peekHotspot(uInt16 address, uInt8& value)
{
  switch(address & 0x0FFF)
  {
    case 0x0FF2:  // Read next byte from Rx buffer
      return false;

    case 0x0FF3:  // Get number of unread bytes in Rx buffer
      return false;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlusROM::pokeHotspot(uInt16 address, uInt8 value)
{
  switch(address & 0x0FFF)
  {
    case 0x0FF0:  // Write byte to Tx buffer
      return false;

    case 0x0FF1:  // Write byte to Tx buffer and send to backend
                  // (and receive into Rx buffer)
      return false;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlusROM::save(Serializer& out) const
{
  try
  {
    out.putByteArray(myRxBuffer.data(), myRxBuffer.size());
    out.putByteArray(myTxBuffer.data(), myTxBuffer.size());
  }
  catch(...)
  {
    cerr << "ERROR: PlusROM::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlusROM::load(Serializer& in)
{
  try
  {
    in.getByteArray(myRxBuffer.data(), myRxBuffer.size());
    in.getByteArray(myTxBuffer.data(), myTxBuffer.size());
  }
  catch(...)
  {
    cerr << "ERROR: PlusROM::load" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlusROM::isValidHost(const string& host) const
{
  // TODO: This isn't 100% either, as we're supposed to check for the length
  //       of each part between '.' in the range 1 .. 63
  //  Perhaps a better function will be included with whatever network
  //  library we decide to use
  static std::regex rgx(R"(^(([a-z0-9]|[a-z0-9][a-z0-9\-]*[a-z0-9])\.)*([a-z0-9]|[a-z0-9][a-z0-9\-]*[a-z0-9])$)", std::regex_constants::icase);

  return std::regex_match(host, rgx);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlusROM::isValidPath(const string& path) const
{
  // TODO: This isn't 100%
  //  Perhaps a better function will be included with whatever network
  //  library we decide to use
  for(auto c: path)
    if(!((c > 44 && c < 58) || (c > 64 && c < 91) || (c > 96 && c < 122)))
      return false;

  return true;
}
