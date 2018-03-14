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

#ifndef MT24LC256_HXX
#define MT24LC256_HXX

class Controller;
class System;

#include "bspf.hxx"

#ifdef PAGE_SIZE
#undef PAGE_SIZE
#endif

/**
  Emulates a Microchip Technology Inc. 24LC256, a 32KB Serial Electrically
  Erasable PROM accessed using the I2C protocol.  Thanks to J. Payson
  (aka Supercat) for the bulk of this code.

  @author  Stephen Anthony & J. Payson
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
    ~MT24LC256();

  private:
    // Sizes of the EEPROM
    static constexpr uInt32 FLASH_SIZE = 32 * 1024;

  public:
    static constexpr uInt32 PAGE_SIZE = 64;
    static constexpr uInt32 PAGE_NUM = FLASH_SIZE / PAGE_SIZE;

    /** Read boolean data from the SDA line */
    bool readSDA() const { return jpee_mdat && jpee_sdat; }

    /** Write boolean data to the SDA and SCL lines */
    void writeSDA(bool state);
    void writeSCL(bool state);

    /** Called when the system is being reset */
    void systemReset();

    /** Erase entire EEPROM to known state ($FF) */
    void eraseAll();

    /** Erase the pages used by the current ROM to known state ($FF) */
    void eraseCurrent();

    /** Returns true if the page is used by the current ROM */
    bool isPageUsed(uInt32 page) const;

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
    // Inital state value of flash EEPROM
    static constexpr uInt8 INIT_VALUE = 0xff;

    // The system of the parent controller
    const System& mySystem;

    // The EEPROM data
    uInt8 myData[FLASH_SIZE];

    // Track which pages are used
    bool myPageHit[PAGE_NUM];

    // Cached state of the SDA and SCL pins on the last write
    bool mySDA, mySCL;

    // Indicates that a timer has been set and hasn't expired yet
    bool myTimerActive;

    // Indicates when the timer was set
    uInt64 myCyclesWhenTimerSet;

    // Indicates when the SDA and SCL pins were set/written
    uInt64 myCyclesWhenSDASet, myCyclesWhenSCLSet;

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
    // Following constructors and assignment operators not supported
    MT24LC256() = delete;
    MT24LC256(const MT24LC256&) = delete;
    MT24LC256(MT24LC256&&) = delete;
    MT24LC256& operator=(const MT24LC256&) = delete;
    MT24LC256& operator=(MT24LC256&&) = delete;
};

#endif
