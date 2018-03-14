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

#ifndef MENU_HXX
#define MENU_HXX

class OSystem;

#include "DialogContainer.hxx"

/**
  The base dialog for all configuration menus in Stella.

  @author  Stephen Anthony
*/
class Menu : public DialogContainer
{
  public:
    /**
      Create a new menu stack
    */
    Menu(OSystem& osystem);
    virtual ~Menu() = default;

  private:
    // Following constructors and assignment operators not supported
    Menu() = delete;
    Menu(const Menu&) = delete;
    Menu(Menu&&) = delete;
    Menu& operator=(const Menu&) = delete;
    Menu& operator=(Menu&&) = delete;
};

#endif
