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
// $Id: MediaFactory.hxx 2318 2011-12-31 21:56:36Z stephena $
//============================================================================

#ifndef MEDIA_FACTORY_HXX
#define MEDIA_FACTORY_HXX

class FrameBuffer;
class Sound;
class OSystem;

/**
  This class deals with the different framebuffer/sound implementations
  for the various ports of Stella, and always returns a valid media object
  based on the specific port and restrictions on that port.

  @author  Stephen Anthony
  @version $Id: MediaFactory.hxx 2318 2011-12-31 21:56:36Z stephena $
*/
class MediaFactory
{
  public:
    static FrameBuffer* createVideo(OSystem* osystem);
    static Sound* createAudio(OSystem* osystem);
};

#endif
