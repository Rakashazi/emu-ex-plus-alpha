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

#ifndef CARTRIDGEMDM_HXX
#define CARTRIDGEMDM_HXX

#include "bspf.hxx"
#include "Cart.hxx"
#include "System.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "CartMDMWidget.hxx"
#endif

/**
  Cartridge class used for "Menu Driven Megacart" as described at the
  following link and developed by Edwin Blink:

    http://atariage.com/forums/topic/56073-cheap-2k4k-x-in-1-menu-driven-multicart-for-atari-2600

  Note that this code implements a modified scheme (as designed by E. Blink).
  In this version, the hotspots are from $800 to $BFF instead of $800 to $FFF.

  The hotspots in this scheme are read/write at addresses $800 to $BFF, where
  the lower byte determines the actual 4K bank switch to.  In the current
  implementation, only 128 banks are supported, so selecting bank 128+ results
  in further bankswitching being locked.  A reset line is used to reset to
  bank 0 and re-enable bankswitching.

  Therefore, there are 128 banks / 512K possible in total.

  @author  Stephen Anthony, based on 0840 scheme by Fred X. Quimby
*/
class CartridgeMDM : public Cartridge
{
  friend class CartridgeMDMWidget;

  public:
    /**
      Create a new cartridge using the specified image

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param settings  A reference to the various settings (read-only)
    */
    CartridgeMDM(const BytePtr& image, uInt32 size, const Settings& settings);
    virtual ~CartridgeMDM() = default;

  public:
    /**
      Reset device to its power-on state
    */
    void reset() override;

    /**
      Install cartridge in the specified system.  Invoked by the system
      when the cartridge is attached to it.

      @param system The system the device should install itself in
    */
    void install(System& system) override;

    /**
      Install pages for the specified bank in the system.

      @param bank The bank that should be installed in the system
    */
    bool bank(uInt16 bank) override;

    /**
      Get the current bank.
    */
    uInt16 getBank() const override;

    /**
      Query the number of banks supported by the cartridge.
    */
    uInt16 bankCount() const override;

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
    string name() const override { return "CartridgeMDM"; }

  #ifdef DEBUGGER_SUPPORT
    /**
      Get debugger widget responsible for accessing the inner workings
      of the cart.
    */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
        const GUI::Font& nfont, int x, int y, int w, int h) override
    {
      return new CartridgeMDMWidget(boss, lfont, nfont, x, y, w, h, *this);
    }
  #endif

  public:
    /**
      Get the byte at the specified address.

      @return The byte at the specified address
    */
    uInt8 peek(uInt16 address) override;

    /**
      Change the byte at the specified address to the given value

      @param address The address where the value should be stored
      @param value The value to be stored at the address
      @return  True if the poke changed the device address space, else false
    */
    bool poke(uInt16 address, uInt8 value) override;

  private:
    // Pointer to a dynamically allocated ROM image of the cartridge
    BytePtr myImage;

    // Size of the ROM image
    uInt32 mySize;

    // Previous Device's page access
    System::PageAccess myHotSpotPageAccess[8];

    // Indicates the offset into the ROM image (aligns to current bank)
    uInt32 myBankOffset;

    // Indicates whether banking has been disabled due to a bankswitch
    // above bank 127
    bool myBankingDisabled;

  private:
    // Following constructors and assignment operators not supported
    CartridgeMDM() = delete;
    CartridgeMDM(const CartridgeMDM&) = delete;
    CartridgeMDM(CartridgeMDM&&) = delete;
    CartridgeMDM& operator=(const CartridgeMDM&) = delete;
    CartridgeMDM& operator=(CartridgeMDM&&) = delete;
};

#endif
