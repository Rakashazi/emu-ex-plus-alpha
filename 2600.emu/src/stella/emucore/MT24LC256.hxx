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
// $Id: MT24LC256.hxx 3131 2015-01-01 03:49:32Z stephena $
//============================================================================

#ifndef MT24LC256_HXX
#define MT24LC256_HXX

class Controller;
class System;

#include "bspf.hxx"

/**
  Emulates a Microchip Technology Inc. 24LC256, a 32KB Serial Electrically
  Erasable PROM accessed using the I2C protocol.  Thanks to J. Payson
  (aka Supercat) for the bulk of this code.

  @author  Stephen Anthony & J. Payson
  @version $Id: MT24LC256.hxx 3131 2015-01-01 03:49:32Z stephena $
*/
class MT24LC256
{
  public:
    /**
      Create a new 24LC256 with its data stored in the given file

      @param filename Data file containing the EEPROM data
      @param system   The system using the controller of this device
    */
    MT24LC256(const string& filename, const System& system);
 
    /**
      Destructor
    */
    ~MT24LC256();

  public:
    /** Read boolean data from the SDA line */
    bool readSDA() const;

    /** Write boolean data to the SDA and SCL lines */
    void writeSDA(bool state);
    void writeSCL(bool state);

    /** Erase entire EEPROM to known state ($FF) */
    void erase();

    /**
      Notification method invoked by the system right before the
      system resets its cycle counter to zero.  It may be necessary 
      to override this method for devices that remember cycle counts.
    */
    void systemCyclesReset();

  private:
    // I2C access code provided by Supercat
    void jpee_init();
    void jpee_data_start();
    void jpee_data_stop();
    void jpee_clock_fall();
    int  jpee_logproc(char const *st);
    bool jpee_timercheck(int mode);

    void update();

  private:
    // The system of the parent controller
    const System& mySystem;

    // The EEPROM data
    uInt8 myData[32768];

    // Cached state of the SDA and SCL pins on the last write
    bool mySDA, mySCL;

    // Indicates that a timer has been set and hasn't expired yet
    bool myTimerActive;

    // Indicates when the timer was set
    uInt32 myCyclesWhenTimerSet;

    // Indicates when the SDA and SCL pins were set/written
    uInt32 myCyclesWhenSDASet, myCyclesWhenSCLSet;

    // The file containing the EEPROM data
    string myDataFile;

    // Indicates if a valid EEPROM data file exists/was successfully loaded
    bool myDataFileExists;

    // Indicates if the EEPROM has changed since class invocation
    bool myDataChanged;

    // Required for I2C functionality
    Int32 jpee_mdat, jpee_sdat, jpee_mclk;
    Int32 jpee_sizemask, jpee_pagemask, jpee_smallmode, jpee_logmode;
    Int32 jpee_pptr, jpee_state, jpee_nb;
    uInt32 jpee_address, jpee_ad_known;
    uInt8 jpee_packet[70];

  private:
    // Copy constructor and assignment operator not supported
    MT24LC256(const MT24LC256&);
    MT24LC256& operator = (const MT24LC256&);
};

#endif
