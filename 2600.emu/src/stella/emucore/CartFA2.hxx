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

#ifndef CARTRIDGEFA2_HXX
#define CARTRIDGEFA2_HXX

class System;

#include "bspf.hxx"
#include "Cart.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "CartFA2Widget.hxx"
#endif

/**
  This is an extended version of the CBS RAM Plus bankswitching scheme
  supported by the Harmony cartridge.

  There are six (or seven) 4K banks, accessible by read/write to $1FF5 -
  $1FFA (or $1FFB), and 256 bytes of RAM.

  The 256 bytes of RAM can be loaded/saved to Harmony cart flash by
  accessing $1FF4 (see ramReadWrite() for more information), which is
  emulated by storing in a file.
  RAM read port is $1100 - $11FF, write port is $1000 - $10FF.

  For 29K versions of the scheme, the first 1K is ARM code (implements
  actual bankswitching on the Harmony cart), which is completely ignored
  by the emulator.  Also supported is a 32K variant.  In any event, only
  data at 1K - 29K of the ROM is used.

  @author  Chris D. Walton
*/
class CartridgeFA2 : public Cartridge
{
  friend class CartridgeFA2Widget;

  public:
    /**
      Create a new cartridge using the specified image

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param osystem   A reference to the OSystem currently in use
    */
    CartridgeFA2(const BytePtr& image, uInt32 size, const OSystem& osystem);
    virtual ~CartridgeFA2() = default;

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
    string name() const override { return "CartridgeFA2"; }

    /**
      Informs the cartridge about the name of the ROM file used when
      creating this cart.

      @param name  The properties file name of the ROM
    */
    void setRomName(const string& name) override;

  #ifdef DEBUGGER_SUPPORT
    /**
      Get debugger widget responsible for accessing the inner workings
      of the cart.
    */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
        const GUI::Font& nfont, int x, int y, int w, int h) override
    {
      return new CartridgeFA2Widget(boss, lfont, nfont, x, y, w, h, *this);
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
    /**
      Either load or save internal RAM to Harmony flash (represented by
      a file in emulation).

      @return  The value at $FF4 with bit 6 set or cleared (depending on
               whether the RAM access was busy or successful)
    */
    uInt8 ramReadWrite();

    /**
      Modify Harmony flash directly (represented by a file in emulation),
      ignoring any timing emulation.  This is for use strictly in the
      debugger, so you can have low-level access to the Flash media.

      @param operation  0 for erase, 1 for read, 2 for write
    */
    void flash(uInt8 operation);

  private:
    // OSsytem currently in use
    const OSystem& myOSystem;

    // The 24K/28K ROM image of the cartridge
    uInt8 myImage[28 * 1024];

    // Actual usable size of the ROM image
    uInt32 mySize;

    // The 256 bytes of RAM on the cartridge
    uInt8 myRAM[256];

    // The time after which the first request of a load/save operation
    // will actually be completed
    // Due to flash RAM constraints, a read/write isn't instantaneous,
    // so we need to emulate the delay as well
    uInt64 myRamAccessTimeout;

    // Full pathname of the file to use when emulating load/save
    // of internal RAM to Harmony cart flash
    string myFlashFile;

    // Indicates the offset into the ROM image (aligns to current bank)
    uInt16 myBankOffset;

  private:
    // Following constructors and assignment operators not supported
    CartridgeFA2() = delete;
    CartridgeFA2(const CartridgeFA2&) = delete;
    CartridgeFA2(CartridgeFA2&&) = delete;
    CartridgeFA2& operator=(const CartridgeFA2&) = delete;
    CartridgeFA2& operator=(CartridgeFA2&&) = delete;
};

#endif
