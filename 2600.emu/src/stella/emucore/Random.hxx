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

#ifndef RANDOM_HXX
#define RANDOM_HXX

#include "bspf.hxx"
#include "OSystem.hxx"
#include "Serializable.hxx"

/**
  This is a quick-and-dirty random number generator.  It is based on
  information in Chapter 7 of "Numerical Recipes in C".  It's a simple
  linear congruential generator.

  @author  Bradford W. Mott
*/
class Random : public Serializable
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
      myValue = uInt32(myOSystem.getTicks());
    }

    /**
      Answer the next random number from the random number generator

      @return A random number
    */
    uInt32 next() const
    {
      return (myValue = (myValue * 2416 + 374441) % 1771875);
    }

    /**
      Save the current state of this device to the given Serializer.

      @param out  The Serializer object to use
      @return  False on any errors, else true
    */
    bool save(Serializer& out) const override
    {
      try
      {
        out.putString(name());
        out.putInt(myValue);
      }
      catch(...)
      {
        cerr << "ERROR: Random::save" << endl;
        return false;
      }

      return true;
    }

    /**
      Load the current state of this device from the given Serializer.

      @param in  The Serializer object to use
      @return  False on any errors, else true
    */
    bool load(Serializer& in) override
    {
      try
      {
        if(in.getString() != name())
          return false;

        myValue = in.getInt();
      }
      catch(...)
      {
        cerr << "ERROR: Random::load" << endl;
        return false;
      }

      return true;
    }

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const override { return "Random"; }

  private:
    // Set the OSystem we're using
    const OSystem& myOSystem;

    // Indicates the next random number
    // We make this mutable, since it's not immediately obvious that
    // calling next() should change internal state (ie, the *logical*
    // state of the object shouldn't change just by asking for another
    // random number)
    mutable uInt32 myValue;

  private:
    // Following constructors and assignment operators not supported
    Random() = delete;
    Random(const Random&) = delete;
    Random(Random&&) = delete;
    Random& operator=(const Random&) = delete;
    Random& operator=(Random&&) = delete;
};

#endif
