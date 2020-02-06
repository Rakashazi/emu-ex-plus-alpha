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
//============================================================================

#ifndef TIASURFACE_HXX
#define TIASURFACE_HXX

class TIA;
class Console;
class OSystem;
class FBSurface;

#include <thread>

#include "Rect.hxx"
#include "FrameBuffer.hxx"
#include "NTSCFilter.hxx"
#include "PhosphorHandler.hxx"
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
    explicit TIASurface(OSystem& system);

    /**
      Set the TIA object, which is needed for actually rendering the TIA image.
    */
    void initialize(const Console& console, const FrameBuffer::VideoMode& mode);

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
    void setPalette(const PaletteArray& tia_palette, const PaletteArray& rgb_palette);

    /**
      Get the TIA base surface for use in saving to a PNG image.
    */
    const FBSurface& baseSurface(Common::Rect& rect) const;

    /**
      Use the palette to map a single indexed pixel color. This is used by the TIA output widget.
     */
    uInt32 mapIndexedPixel(uInt8 indexedColor, uInt8 shift = 0);

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
    bool phosphorEnabled() const { return myPhosphorHandler.phosphorEnabled(); }

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
      This method prepares the current frame for taking a snapshot.
      In particular, in phosphor modes the blending is adjusted slightly to
      generate better images.
    */
    void renderForSnapshot();

    /**
      Save a snapshot after rendering.
    */
    void saveSnapShot() { mySaveSnapFlag = true; }

    /**
      Update surface settings.
     */
    void updateSurfaceSettings();

  private:
    /**
      Average current calculated buffer's pixel with previous calculated buffer's pixel (50:50).
    */
    uInt32 averageBuffers(uInt32 bufOfs);

  private:
    OSystem& myOSystem;
    FrameBuffer& myFB;
    TIA* myTIA{nullptr};

    shared_ptr<FBSurface> myTiaSurface, mySLineSurface, myBaseTiaSurface;

    // Enumeration created such that phosphor off/on is in LSB,
    // and Blargg off/on is in MSB
    enum class Filter: uInt8 {
      Normal         = 0x00,
      Phosphor       = 0x01,
      BlarggNormal   = 0x10,
      BlarggPhosphor = 0x11
    };
    Filter myFilter{Filter::Normal};

    // NTSC object to use in TIA rendering mode
    NTSCFilter myNTSCFilter;

    /////////////////////////////////////////////////////////////
    // Phosphor mode items (aka reduced flicker on 30Hz screens)
    // RGB frame buffer
    PhosphorHandler myPhosphorHandler;

    std::array<uInt32, AtariNTSC::outWidth(TIAConstants::frameBufferWidth) *
        TIAConstants::frameBufferHeight> myRGBFramebuffer;
    std::array<uInt32, AtariNTSC::outWidth(TIAConstants::frameBufferWidth) *
        TIAConstants::frameBufferHeight> myPrevRGBFramebuffer;
    /////////////////////////////////////////////////////////////

    // Use scanlines in TIA rendering mode
    bool myScanlinesEnabled{false};

    // Palette for normal TIA rendering mode
    PaletteArray myPalette;

    // Flag for saving a snapshot
    bool mySaveSnapFlag{false};

  private:
    // Following constructors and assignment operators not supported
    TIASurface() = delete;
    TIASurface(const TIASurface&) = delete;
    TIASurface(TIASurface&&) = delete;
    TIASurface& operator=(const TIASurface&) = delete;
    TIASurface& operator=(TIASurface&&) = delete;
};

#endif
