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

#ifndef CARTRIDGECV_HXX
#define CARTRIDGECV_HXX

class System;

#include "bspf.hxx"
#include "Cart.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "CartCVWidget.hxx"
#endif

/**
  Cartridge class used for Commavid's extra-RAM games.

  $F000-$F3FF read from RAM
  $F400-$F7FF write to RAM
  $F800-$FFFF ROM

  @author  Eckhard Stolberg
*/
class CartridgeCV : public Cartridge
{
  friend class CartridgeCVWidget;

  public:
    /**
      Create a new cartridge using the specified image

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param settings  A reference to the various settings (read-only)
    */
    CartridgeCV(const BytePtr& image, uInt32 size, const Settings& settings);
    virtual ~CartridgeCV() = default;

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
    string name() const override { return "CartridgeCV"; }

  #ifdef DEBUGGER_SUPPORT
    /**
      Get debugger widget responsible for accessing the inner workings
      of the cart.
    */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
        const GUI::Font& nfont, int x, int y, int w, int h) override
    {
      return new CartridgeCVWidget(boss, lfont, nfont, x, y, w, h, *this);
    }
  #endif

  public:
    /**
      Get the byte at the specified address

      @return The byte at the specified address
    */
    uInt8 peek(uInt16 address) override;

  private:
    // Pointer to the initial RAM data from the cart
    // This doesn't always exist, so we don't pre-allocate it
    BytePtr myInitialRAM;

    // Initial size of the cart data
    uInt32 mySize;

    // The 2k ROM image for the cartridge
    uInt8 myImage[2048];

    // The 1024 bytes of RAM
    uInt8 myRAM[1024];

  private:
    // Following constructors and assignment operators not supported
    CartridgeCV() = delete;
    CartridgeCV(const CartridgeCV&) = delete;
    CartridgeCV(CartridgeCV&&) = delete;
    CartridgeCV& operator=(const CartridgeCV&) = delete;
    CartridgeCV& operator=(CartridgeCV&&) = delete;
};

#endif
