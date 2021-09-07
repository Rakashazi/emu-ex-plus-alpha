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

#ifndef CARTRIDGEE7_HXX
#define CARTRIDGEE7_HXX

class System;

#include "bspf.hxx"
#include "Cart.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "CartE7Widget.hxx"
#endif
#include "CartMNetwork.hxx"

/**
  This is the cartridge class for 16K M-Network bankswitched games.

  @author  Bradford W. Mott, Thomas Jentzsch
*/
class CartridgeE7 : public CartridgeMNetwork
{
  public:
    /**
      Create a new cartridge using the specified image

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param md5       The md5sum of the ROM image
      @param settings  A reference to the various settings (read-only)
    */
    CartridgeE7(const ByteBuffer& image, size_t size, const string& md5,
                const Settings& settings);
    ~CartridgeE7() override = default;

  public:
    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const override { return "CartridgeE7"; }

  #ifdef DEBUGGER_SUPPORT
    /**
      Get debugger widget responsible for accessing the inner workings
      of the cart.
    */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
        const GUI::Font& nfont, int x, int y, int w, int h) override
    {
      return new CartridgeE7Widget(boss, lfont, nfont, x, y, w, h, *this);
    }
  #endif

  private:
    /**
      Check hotspots and switch bank if triggered.
    */
    void checkSwitchBank(uInt16 address) override;

  private:
    // Following constructors and assignment operators not supported
    CartridgeE7() = delete;
    CartridgeE7(const CartridgeE7&) = delete;
    CartridgeE7(CartridgeE7&&) = delete;
    CartridgeE7& operator=(const CartridgeE7&) = delete;
    CartridgeE7& operator=(CartridgeE7&&) = delete;
};

#endif
