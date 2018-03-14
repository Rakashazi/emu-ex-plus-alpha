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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef MD5_HXX
#define MD5_HXX

class FilesystemNode;

#include "bspf.hxx"

namespace MD5 {

/**
  Get the MD5 Message-Digest of the specified message with the
  given length.  The digest consists of 32 hexadecimal digits.

  @param buffer The message to compute the digest of
  @param length The length of the message
  @return The message-digest
*/
string hash(const BytePtr& buffer, uInt32 length);
string hash(const uInt8* buffer, uInt32 length);

/**
  Get the MD5 Message-Digest of the file contained in 'node'.
  The digest consists of 32 hexadecimal digits.

  @param node The file node to compute the digest of
  @return The message-digest
*/
string hash(const FilesystemNode& node);

}  // Namespace MD5

#endif
