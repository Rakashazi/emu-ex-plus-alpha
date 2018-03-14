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

#ifndef CARTRIDGEAR_HXX
#define CARTRIDGEAR_HXX

class System;

#include "bspf.hxx"
#include "Cart.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "CartARWidget.hxx"
#endif

/**
  FIXME: This scheme needs to be be described in more detail.

  This is the cartridge class for Arcadia (aka Starpath) Supercharger
  games.  Christopher Salomon provided most of the technical details
  used in creating this class.  A good description of the Supercharger
  is provided in the Cuttle Cart's manual.

  The Supercharger has four 2K banks.  There are three banks of RAM
  and one bank of ROM.  All 6K of the RAM can be read and written.

  @author  Bradford W. Mott
*/
class CartridgeAR : public Cartridge
{
  friend class CartridgeARWidget;

  public:
    /**
      Create a new cartridge using the specified image and size

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param settings  A reference to the various settings (read-only)
    */
    CartridgeAR(const BytePtr& image, uInt32 size, const Settings& settings);
    virtual ~CartridgeAR() = default;

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
    string name() const override { return "CartridgeAR"; }

  #ifdef DEBUGGER_SUPPORT
    /**
      Get debugger widget responsible for accessing the inner workings
      of the cart.
    */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
        const GUI::Font& nfont, int x, int y, int w, int h) override
    {
      return new CartridgeARWidget(boss, lfont, nfont, x, y, w, h, *this);
    }
  #endif

  public:
    /**
      Get the byte at the specified address

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
    /**
      Query the given address type for the associated disassembly flags.

      @param address  The address to query
    */
    uInt8 getAccessFlags(uInt16 address) const override;
    /**
      Change the given address to use the given disassembly flags.

      @param address  The address to modify
      @param flags    A bitfield of DisasmType directives for the given address
    */
    void setAccessFlags(uInt16 address, uInt8 flags) override;

    // Handle a change to the bank configuration
    bool bankConfiguration(uInt8 configuration);

    // Compute the sum of the array of bytes
    uInt8 checksum(uInt8* s, uInt16 length);

    // Load the specified load into SC RAM
    void loadIntoRAM(uInt8 load);

    // Sets up a "dummy" BIOS ROM in the ROM bank of the cartridge
    void initializeROM();

  private:
    // Indicates the offset within the image for the corresponding bank
    uInt32 myImageOffset[2];

    // The 6K of RAM and 2K of ROM contained in the Supercharger
    uInt8 myImage[8192];

    // The 256 byte header for the current 8448 byte load
    uInt8 myHeader[256];

    // Size of the ROM image
    uInt32 mySize;

    // All of the 8448 byte loads associated with the game
    BytePtr myLoadImages;

    // Indicates how many 8448 loads there are
    uInt8 myNumberOfLoadImages;

    // Indicates if the RAM is write enabled
    bool myWriteEnabled;

    // Indicates if the ROM's power is on or off
    bool myPower;

    // Data hold register used for writing
    uInt8 myDataHoldRegister;

    // Indicates number of distinct accesses when data hold register was set
    uInt32 myNumberOfDistinctAccesses;

    // Indicates if a write is pending or not
    bool myWritePending;

    // Indicates which bank is currently active
    uInt16 myCurrentBank;

    // Fake SC-BIOS code to simulate the Supercharger load bars
    static uInt8 ourDummyROMCode[294];

    // Default 256-byte header to use if one isn't included in the ROM
    // This data comes from z26
    static const uInt8 ourDefaultHeader[256];

  private:
    // Following constructors and assignment operators not supported
    CartridgeAR() = delete;
    CartridgeAR(const CartridgeAR&) = delete;
    CartridgeAR(CartridgeAR&&) = delete;
    CartridgeAR& operator=(const CartridgeAR&) = delete;
    CartridgeAR& operator=(CartridgeAR&&) = delete;
};

#endif
