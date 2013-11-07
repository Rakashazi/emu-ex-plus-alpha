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
// $Id: FrameBuffer.hxx 2579 2013-01-04 19:49:01Z stephena $
//============================================================================

#ifndef FRAMEBUFFER_HXX
#define FRAMEBUFFER_HXX

#include "EventHandler.hxx"
class OSystem;

// Return values for initialization of framebuffer window
enum FBInitStatus {
  kSuccess,
  kFailComplete,
  kFailTooLarge,
  kFailNotSupported,
};


class FrameBuffer
{
  public:
    /**
      Creates a new Frame Buffer
    */
    FrameBuffer(/*OSystem* osystem*/);

    /**
      (Re)initializes the framebuffer display.  This must be called before any
      calls are made to derived methods.

      @param title   The title of the window
      @param width   The width of the framebuffer
      @param height  The height of the framebuffer

      @return  Status of initialization (see FBInitStatus 'enum')
    */
    FBInitStatus initialize(const string& title, uInt32 width, uInt32 height);

    /**
      Updates the display, which depending on the current mode could mean
      drawing the TIA, any pending menus, etc.
    */
    void update();

    /**
      Shows a message containing frame statistics for the current frame.
    */
    void showFrameStats(bool enable);

    /**
      Refresh display according to the current state, taking single vs.
      double-buffered modes into account, and redrawing accordingly.
    */
    void refresh();

    /**
      Set up the TIA/emulation palette for a screen of any depth > 8.

      @param palette  The array of colors
    */
    void setTIAPalette(const uInt32* palette);

    /**
      Shows a message onscreen.

      @param message  The message to be shown
      @param position Onscreen position for the message
      @param force    Force showing this message, even if messages are disabled
      @param color    Color of text in the message
    */
    void showMessage(const string& message,
                     int position = 0,
                     bool force = false,
                     uInt32 color = 0) {};

  //////////////////////////////////////////////////////////////////////
  // The following methods are system-specific and must be implemented
  // in derived classes.
  //////////////////////////////////////////////////////////////////////
  public:
    /**
      Enable/disable phosphor effect.
    */
    void enablePhosphor(bool enable, int blend);

    /**
      Used to calculate an averaged color for the 'phosphor' effect.

      @param c1  Color 1
      @param c2  Color 2

      @return  Averaged value of the two colors
    */
    uInt8 getPhosphor(uInt8 c1, uInt8 c2) const;

    bool myUsePhosphor = false;
    int myPhosphorBlend = 77;
};

#endif
