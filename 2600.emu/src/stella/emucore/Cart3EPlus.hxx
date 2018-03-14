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

#ifndef CARTRIDGE_3EPLUS_HXX
#define CARTRIDGE_3EPLUS_HXX

class System;

#include "bspf.hxx"
#include "Cart.hxx"

#ifdef DEBUGGER_SUPPORT
class Cartridge3EPlusWidget;
  #include "Cart3EPlusWidget.hxx"
#endif

/**
  Cartridge class from Thomas Jentzsch, mostly based on the 'DASH' scheme
  with the following changes:

  RAM areas:
    - read $x000, write $x200
    - read $x400, write $x600
    - read $x800, write $xa00
    - read $xc00, write $xe00

 @author  Thomas Jentzsch and Stephen Anthony
*/

class Cartridge3EPlus: public Cartridge
{
  friend class Cartridge3EPlusWidget;

  public:
    /**
      Create a new cartridge using the specified image and size

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param settings  A reference to the various settings (read-only)
    */
    Cartridge3EPlus(const BytePtr& image, uInt32 size, const Settings& settings);
    virtual ~Cartridge3EPlus() = default;

  public:
    /** Reset device to its power-on state */
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
    string name() const override { return "Cartridge3E+"; }

  #ifdef DEBUGGER_SUPPORT
    /**
      Get debugger widget responsible for accessing the inner workings
      of the cart.
    */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
        const GUI::Font& nfont, int x, int y, int w, int h) override
    {
      return new Cartridge3EPlusWidget(boss, lfont, nfont, x, y, w, h, *this);
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

      @param address  The address where the value should be stored
      @param value    The value to be stored at the address
      @return         True if the poke changed the device address space, else false
    */
    bool poke(uInt16 address, uInt8 value) override;

  private:
    bool bankRAM(uInt8 bank);      // switch a RAM bank
    bool bankROM(uInt8 bank);      // switch a ROM bank

    void bankRAMSlot(uInt16 bank); // switch in a 512b RAM slot (lower or upper 1/2 bank)
    void bankROMSlot(uInt16 bank); // switch in a 512b RAM slot (read or write port)

    void initializeBankState();    // set all banks according to current bankInUse state

    // We have an array that indicates for each of the 8 512 byte areas of the address space, which ROM/RAM
    // bank is used in that area. ROM switches 1K so occupies 2 successive entries for each switch. RAM occupies
    // two as well, one 512 byte for read and one for write. The RAM locations are +0x800 apart, and the ROM
    // are consecutive. This allows us to determine on a read/write exactly where the data is.

    static constexpr uInt16 BANK_UNDEFINED = 0x8000;   // bank is undefined and inaccessible
    uInt16 bankInUse[8];     // bank being used for ROM/RAM (eight 512 byte areas)

    static constexpr uInt16 BANK_SWITCH_HOTSPOT_RAM = 0x3E;   // writes to this address cause bankswitching
    static constexpr uInt16 BANK_SWITCH_HOTSPOT_ROM = 0x3F;   // writes to this address cause bankswitching

    static constexpr uInt8 BANK_BITS = 6;                         // # bits for bank
    static constexpr uInt8 BIT_BANK_MASK = (1 << BANK_BITS) - 1;  // mask for those bits
    static constexpr uInt16 BITMASK_LOWERUPPER = 0x100;   // flags lower or upper section of bank (1==upper)
    static constexpr uInt16 BITMASK_ROMRAM     = 0x200;   // flags ROM or RAM bank switching (1==RAM)

    static constexpr uInt16 MAXIMUM_BANK_COUNT = (1 << BANK_BITS);
    static constexpr uInt16 RAM_BANK_TO_POWER = 9;    // 2^n = 512
    static constexpr uInt16 RAM_BANK_SIZE = (1 << RAM_BANK_TO_POWER);
    static constexpr uInt16 BITMASK_RAM_BANK = (RAM_BANK_SIZE - 1);
    static constexpr uInt32 RAM_TOTAL_SIZE = MAXIMUM_BANK_COUNT * RAM_BANK_SIZE;

    static constexpr uInt16 ROM_BANK_TO_POWER = 10;   // 2^n = 1024
    static constexpr uInt16 ROM_BANK_SIZE = (1 << ROM_BANK_TO_POWER);
    static constexpr uInt16 BITMASK_ROM_BANK = (ROM_BANK_SIZE - 1);

    static constexpr uInt16 ROM_BANK_COUNT = 64;

    static constexpr uInt16 RAM_WRITE_OFFSET = 0x200;

    BytePtr myImage;  // Pointer to a dynamically allocated ROM image of the cartridge
    uInt32  mySize;   // Size of the ROM image
    uInt8 myRAM[RAM_TOTAL_SIZE];

  private:
    // Following constructors and assignment operators not supported
    Cartridge3EPlus() = delete;
    Cartridge3EPlus(const Cartridge3EPlus&) = delete;
    Cartridge3EPlus(Cartridge3EPlus&&) = delete;
    Cartridge3EPlus& operator=(const Cartridge3EPlus&) = delete;
    Cartridge3EPlus& operator=(Cartridge3EPlus&&) = delete;
};

#endif
