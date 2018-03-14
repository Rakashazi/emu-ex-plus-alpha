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

#ifndef TIASURFACE_HXX
#define TIASURFACE_HXX

class TIA;
class Console;
class OSystem;
class FrameBuffer;
class FBSurface;
class VideoMode;

#include <thread>

#include "Rect.hxx"
#include "NTSCFilter.hxx"
#include "bspf.hxx"
#include "TIAConstants.hxx"

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
    const FBSurface& baseSurface(GUI::Rect& rect) const;

    /**
      Get the TIA pixel associated with the given TIA buffer index,
      shifting by the given offset (for greyscale values).
    */
    uInt32 pixel(uInt32 idx, uInt8 shift = 0);

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
      Enable/disable/query phosphor effect.
    */
    void enablePhosphor(bool enable, int blend = -1);
    bool phosphorEnabled() const { return myUsePhosphor; }

    /**
      Used to calculate an averaged color for the 'phosphor' effect.

      @param c1  Color 1
      @param c2  Color 2

      @return  Averaged value of the two colors
    */
    inline uInt8 getPhosphor(const uInt8 c1, uInt8 c2) const {
      // Use maximum of current and decayed previous values
      c2 = uInt8(c2 * myPhosphorPercent);
      if(c1 > c2)  return c1; // raise (assumed immediate)
      else         return c2; // decay
    }

    /**
      Used to calculate an averaged color for the 'phosphor' effect.

      @param c  RGB Color 1 (current frame)
      @param cp RGB Color 2 (previous frame)

      @return  Averaged value of the two RGB colors
    */
    uInt32 getRGBPhosphor(const uInt32 c, const uInt32 cp) const;

    /**
      Enable/disable/query NTSC filtering effects.
    */
    void enableNTSC(bool enable);
    bool ntscEnabled() const { return uInt8(myFilter) & 0x10; }
    string effectsInfo() const;

    /**
      This method should be called to draw the TIA image(s) to the screen.
    */
    void render();

    /**
      This method renders the current frame again.
    */
    void reRender();

  private:
    OSystem& myOSystem;
    FrameBuffer& myFB;
    TIA* myTIA;

    shared_ptr<FBSurface> myTiaSurface, mySLineSurface, myBaseTiaSurface;

    // Enumeration created such that phosphor off/on is in LSB,
    // and Blargg off/on is in MSB
    enum class Filter: uInt8 {
      Normal         = 0x00,
      Phosphor       = 0x01,
      BlarggNormal   = 0x10,
      BlarggPhosphor = 0x11
    };
    Filter myFilter;

    enum {
      kTIAW  = 160,
      kTIAH  = TIAConstants::frameBufferHeight,
      kScanH = kTIAH*2
    };

    // NTSC object to use in TIA rendering mode
    NTSCFilter myNTSCFilter;

    /////////////////////////////////////////////////////////////
    // Phosphor mode items (aka reduced flicker on 30Hz screens)
    // RGB frame buffer
    uInt32 myRGBFramebuffer[AtariNTSC::outWidth(kTIAW) * kTIAH];

    // Use phosphor effect
    bool myUsePhosphor;

    // Amount to blend when using phosphor effect
    float myPhosphorPercent;

    // Precalculated averaged phosphor colors
    uInt8 myPhosphorPalette[256][256];
    /////////////////////////////////////////////////////////////

    // Use scanlines in TIA rendering mode
    bool myScanlinesEnabled;

    // Palette for normal TIA rendering mode
    const uInt32* myPalette;

  private:
    // Following constructors and assignment operators not supported
    TIASurface() = delete;
    TIASurface(const TIASurface&) = delete;
    TIASurface(TIASurface&&) = delete;
    TIASurface& operator=(const TIASurface&) = delete;
    TIASurface& operator=(TIASurface&&) = delete;
};

#endif
