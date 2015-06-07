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
// $Id: CompuMate.cxx 3131 2015-01-01 03:49:32Z stephena $
//============================================================================

#include "Control.hxx"
#include "StellaKeys.hxx"
#include "CompuMate.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompuMate::CompuMate(const Console& console, const Event& event,
                     const System& system)
  : myConsole(console),
    myEvent(event)
{
  // These controller pointers will be retrieved by the Console, which will
  // also take ownership of them
  myLeftController  = make_ptr<CMControl>(*this, Controller::Left, event, system);
  myRightController = make_ptr<CMControl>(*this, Controller::Right, event, system);

  myLeftController->myAnalogPinValue[Controller::Nine] = Controller::maximumResistance;
  myLeftController->myAnalogPinValue[Controller::Five] = Controller::minimumResistance;
  myRightController->myAnalogPinValue[Controller::Nine] = Controller::minimumResistance;
  myRightController->myAnalogPinValue[Controller::Five] = Controller::maximumResistance;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompuMate::enableKeyHandling(bool enable)
{
  if(enable)
    myKeyTable = myEvent.getKeys();
  else
  {
    for(uInt32 i = 0; i < KBDK_LAST; ++i)
      myInternalKeyTable[i] = false;

    myKeyTable = myInternalKeyTable;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompuMate::update()
{
  // Handle SWCHA changes - the following comes almost directly from z26
  Controller& lp = myConsole.leftController();
  Controller& rp = myConsole.rightController();

  lp.myAnalogPinValue[Controller::Nine] = Controller::maximumResistance;
  lp.myAnalogPinValue[Controller::Five] = Controller::minimumResistance;
  lp.myDigitalPinState[Controller::Six] = true;
  rp.myAnalogPinValue[Controller::Nine] = Controller::minimumResistance;
  rp.myAnalogPinValue[Controller::Five] = Controller::maximumResistance;
  rp.myDigitalPinState[Controller::Six] = true;

  if (myKeyTable[KBDK_LSHIFT] || myKeyTable[KBDK_RSHIFT])
    rp.myAnalogPinValue[Controller::Five] = Controller::minimumResistance;
  if (myKeyTable[KBDK_LCTRL] || myKeyTable[KBDK_RCTRL])
    lp.myAnalogPinValue[Controller::Nine] = Controller::minimumResistance;

  rp.myDigitalPinState[Controller::Three] = true;
  rp.myDigitalPinState[Controller::Four] = true;

  switch(myColumn)
  {
    case 0:
      if (myKeyTable[KBDK_7]) lp.myDigitalPinState[Controller::Six] = false;
      if (myKeyTable[KBDK_U]) rp.myDigitalPinState[Controller::Three] = false;
      if (myKeyTable[KBDK_J]) rp.myDigitalPinState[Controller::Six] = false;
      if (myKeyTable[KBDK_M]) rp.myDigitalPinState[Controller::Four] = false;
      break;
    case 1:
      if (myKeyTable[KBDK_6]) lp.myDigitalPinState[Controller::Six] = false;
      // Emulate the '?' character (Shift-6) with the actual question key
      if (myKeyTable[KBDK_SLASH] && (myKeyTable[KBDK_LSHIFT] || myKeyTable[KBDK_RSHIFT]))
      {
        rp.myAnalogPinValue[Controller::Five] = Controller::minimumResistance;
        lp.myDigitalPinState[Controller::Six] = false;
      }
      if (myKeyTable[KBDK_Y]) rp.myDigitalPinState[Controller::Three] = false;
      if (myKeyTable[KBDK_H]) rp.myDigitalPinState[Controller::Six] = false;
      if (myKeyTable[KBDK_N]) rp.myDigitalPinState[Controller::Four] = false;
      break;
    case 2:
      if (myKeyTable[KBDK_8]) lp.myDigitalPinState[Controller::Six] = false;
      // Emulate the '[' character (Shift-8) with the actual key
      if (myKeyTable[KBDK_LEFTBRACKET] && !(myKeyTable[KBDK_LSHIFT] || myKeyTable[KBDK_RSHIFT]))
      {
        rp.myAnalogPinValue[Controller::Five] = Controller::minimumResistance;
        lp.myDigitalPinState[Controller::Six] = false;
      }
      if (myKeyTable[KBDK_I]) rp.myDigitalPinState[Controller::Three] = false;
      if (myKeyTable[KBDK_K]) rp.myDigitalPinState[Controller::Six] = false;
      if (myKeyTable[KBDK_COMMA]) rp.myDigitalPinState[Controller::Four] = false;
      break;
    case 3:
      if (myKeyTable[KBDK_2]) lp.myDigitalPinState[Controller::Six] = false;
      // Emulate the '-' character (Shift-2) with the actual minus key
      if (myKeyTable[KBDK_MINUS] && !(myKeyTable[KBDK_LSHIFT] || myKeyTable[KBDK_RSHIFT]))
      {
        rp.myAnalogPinValue[Controller::Five] = Controller::minimumResistance;
        lp.myDigitalPinState[Controller::Six] = false;
      }
      if (myKeyTable[KBDK_W]) rp.myDigitalPinState[Controller::Three] = false;
      if (myKeyTable[KBDK_S]) rp.myDigitalPinState[Controller::Six] = false;
      if (myKeyTable[KBDK_X]) rp.myDigitalPinState[Controller::Four] = false;
      break;
    case 4:
      if (myKeyTable[KBDK_3]) lp.myDigitalPinState[Controller::Six] = false;
      if (myKeyTable[KBDK_E]) rp.myDigitalPinState[Controller::Three] = false;
      if (myKeyTable[KBDK_D]) rp.myDigitalPinState[Controller::Six] = false;
      if (myKeyTable[KBDK_C]) rp.myDigitalPinState[Controller::Four] = false;
      break;
    case 5:
      if (myKeyTable[KBDK_0]) lp.myDigitalPinState[Controller::Six] = false;
      // Emulate the quote character (Shift-0) with the actual quote key
      if (myKeyTable[KBDK_APOSTROPHE] && (myKeyTable[KBDK_LSHIFT] || myKeyTable[KBDK_RSHIFT]))
      {
        rp.myAnalogPinValue[Controller::Five] = Controller::minimumResistance;
        lp.myDigitalPinState[Controller::Six] = false;
      }
      if (myKeyTable[KBDK_P]) rp.myDigitalPinState[Controller::Three] = false;
      if (myKeyTable[KBDK_RETURN] || myKeyTable[KBDK_KP_ENTER])
        rp.myDigitalPinState[Controller::Six] = false;
      if (myKeyTable[KBDK_SPACE]) rp.myDigitalPinState[Controller::Four] = false;
      // Emulate Ctrl-space (aka backspace) with the actual Backspace key
      if (myKeyTable[KBDK_BACKSPACE])
      {
        lp.myAnalogPinValue[Controller::Nine] = Controller::minimumResistance;
        rp.myDigitalPinState[Controller::Four] = false;
      }
      break;
    case 6:
      if (myKeyTable[KBDK_9]) lp.myDigitalPinState[Controller::Six] = false;
      // Emulate the ']' character (Shift-9) with the actual key
      if (myKeyTable[KBDK_RIGHTBRACKET] && !(myKeyTable[KBDK_LSHIFT] || myKeyTable[KBDK_RSHIFT]))
      {
        rp.myAnalogPinValue[Controller::Five] = Controller::minimumResistance;
        lp.myDigitalPinState[Controller::Six] = false;
      }
      if (myKeyTable[KBDK_O]) rp.myDigitalPinState[Controller::Three] = false;
      if (myKeyTable[KBDK_L]) rp.myDigitalPinState[Controller::Six] = false;
      if (myKeyTable[KBDK_PERIOD]) rp.myDigitalPinState[Controller::Four] = false;
      break;
    case 7:
      if (myKeyTable[KBDK_5]) lp.myDigitalPinState[Controller::Six] = false;
      // Emulate the '=' character (Shift-5) with the actual equals key
      if (myKeyTable[KBDK_EQUALS] && !(myKeyTable[KBDK_LSHIFT] || myKeyTable[KBDK_RSHIFT]))
      {
        rp.myAnalogPinValue[Controller::Five] = Controller::minimumResistance;
        lp.myDigitalPinState[Controller::Six] = false;
      }
      if (myKeyTable[KBDK_T]) rp.myDigitalPinState[Controller::Three] = false;
      if (myKeyTable[KBDK_G]) rp.myDigitalPinState[Controller::Six] = false;
      if (myKeyTable[KBDK_B]) rp.myDigitalPinState[Controller::Four] = false;
      break;
    case 8:
      if (myKeyTable[KBDK_1]) lp.myDigitalPinState[Controller::Six] = false;
      // Emulate the '+' character (Shift-1) with the actual plus key (Shift-=)
      if (myKeyTable[KBDK_EQUALS] && (myKeyTable[KBDK_LSHIFT] || myKeyTable[KBDK_RSHIFT]))
      {
        rp.myAnalogPinValue[Controller::Five] = Controller::minimumResistance;
        lp.myDigitalPinState[Controller::Six] = false;
      }
      if (myKeyTable[KBDK_Q]) rp.myDigitalPinState[Controller::Three] = false;
      if (myKeyTable[KBDK_A]) rp.myDigitalPinState[Controller::Six] = false;
      if (myKeyTable[KBDK_Z]) rp.myDigitalPinState[Controller::Four] = false;
      break;
    case 9:
      if (myKeyTable[KBDK_4]) lp.myDigitalPinState[Controller::Six] = false;
      // Emulate the '/' character (Shift-4) with the actual slash key
      if (myKeyTable[KBDK_SLASH] && !(myKeyTable[KBDK_LSHIFT] || myKeyTable[KBDK_RSHIFT]))
      {
        rp.myAnalogPinValue[Controller::Five] = Controller::minimumResistance;
        lp.myDigitalPinState[Controller::Six] = false;
      }
      if (myKeyTable[KBDK_R]) rp.myDigitalPinState[Controller::Three] = false;
      if (myKeyTable[KBDK_F]) rp.myDigitalPinState[Controller::Six] = false;
      if (myKeyTable[KBDK_V]) rp.myDigitalPinState[Controller::Four] = false;
      break;
    default:
      break;
  }
}
