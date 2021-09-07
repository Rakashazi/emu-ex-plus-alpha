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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef CARTRIDGE_E78K_HXX
#define CARTRIDGE_E78K_HXX

#include "bspf.hxx"
#include "Cart.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "CartE78KWidget.hxx"
#endif
#include "CartMNetwork.hxx"

/**
  This is the cartridge class for 8K M-Network bankswitched games.

  @author  Bradford W. Mott, Thomas Jentzsch
*/
class CartridgeE78K : public CartridgeMNetwork
{
  public:
    /**
      Create a new cartridge using the specified image

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param md5       The md5sum of the ROM image
      @param settings  A reference to the various settings (read-only)
    */
    CartridgeE78K(const ByteBuffer& image, size_t size, const string& md5,
                  const Settings& settings);
    ~CartridgeE78K() override = default;

  public:
    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const override { return "CartridgeE78K"; }

  #ifdef DEBUGGER_SUPPORT
    /**
      Get debugger widget responsible for accessing the inner workings
      of the cart.
      */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
                                 const GUI::Font& nfont, int x, int y, int w, int h) override
    {
      return new CartridgeE78KWidget(boss, lfont, nfont, x, y, w, h, *this);
    }
  #endif

  private:
    /**
      Check hotspots and switch bank if triggered.
    */
    void checkSwitchBank(uInt16 address) override;

  private:
    // Following constructors and assignment operators not supported
    CartridgeE78K() = delete;
    CartridgeE78K(const CartridgeE78K&) = delete;
    CartridgeE78K(CartridgeE78K&&) = delete;
    CartridgeE78K& operator=(const CartridgeE78K&) = delete;
    CartridgeE78K& operator=(CartridgeE78K&&) = delete;
};

#endif
