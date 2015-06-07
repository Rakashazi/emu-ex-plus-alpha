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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: CartWD.hxx 3131 2015-01-01 03:49:32Z stephena $
//============================================================================

#ifndef CARTRIDGEWD_HXX
#define CARTRIDGEWD_HXX

class System;

#include "bspf.hxx"
#include "Cart.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "CartWDWidget.hxx"
#endif

/**
  This is the cartridge class for a "Wickstead Design" prototype cart.
  The ROM is normally 8K, but sometimes has an extra 3 bytes appended,
  to be mapped as described below.  There is also 64 bytes of RAM.
  In this bankswitching scheme the 2600's 4K cartridge address space 
  is broken into four 1K segments.  The desired arrangement of 1K slices
  is selected by accessing $30 - $3F of TIA address space.  The slices
  are mapped into all 4 segments at once as follows:

    $0030: 0,0,1,2
    $0031: 0,1,3,2
    $0032: 4,5,6,7
    $0033: 7,4,3,2
    $0034: 0,0,6,7
    $0035: 0,1,7,6
    $0036: 3,2,4,5
    $0037: 6,0,5,1
    $0038: 0,0,1,2
    $0039: 0,1,3,2
    $003A: 4,5,6,7
    $003B: 7,4,3,2
    $003C: 0,0,6,7*
    $003D: 0,1,7,6*
    $003E: 3,2,4,5*
    $003F: 6,0,5,1*

  In the last 4 cases, the extra 3 bytes of ROM past the 8K boundary are
  mapped into $3FC - $3FE of the uppermost (third) segment.

  The 64 bytes of RAM are accessible at $1000 - $103F (read port) and
  $1040 - $107F (write port).  Because the RAM takes 128 bytes of address
  space, the range $1000 - $107F of segment 0 ROM will never be available.

  @author  Stephen Anthony
*/
class CartridgeWD : public Cartridge
{
  friend class CartridgeWDWidget;

  public:
    /**
      Create a new cartridge using the specified image

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param settings  A reference to the various settings (read-only)
    */
    CartridgeWD(const uInt8* image, uInt32 size, const Settings& settings);
 
    /**
      Destructor
    */
    virtual ~CartridgeWD();

  public:
    /**
      Reset device to its power-on state
    */
    void reset();

    /**
      Install cartridge in the specified system.  Invoked by the system
      when the cartridge is attached to it.

      @param system The system the device should install itself in
    */
    void install(System& system);

    /**
      Install pages for the specified bank in the system.

      @param bank The bank that should be installed in the system
    */
    bool bank(uInt16 bank);

    /**
      Get the current bank.
    */
    uInt16 getBank() const;

    /**
      Query the number of banks supported by the cartridge.
    */
    uInt16 bankCount() const;

    /**
      Patch the cartridge ROM.

      @param address  The ROM address to patch
      @param value    The value to place into the address
      @return    Success or failure of the patch operation
    */
    bool patch(uInt16 address, uInt8 value);

    /**
      Access the internal ROM image for this cartridge.

      @param size  Set to the size of the internal ROM image data
      @return  A pointer to the internal ROM image data
    */
    const uInt8* getImage(int& size) const;

    /**
      Save the current state of this cart to the given Serializer.

      @param out  The Serializer object to use
      @return  False on any errors, else true
    */
    bool save(Serializer& out) const;

    /**
      Load the current state of this cart from the given Serializer.

      @param in  The Serializer object to use
      @return  False on any errors, else true
    */
    bool load(Serializer& in);

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const { return "CartridgeWD"; }

  #ifdef DEBUGGER_SUPPORT
    /**
      Get debugger widget responsible for accessing the inner workings
      of the cart.
    */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
        const GUI::Font& nfont, int x, int y, int w, int h)
    {
      return new CartridgeWDWidget(boss, lfont, nfont, x, y, w, h, *this);
    }
  #endif

  public:
    /**
      Get the byte at the specified address.

      @return The byte at the specified address
    */
    uInt8 peek(uInt16 address);

    /**
      Change the byte at the specified address to the given value.

      @param address The address where the value should be stored
      @param value The value to be stored at the address
      @return  True if the poke changed the device address space, else false
    */
    bool poke(uInt16 address, uInt8 value);

  private:
    /**
      Install the specified slice for segment zero.

      @param slice  The slice to map into the segment
    */
    void segmentZero(uInt8 slice);

    /**
      Install the specified slice for segment one.

      @param slice  The slice to map into the segment
    */
    void segmentOne(uInt8 slice);

    /**
      Install the specified slice for segment two.

      @param slice  The slice to map into the segment
    */
    void segmentTwo(uInt8 slice);

    /**
      Install the specified slice for segment three.
      Note that this method also takes care of mapping extra 3 bytes.

      @param slice      The slice to map into the segment
      @param map3bytes  Whether to map in an extra 3 bytes
    */
    void segmentThree(uInt8 slice, bool map3bytes);

  private:
    // Indicates which bank is currently active
    uInt16 myCurrentBank;

    // Indicates the actual size of the ROM image (either 8K or 8K + 3)
    uInt32 mySize;

    // The 8K ROM image of the cartridge
    uInt8 myImage[8195];

    // The 64 bytes RAM of the cartridge
    uInt8 myRAM[64];

    // The 1K ROM mirror of segment 3 (sometimes contains extra 3 bytes)
    uInt8 mySegment3[1024];

    // Indicates the offset for each of the four segments
    uInt16 myOffset[4];

    // Indicates the cycle at which a bankswitch was initiated
    uInt32 myCyclesAtBankswitchInit;

    // Indicates the bank we wish to switch to in the future
    uInt16 myPendingBank;

    // The arrangement of banks to use on each hotspot read
    struct BankOrg {
      uInt8 zero, one, two, three;
      bool map3bytes;
    };
    static BankOrg ourBankOrg[16];
};

#endif
