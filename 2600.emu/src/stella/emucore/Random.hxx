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
// $Id: Random.hxx 3131 2015-01-01 03:49:32Z stephena $
//============================================================================

#ifndef RANDOM_HXX
#define RANDOM_HXX

#include <time.h>

#include "bspf.hxx"
#include "OSystem.hxx"

/**
  This is a quick-and-dirty random number generator.  It is based on 
  information in Chapter 7 of "Numerical Recipes in C".  It's a simple 
  linear congruential generator.

  @author  Bradford W. Mott
  @version $Id: Random.hxx 3131 2015-01-01 03:49:32Z stephena $
*/
class Random
{
  public:
    /**
      Create a new random number generator
    */
    Random(const OSystem& osystem) : myOSystem(osystem) { initSeed(); }
    
    /**
      Re-initialize the random number generator with a new seed,
      to generate a different set of random numbers.
    */
    void initSeed()
    {
      myValue = (uInt32) myOSystem.getTicks();
    }

    /**
      Answer the next random number from the random number generator

      @return A random number
    */
    uInt32 next()
    {
      return (myValue = (myValue * 2416 + 374441) % 1771875);
    }

  private:
    // Set the OSystem we're using
    const OSystem& myOSystem;

    // Indicates the next random number
    uInt32 myValue;
};

#endif
