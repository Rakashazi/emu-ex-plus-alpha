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

#include "JitterEmulation.hxx"

enum Metrics: uInt32 {
  framesForStableHeight       = 2,
  framesUntilDestabilization  = 10,
  minDeltaForJitter           = 3,
  maxJitter                   = 50
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
JitterEmulation::JitterEmulation()
{
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JitterEmulation::reset()
{
  myLastFrameScanlines = 0;
  myStableFrameFinalLines = -1;
  myStableFrames = 0;
  myStabilizationCounter = 0;
  myDestabilizationCounter = 0;
  myJitter = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JitterEmulation::frameComplete(uInt32 scanlineCount)
{
  if (Int32(scanlineCount) != myStableFrameFinalLines) {
    if (myDestabilizationCounter++ > Metrics::framesUntilDestabilization) myStableFrameFinalLines = -1;

    if (scanlineCount == myLastFrameScanlines) {

      if (++myStabilizationCounter >= Metrics::framesForStableHeight) {
        if (myStableFrameFinalLines > 0) updateJitter(scanlineCount - myStableFrameFinalLines);

        myStableFrameFinalLines = scanlineCount;
        myDestabilizationCounter = 0;
      }

    }
    else myStabilizationCounter = 0;
  }
  else myDestabilizationCounter = 0;

  myLastFrameScanlines = scanlineCount;

  if (myJitter > 0) myJitter = std::max(myJitter - myJitterFactor, 0);
  if (myJitter < 0) myJitter = std::min(myJitter + myJitterFactor, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JitterEmulation::updateJitter(Int32 scanlineDifference)
{
  if (uInt32(abs(scanlineDifference)) < Metrics::minDeltaForJitter) return;

  Int32 jitter = std::min<Int32>(scanlineDifference, Metrics::maxJitter);
  jitter = std::max<Int32>(jitter, -myYStart);

  if (jitter > 0) jitter += myJitterFactor;
  if (jitter < 0) jitter -= myJitterFactor;

  if (abs(jitter) > abs(myJitter)) myJitter = jitter;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool JitterEmulation::save(Serializer& out) const
{
  try
  {
    out.putInt(myLastFrameScanlines);
    out.putInt(myStableFrameFinalLines);
    out.putInt(myStableFrames);
    out.putInt(myStabilizationCounter);
    out.putInt(myDestabilizationCounter);
    out.putInt(myJitter);
    out.putInt(myJitterFactor);
    out.putInt(myYStart);
  }
  catch(...)
  {
    cerr << "ERROR: JitterEmulation::save" << std::endl;

    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool JitterEmulation::load(Serializer& in)
{
  try
  {
    myLastFrameScanlines = in.getInt();
    myStableFrameFinalLines = in.getInt();
    myStableFrames = in.getInt();
    myStabilizationCounter = in.getInt();
    myDestabilizationCounter = in.getInt();
    myJitter = in.getInt();
    myJitterFactor = in.getInt();
    myYStart = in.getInt();
  }
  catch (...)
  {
    cerr << "ERROR: JitterEmulation::load" << std::endl;

    return false;
  }

  return true;
}
