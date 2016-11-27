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
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: NTSCFilter.hxx 3239 2015-12-29 19:22:46Z stephena $
//============================================================================

#ifndef NTSC_FILTER_HXX
#define NTSC_FILTER_HXX

class TIASurface;
class Settings;

#include "bspf.hxx"
#include "atari_ntsc.hxx"

#define SCALE_FROM_100(x) ((x/50.0)-1.0)
#define SCALE_TO_100(x) uInt32(50*(x+1.0))

/**
  This class is based on the Blargg NTSC filter code from Atari800,
  and is derived from 'filter_ntsc.(h|c)'.  Original code based on
  implementation from http://www.slack.net/~ant.

  The class is basically a thin wrapper around atari_ntsc_xxx structs
  and methods, so that the rest of the codebase isn't affected by
  updated versions of Blargg code.
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
    void setTIAPalette(const TIASurface& tiaSurface, const uInt32* palette);

    // The following are meant to be used strictly for toggling from the GUI
    string setPreset(Preset preset);

    // Get current preset info encoded as a string
    string getPreset() const;

    // Reinitialises the NTSC filter (automatically called after settings
    // have changed)
    inline void updateFilter()
    {
      atari_ntsc_init(&myFilter, &mySetup, myTIAPalette);
    }

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
    // In the current implementation, the source pitch is always the
    // same as the actual width
    inline void blit_single(uInt8* src_buf, int src_width, int src_height,
                            uInt32* dest_buf, long dest_pitch)
    {
      atari_ntsc_blit_single(&myFilter, src_buf, src_width, src_width, src_height,
                             dest_buf, dest_pitch);
    }
    inline void blit_double(uInt8* src_buf, uInt8* src_back_buf,
                            int src_width, int src_height,
                            uInt32* dest_buf, long dest_pitch)
    {
      atari_ntsc_blit_double(&myFilter, src_buf, src_back_buf, src_width, src_width,
                             src_height, dest_buf, dest_pitch);
    }

  private:
    // Convert from atari_ntsc_setup_t values to equivalent adjustables
    void convertToAdjustable(Adjustable& adjustable,
                             const atari_ntsc_setup_t& setup) const;

  private:
    // The NTSC filter structure
    atari_ntsc_t myFilter;

    // Contains controls used to adjust the palette in the NTSC filter
    // This is the main setup object used by the underlying ntsc code
    atari_ntsc_setup_t mySetup;

    // This setup is used only in custom mode (after it is modified,
    // it is copied to mySetup)
    static atari_ntsc_setup_t myCustomSetup;

    // Current preset in use
    Preset myPreset;

    // The base 2600 palette contains 128 colours
    // However, 'phosphor' mode needs a 128x128 matrix to simulate
    // low-flicker output, so we need 128x128 + 128, or 129x128
    // Note that this is a huge hack, which hopefully will go
    // away once the phosphor effect can be more properly emulated
    // Memory layout is as follows:
    //
    //    128x128 in first bytes of array
    //    128     in last bytes of array
    //    Each colour is represented by 3 bytes, in R,G,B order
    uInt8 myTIAPalette[atari_ntsc_palette_size * 3];

    struct AdjustableTag {
      const char* type;
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
