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
// $Id: MediaFactory.hxx 2600 2013-02-10 21:47:47Z stephena $
//============================================================================

#ifndef MEDIA_FACTORY_HXX
#define MEDIA_FACTORY_HXX

#include "OSystem.hxx"
#include "Settings.hxx"

#include "FrameBuffer.hxx"
#include "FrameBufferSoft.hxx"
#ifdef DISPLAY_OPENGL
  #include "FrameBufferGL.hxx"
#endif

#include "Sound.hxx"
#ifdef SOUND_SUPPORT
  #include "SoundSDL.hxx"
#else
  #include "SoundNull.hxx"
#endif

/**
  This class deals with the different framebuffer/sound implementations
  for the various ports of Stella, and always returns a valid media object
  based on the specific port and restrictions on that port.

  I think you can see why this mess was put into a factory class :)

  @author  Stephen Anthony
  @version $Id: MediaFactory.hxx 2600 2013-02-10 21:47:47Z stephena $
*/
class MediaFactory
{
  public:
    static FrameBuffer* createVideo(OSystem* osystem)
    {
      FrameBuffer* fb = (FrameBuffer*) NULL;

      // OpenGL mode *may* fail, so we check for it first
    #ifdef DISPLAY_OPENGL
      if(osystem->settings().getString("video") == "gl")
      {
        const string& gl_lib = osystem->settings().getString("gl_lib");
        if(FrameBufferGL::loadLibrary(gl_lib))
          fb = new FrameBufferGL(osystem);
      }
    #endif

      // If OpenGL failed, or if it wasn't requested, create the appropriate
      // software framebuffer
      if(!fb)
        fb = new FrameBufferSoft(osystem);

      // This should never happen
      assert(fb != NULL);

      return fb;
    }

    static Sound* createAudio(OSystem* osystem)
    {
      Sound* sound = (Sound*) NULL;

    #ifdef SOUND_SUPPORT
      sound = new SoundSDL(osystem);
    #else
      sound = new SoundNull(osystem);
    #endif

      return sound;
    }
};

#endif
