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

#ifndef NTSC_FILTER_HXX
#define NTSC_FILTER_HXX

class TIASurface;
class Settings;

#include "bspf.hxx"
#include "AtariNTSC.hxx"

/**
  This class is based on the Blargg NTSC filter code from Atari800,
  and is derived from 'filter_ntsc.(h|c)'.  Original code based on
  implementation from http://www.slack.net/~ant.

  The class is basically a thin wrapper around the AtariNTSC class.
*/
class NTSCFilter
{
  public:
    NTSCFilter();

  public:
    // Set one of the available preset adjustments (Composite, S-Video, RGB, etc)
    enum Preset {
      PRESET_OFF,
      PRESET_COMPOSITE,
      PRESET_SVIDEO,
      PRESET_RGB,
      PRESET_BAD,
      PRESET_CUSTOM
    };

    /* Normally used in conjunction with custom mode, contains all
       aspects currently adjustable in NTSC TV emulation. */
    struct Adjustable {
      uInt32 hue, saturation, contrast, brightness, gamma,
             sharpness, resolution, artifacts, fringing, bleed;
    };

  public:
    /* Informs the NTSC filter about the current TIA palette.  The filter
       uses this as a baseline for calculating its own internal palette
       in YIQ format.
    */
    inline void setTIAPalette(const uInt32* palette) {
      uInt8* ptr = myTIAPalette;

      // Set palette for normal fill
      for(uInt32 i = 0; i < AtariNTSC::palette_size; ++i)
      {
        *ptr++ = (palette[i] >> 16) & 0xff;
        *ptr++ = (palette[i] >> 8) & 0xff;
        *ptr++ = palette[i] & 0xff;
      }
      myNTSC.initializePalette(myTIAPalette);
    }

    inline void setPhosphorPalette(uInt8 palette[256][256]) {
      myNTSC.setPhosphorPalette(palette);
    }

    // The following are meant to be used strictly for toggling from the GUI
    string setPreset(Preset preset);

    // Get current preset info encoded as a string
    string getPreset() const;

    // Get adjustables for the given preset
    // Values will be scaled to 0 - 100 range, independent of how
    // they're actually stored internally
    void getAdjustables(Adjustable& adjustable, Preset preset) const;

    // Set custom adjustables to given values
    // Values will be scaled to 0 - 100 range, independent of how
    // they're actually stored internally
    void setCustomAdjustables(Adjustable& adjustable);

    // The following methods cycle through each custom adjustable
    // They are used in conjunction with the increase/decrease
    // methods, which change the currently selected adjustable
    // Changes are made this way since otherwise 20 key-combinations
    // would be needed to dynamically change each setting, and now
    // only 4 combinations are necessary
    string setNextAdjustable();
    string setPreviousAdjustable();
    string increaseAdjustable();
    string decreaseAdjustable();

    // Load and save NTSC-related settings
    void loadConfig(const Settings& settings);
    void saveConfig(Settings& settings) const;

    // Perform Blargg filtering on input buffer, place results in
    // output buffer
    inline void render(uInt8* src_buf, uInt32 src_width, uInt32 src_height,
                       uInt32* dest_buf, uInt32 dest_pitch)
    {
      myNTSC.render(src_buf, src_width, src_height, dest_buf, dest_pitch);
    }
    inline void render(uInt8* src_buf, uInt32 src_width, uInt32 src_height,
                       uInt32* dest_buf, uInt32 dest_pitch, uInt32* prev_buf)
    {
      myNTSC.render(src_buf, src_width, src_height, dest_buf, dest_pitch, prev_buf);
    }

    // Enable threading for the NTSC rendering
    inline void enableThreading(bool enable)
    {
      myNTSC.enableThreading(enable);
    }

  private:
    // Convert from atari_ntsc_setup_t values to equivalent adjustables
    void convertToAdjustable(Adjustable& adjustable,
                             const AtariNTSC::Setup& setup) const;

  private:
    // The NTSC object
    AtariNTSC myNTSC;

    // Contains controls used to adjust the palette in the NTSC filter
    // This is the main setup object used by the underlying ntsc code
    AtariNTSC::Setup mySetup;

    // This setup is used only in custom mode (after it is modified,
    // it is copied to mySetup)
    static AtariNTSC::Setup myCustomSetup;

    // Current preset in use
    Preset myPreset;

    // The base 2600 palette contains 128 normal colours
    // and 128 black&white colours (PAL colour loss)
    // Each colour is represented by 3 bytes, in R,G,B order
    uInt8 myTIAPalette[AtariNTSC::palette_size * 3];

    struct AdjustableTag {
      const char* const type;
      double* value;
    };
    uInt32 myCurrentAdjustable;
    static const AdjustableTag ourCustomAdjustables[10];

  private:
    // Following constructors and assignment operators not supported
    NTSCFilter(const NTSCFilter&) = delete;
    NTSCFilter(NTSCFilter&&) = delete;
    NTSCFilter& operator=(const NTSCFilter&) = delete;
    NTSCFilter& operator=(NTSCFilter&&) = delete;
};

#endif
