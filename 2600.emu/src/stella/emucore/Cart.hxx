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

#ifndef CARTRIDGE_HXX
#define CARTRIDGE_HXX

class Cartridge;
class Properties;
class CartDebugWidget;
class CartRamWidget;
class GuiObject;

#include "bspf.hxx"
#include "Device.hxx"
#include "Settings.hxx"
#include "Font.hxx"

/**
  A cartridge is a device which contains the machine code for a
  game and handles any bankswitching performed by the cartridge.
  A 'bank' is defined as a 4K block that is visible in the
  0x1000-0x2000 area (or its mirrors).

  @author  Bradford W. Mott
*/
class Cartridge : public Device
{
  public:
    /**
      Create a new cartridge

      @param settings  A reference to the various settings (read-only)
    */
    Cartridge(const Settings& settings);
    virtual ~Cartridge() = default;

    /**
      Set/query some information about this cartridge.
    */
    void setAbout(const string& about, const string& type, const string& id);
    const string& about() const { return myAbout; }
    const string& detectedType() const { return myDetectedType; }
    const string& multiCartID() const  { return myMultiCartID;  }

    /**
      Save the internal (patched) ROM image.

      @param out  The output file stream to save the image
    */
    bool saveROM(ofstream& out) const;

    /**
      Lock/unlock bankswitching capability.  The debugger will lock
      the banks before querying the cart state, otherwise reading values
      could inadvertantly cause a bankswitch to occur.
    */
    void lockBank()   { myBankLocked = true;  }
    void unlockBank() { myBankLocked = false; }
    bool bankLocked() const { return myBankLocked; }

    /**
      Get the default startup bank for a cart.  This is the bank where
      the system will look at address 0xFFFC to determine where to
      start running code.

      @return  The startup bank
    */
    uInt16 startBank() const { return myStartBank; }

    /**
      Answer whether the bank has changed since the last time this
      method was called.  Each cart class is able to override this
      method to deal with its specific functionality.  In those cases,
      the derived class is still responsible for calling this base
      function.

      @return  Whether the bank was changed
    */
    virtual bool bankChanged();

  public:
    //////////////////////////////////////////////////////////////////////
    // The following methods are cart-specific and will usually be
    // implemented in derived classes.  Carts which don't support
    // bankswitching (for any reason) do not have to provide an
    // implementation for bankswitch-related methods.
    //////////////////////////////////////////////////////////////////////
    /**
      Set the specified bank.  This is used only when the bankswitching
      scheme defines banks in a standard format (ie, 0 for first bank,
      1 for second, etc).  Carts which will handle their own bankswitching
      completely or non-bankswitched carts can ignore this method.
    */
    virtual bool bank(uInt16) { return false; }

    /**
      Get the current bank.  Carts which have only one bank (either real
      or virtual) always report that bank as zero.
    */
    virtual uInt16 getBank() const { return 0; }

    /**
      Query the number of 'banks' supported by the cartridge.  Note that
      this information is cart-specific, where each cart basically defines
      what a 'bank' is.

      For the normal Atari-manufactured carts, a standard bank is a 4K
      block that is directly accessible in the 4K address space.  In other
      cases where ROMs have 2K blocks in some preset area, the bankCount
      is the number of such blocks.  Finally, in some esoteric schemes,
      the number of ways that the addressing can change (multiple ROM and
      RAM slices at multiple access points) is so complicated that the
      cart will report having only one 'virtual' bank.
    */
    virtual uInt16 bankCount() const { return 1; }

    /**
      Patch the cartridge ROM.

      @param address  The ROM address to patch
      @param value    The value to place into the address
      @return    Success or failure of the patch operation
    */
    virtual bool patch(uInt16 address, uInt8 value) = 0;

    /**
      Access the internal ROM image for this cartridge.

      @param size  Set to the size of the internal ROM image data
      @return  A pointer to the internal ROM image data
    */
    virtual const uInt8* getImage(uInt32& size) const = 0;

    /**
      Informs the cartridge about the name of the ROM file used when
      creating this cart.

      @param name  The properties file name of the ROM
    */
    virtual void setRomName(const string& name) { }

    /**
      Thumbulator only supports 16-bit ARM code.  Some Harmony/Melody drivers,
      such as BUS and CDF, feature 32-bit ARM code subroutines.  This is used
      to pass values back to the cartridge class to emulate those subroutines.
    */
    virtual uInt32 thumbCallback(uInt8 function, uInt32 value1, uInt32 value2) { return 0; }

    /**
      Get debugger widget responsible for accessing the inner workings
      of the cart.  This will need to be overridden and implemented by
      each specific cart type, since the bankswitching/inner workings
      of each cart type can be very different from each other.
    */
    virtual CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
        const GUI::Font& nfont, int x, int y, int w, int h) { return nullptr; }

  protected:
    /**
      Indicate that an illegal read from a write port has occurred.

      @param address  The address of the illegal read
    */
    void triggerReadFromWritePort(uInt16 address);

    /**
      Create an array that holds code-access information for every byte
      of the ROM (indicated by 'size').  Note that this is only used by
      the debugger, and is unavailable otherwise.

      @param size  The size of the code-access array to create
    */
    void createCodeAccessBase(uInt32 size);

    /**
      Fill the given RAM array with (possibly random) data.

      @param arr  Pointer to the RAM array
      @param size The size of the RAM array
      @param val  If provided, the value to store in the RAM array
    */
    void initializeRAM(uInt8* arr, uInt32 size, uInt8 val = 0) const;

    /**
      Checks if initial RAM randomization is enabled

      @return Whether the initial RAM should be randomized
    */
    bool randomInitialRAM() const;

    /**
      Defines the startup bank. if 'bank' is negative, a random bank will
      be selected.
    */
    void randomizeStartBank();

    /**
      Checks if startup bank randomization is enabled

      @return Whether the startup bank(s) should be randomized
    */
    bool randomStartBank() const;

  protected:
    // Settings class for the application
    const Settings& mySettings;

    // The startup bank to use (where to look for the reset vector address)
    uInt16 myStartBank;

    // Indicates if the bank has changed somehow (a bankswitch has occurred)
    bool myBankChanged;

    // The array containing information about every byte of ROM indicating
    // whether it is used as code.
    BytePtr myCodeAccessBase;

  private:
    // If myBankLocked is true, ignore attempts at bankswitching. This is used
    // by the debugger, when disassembling/dumping ROM.
    bool myBankLocked;

    // Contains various info about this cartridge
    // This needs to be stored separately from child classes, since
    // sometimes the information in both do not match
    // (ie, detected type could be '2in1' while name of cart is '4K')
    string myAbout, myDetectedType, myMultiCartID;

    // Following constructors and assignment operators not supported
    Cartridge() = delete;
    Cartridge(const Cartridge&) = delete;
    Cartridge(Cartridge&&) = delete;
    Cartridge& operator=(const Cartridge&) = delete;
    Cartridge& operator=(Cartridge&&) = delete;
};

#endif
