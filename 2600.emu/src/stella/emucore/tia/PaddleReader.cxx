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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <cmath>

#include "PaddleReader.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PaddleReader::PaddleReader()
{
  setConsoleTiming(ConsoleTiming::ntsc);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PaddleReader::reset(uInt64 timestamp)
{
  myU = 0;
  myIsDumped = false;

  myValue = 0;
  myTimestamp = timestamp;

  setConsoleTiming(ConsoleTiming::ntsc);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PaddleReader::vblank(uInt8 value, uInt64 timestamp)
{
  bool oldIsDumped = myIsDumped;

  if (value & 0x80) {
    myIsDumped = true;
    myU = 0;
    myTimestamp = timestamp;
  } else if (oldIsDumped) {
    myIsDumped = false;
    myTimestamp = timestamp;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 PaddleReader::inpt(uInt64 timestamp)
{
  updateCharge(timestamp);

  bool state = myIsDumped ? false : myU > myUThresh;

  return state ? 0x80 : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PaddleReader::update(double value, uInt64 timestamp, ConsoleTiming consoleTiming)
{
  if (consoleTiming != myConsoleTiming) {
    setConsoleTiming(consoleTiming);
  }

  if (value != myValue) {
    myValue = value;

    if (myValue < 0) {
      // value < 0 signifies either maximum resistance OR analog input connected to
      // ground (keyboard controllers). As we have no way to tell these apart we just
      // assume ground and discharge.
      myU = 0;
      myTimestamp = timestamp;
    } else {
      updateCharge(timestamp);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PaddleReader::setConsoleTiming(ConsoleTiming consoleTiming)
{
  myConsoleTiming = consoleTiming;

  myClockFreq = myConsoleTiming == ConsoleTiming::ntsc ? 60 * 228 * 262 : 50 * 228 * 312;
  myUThresh = USUPP * (1. - exp(-TRIPPOINT_LINES * 228 / myClockFreq  / (RPOT + R0) / C));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PaddleReader::updateCharge(uInt64 timestamp)
{
  if (myIsDumped) return;

  if (myValue >= 0)
    myU = USUPP * (1 - (1 - myU / USUPP) *
      exp(-static_cast<double>(timestamp - myTimestamp) / (myValue * RPOT + R0) / C / myClockFreq));

  myTimestamp = timestamp;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PaddleReader::save(Serializer& out) const
{
  try
  {
    out.putDouble(myUThresh);
    out.putDouble(myU);

    out.putDouble(myValue);
    out.putLong(myTimestamp);

    out.putInt(int(myConsoleTiming));
    out.putDouble(myClockFreq);

    out.putBool(myIsDumped);
  }
  catch(...)
  {
    cerr << "ERROR: TIA_PaddleReader::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PaddleReader::load(Serializer& in)
{
  try
  {
    myUThresh = in.getDouble();
    myU = in.getDouble();

    myValue = in.getDouble();
    myTimestamp = in.getLong();

    myConsoleTiming = ConsoleTiming(in.getInt());
    myClockFreq = in.getDouble();

    myIsDumped = in.getBool();
  }
  catch(...)
  {
    cerr << "ERROR: TIA_PaddleReader::load" << endl;
    return false;
  }

  return true;
}
