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
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: SaveKey.hxx 3258 2016-01-23 22:56:16Z stephena $
//============================================================================

#ifndef SAVEKEY_HXX
#define SAVEKEY_HXX

#include "Control.hxx"
#include "MT24LC256.hxx"

/**
  Richard Hutchinson's SaveKey "controller", consisting of a 32KB EEPROM
  accessible using the I2C protocol.

  This code owes a great debt to Alex Herbert's AtariVox documentation and
  driver code.

  @author  Stephen Anthony
  @version $Id: SaveKey.hxx 3258 2016-01-23 22:56:16Z stephena $
*/
class SaveKey : public Controller
{
  friend class SaveKeyWidget;

  public:
    /**
      Create a new SaveKey controller plugged into the specified jack

      @param jack       The jack the controller is plugged into
      @param event      The event object to use for events
      @param system     The system using this controller
      @param eepromfile The file containing the EEPROM data
    */
    SaveKey(Jack jack, const Event& event, const System& system,
            const string& eepromfile);
    virtual ~SaveKey() = default;

  public:
    using Controller::read;

    /**
      Read the value of the specified digital pin for this controller.

      @param pin The pin of the controller jack to read
      @return The state of the pin
    */
    bool read(DigitalPin pin) override;

    /**
      Write the given value to the specified digital pin for this
      controller.  Writing is only allowed to the pins associated
      with the PIA.  Therefore you cannot write to pin six.

      @param pin The pin of the controller jack to write to
      @param value The value to write to the pin
    */
    void write(DigitalPin pin, bool value) override;

    /**
      Update the entire digital and analog pin state according to the
      events currently set.
    */
    void update() override { }

    /**
      Notification method invoked by the system right before the
      system resets its cycle counter to zero.  It may be necessary 
      to override this method for devices that remember cycle counts.
    */
    void systemCyclesReset() override;

  private:
    // The EEPROM used in the SaveKey
    unique_ptr<MT24LC256> myEEPROM;

  private:
    // Following constructors and assignment operators not supported
    SaveKey() = delete;
    SaveKey(const SaveKey&) = delete;
    SaveKey(SaveKey&&) = delete;
    SaveKey& operator=(const SaveKey&) = delete;
    SaveKey& operator=(SaveKey&&) = delete;
};

#endif
