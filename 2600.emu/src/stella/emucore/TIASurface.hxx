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
// $Id: TIASurface.hxx 3131 2015-01-01 03:49:32Z stephena $
//============================================================================

#ifndef TIASURFACE_HXX
#define TIASURFACE_HXX

class TIA;
class Console;
class OSystem;
class FrameBuffer;
class FBSurface;
class VideoMode;

#include "Rect.hxx"
#include "NTSCFilter.hxx"
#include "bspf.hxx"

/**
  This class is basically a wrapper around all things related to rendering
  the TIA image to FBSurface's, and presenting the results to the screen.
  This is placed in a separate class since currently, rendering a TIA image
  can consist of TV filters, a separate scanline surface, phosphor modes, etc.

  @author  Stephen Anthony
*/

class TIASurface
{
  public:
    /**
      Creates a new TIASurface object
    */
    TIASurface(OSystem& system);

    /**
      Set the TIA object, which is needed for actually rendering the TIA image.
    */
    void initialize(const Console& console, const VideoMode& mode);

    /**
      Set the palette for TIA rendering.  This currently consists of two
      components: the actual TIA palette, and a mixed TIA palette used
      in phosphor mode.  The latter may eventually disappear once a better
      phosphor emulation is developed.

      @param tia_palette  An actual TIA palette, converted to data values
                          that are actually usable by the framebuffer
      @param rgb_palette  The RGB components of the palette, needed for
                          calculating a phosphor palette
    */
    void setPalette(const uInt32* tia_palette, const uInt32* rgb_palette);

    /**
      Get the TIA base surface for use in saving to a PNG image.
    */
    const FBSurface& baseSurface(GUI::Rect& rect);

    /**
      Get the TIA pixel associated with the given TIA buffer index,
      shifting by the given offset (for greyscale values).
    */
    uInt32 pixel(uInt32 idx, uInt8 shift = 0) const;

    /**
      Get the NTSCFilter object associated with the framebuffer
    */
    NTSCFilter& ntsc() { return myNTSCFilter; }

    /**
      Use NTSC filtering effects specified by the given preset.
    */
    void setNTSC(NTSCFilter::Preset preset, bool show = true);

    /**
      Increase/decrease current scanline intensity by given relative amount.
    */
    void setScanlineIntensity(int relative);

    /**
      Toggles interpolation/smoothing of scanlines in TV modes.
    */
    void toggleScanlineInterpolation();

    /**
      Change scanline intensity and interpolation.

      @param relative  If non-zero, change current intensity by
                       'relative' amount, otherwise set to 'absolute'
      @return  New current intensity
    */
    uInt32 enableScanlines(int relative, int absolute = 50);
    void enableScanlineInterpolation(bool enable);

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

    /**
      Enable/disable/query NTSC filtering effects.
    */
    void enableNTSC(bool enable);
    bool ntscEnabled() const { return myFilterType & 0x10; }
    string effectsInfo() const;

    /**
      This method should be called to draw the TIA image(s) to the screen.
    */
    void render();

  private:
    OSystem& myOSystem;
    FrameBuffer& myFB;
    TIA* myTIA;

    shared_ptr<FBSurface> myTiaSurface, mySLineSurface, myBaseTiaSurface;

    // Enumeration created such that phosphor off/on is in LSB,
    // and Blargg off/on is in MSB
    enum FilterType {
      kNormal         = 0x00,
      kPhosphor       = 0x01,
      kBlarggNormal   = 0x10,
      kBlarggPhosphor = 0x11
    };
    FilterType myFilterType;

    enum TIAConstants {
      kTIAW  = 160,
      kTIAH  = 320,
      kScanH = kTIAH*2
    };

    // NTSC object to use in TIA rendering mode
    NTSCFilter myNTSCFilter;

    // Use phosphor effect (aka no flicker on 30Hz screens)
    bool myUsePhosphor;

    // Amount to blend when using phosphor effect
    int myPhosphorBlend;

    // Use scanlines in TIA rendering mode
    bool myScanlinesEnabled;

    // Palette for normal TIA rendering mode
    const uInt32* myPalette;

    // Palette for phosphor rendering mode
    uInt32 myPhosphorPalette[256][256];
};

#endif
