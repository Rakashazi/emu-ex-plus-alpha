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

#include "Event.hxx"
#include "Props.hxx"
#include "Settings.hxx"
#include "Switches.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Switches::Switches(const Event& event, const Properties& properties,
                   const Settings& settings)
  : myEvent(event),
    mySwitches(0xFF),
    myIs7800(false)
{
  if(properties.get(Console_RightDifficulty) == "B")
  {
    mySwitches &= ~0x80;
  }
  else
  {
    mySwitches |= 0x80;
  }

  if(properties.get(Console_LeftDifficulty) == "B")
  {
    mySwitches &= ~0x40;
  }
  else
  {
    mySwitches |= 0x40;
  }

  if(properties.get(Console_TelevisionType) == "COLOR")
  {
    mySwitches |= 0x08;
  }
  else
  {
    mySwitches &= ~0x08;
  }

  toggle7800Mode(settings);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Switches::update()
{
  if(myIs7800)
  {
    if(myEvent.get(Event::Console7800Pause) != 0)
    {
      mySwitches &= ~0x08;
    }
    else
    {
      mySwitches |= 0x08;
    }
  }
  else
  {
    if(myEvent.get(Event::ConsoleColor) != 0)
    {
      mySwitches |= 0x08;
    }
    else if(myEvent.get(Event::ConsoleBlackWhite) != 0)
    {
      mySwitches &= ~0x08;
    }
  }

  if(myEvent.get(Event::ConsoleRightDiffA) != 0)
  {
    mySwitches |= 0x80;
  }
  else if(myEvent.get(Event::ConsoleRightDiffB) != 0)
  {
    mySwitches &= ~0x80;
  }

  if(myEvent.get(Event::ConsoleLeftDiffA) != 0)
  {
    mySwitches |= 0x40;
  }
  else if(myEvent.get(Event::ConsoleLeftDiffB) != 0)
  {
    mySwitches &= ~0x40;
  }

  if(myEvent.get(Event::ConsoleSelect) != 0)
  {
    mySwitches &= ~0x02;
  }
  else
  {
    mySwitches |= 0x02;
  }

  if(myEvent.get(Event::ConsoleReset) != 0)
  {
    mySwitches &= ~0x01;
  }
  else
  {
    mySwitches |= 0x01;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Switches::save(Serializer& out) const
{
  try
  {
    out.putByte(mySwitches);
  }
  catch(...)
  {
    cerr << "ERROR: Switches::save() exception\n";
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Switches::load(Serializer& in)
{
  try
  {
    mySwitches = in.getByte();
  }
  catch(...)
  {
    cerr << "ERROR: Switches::load() exception\n";
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Switches::toggle7800Mode(const Settings& settings)
{
  bool devSettings = settings.getBool("dev.settings");
  myIs7800 = devSettings && (settings.getString("dev.console") == "7800");

  return myIs7800;
}
