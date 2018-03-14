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

#ifndef CARTRIDGE2K_HXX
#define CARTRIDGE2K_HXX

class System;

#include "bspf.hxx"
#include "Cart.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "Cart2KWidget.hxx"
#endif

/**
  This is the standard Atari 2K cartridge.  These cartridges are not
  bankswitched, however, the data repeats twice in the 2600's 4K cartridge
  addressing space.  For 'Sub2K' ROMs (ROMs less than 2K in size), the
  data repeats in intervals based on the size of the ROM (which will
  always be a power of 2).

  @author  Stephen Anthony
*/
class Cartridge2K : public Cartridge
{
  friend class Cartridge2KWidget;

  public:
    /**
      Create a new cartridge using the specified image

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image (<= 2048 bytes)
      @param settings  A reference to the various settings (read-only)
    */
    Cartridge2K(const BytePtr& image, uInt32 size, const Settings& settings);
    virtual ~Cartridge2K() = default;

  public:
    /**
      Reset cartridge to its power-on state
    */
    void reset() override;

    /**
      Install cartridge in the specified system.  Invoked by the system
      when the cartridge is attached to it.

      @param system The system the device should install itself in
    */
    void install(System& system) override;

    /**
      Patch the cartridge ROM.

      @param address  The ROM address to patch
      @param value    The value to place into the address
      @return    Success or failure of the patch operation
    */
    bool patch(uInt16 address, uInt8 value) override;

    /**
      Access the internal ROM image for this cartridge.

      @param size  Set to the size of the internal ROM image data
      @return  A pointer to the internal ROM image data
    */
    const uInt8* getImage(uInt32& size) const override;

    /**
      Save the current state of this cart to the given Serializer.

      @param out  The Serializer object to use
      @return  False on any errors, else true
    */
    bool save(Serializer& out) const override;

    /**
      Load the current state of this cart from the given Serializer.

      @param in  The Serializer object to use
      @return  False on any errors, else true
    */
    bool load(Serializer& in) override;

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const override { return "Cartridge2K"; }

  #ifdef DEBUGGER_SUPPORT
    /**
      Get debugger widget responsible for accessing the inner workings
      of the cart.
    */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
        const GUI::Font& nfont, int x, int y, int w, int h) override
    {
      return new Cartridge2KWidget(boss, lfont, nfont, x, y, w, h, *this);
    }
  #endif

  private:
    // Pointer to a dynamically allocated ROM image of the cartridge
    BytePtr myImage;

    // Size of the ROM image
    uInt32 mySize;

    // Mask to use for mirroring
    uInt32 myMask;

  private:
    // Following constructors and assignment operators not supported
    Cartridge2K() = delete;
    Cartridge2K(const Cartridge2K&) = delete;
    Cartridge2K(Cartridge2K&&) = delete;
    Cartridge2K& operator=(const Cartridge2K&) = delete;
    Cartridge2K& operator=(Cartridge2K&&) = delete;
};

#endif
