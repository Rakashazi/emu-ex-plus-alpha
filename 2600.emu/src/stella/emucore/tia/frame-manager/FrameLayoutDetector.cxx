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

#include "FrameLayoutDetector.hxx"
#include "TIAConstants.hxx"

/**
 * Misc. numeric constants used in the algorithm.
 */
enum Metrics: uInt32 {
  // ideal frame heights
  frameLinesNTSC            = 262,
  frameLinesPAL             = 312,

  // number of scanlines to wait for vsync to start and stop (exceeding ideal frame height)
  waitForVsync              = 100,

  // tolerance window around ideal frame size for TV mode detection
  tvModeDetectionTolerance  = 20,

  // these frames will not be considered for detection
  initialGarbageFrames      = TIAConstants::initialGarbageFrames
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameLayout FrameLayoutDetector::detectedLayout() const{
  // We choose the mode that was detected for the majority of frames.
  return myPalFrames > myNtscFrames ? FrameLayout::pal : FrameLayout::ntsc;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameLayoutDetector::FrameLayoutDetector()
{
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameLayoutDetector::onReset()
{
  myState = State::waitForVsyncStart;
  myNtscFrames = myPalFrames = 0;
  myLinesWaitingForVsyncToStart = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameLayoutDetector::onSetVsync()
{
  if (myVsync)
    setState(State::waitForVsyncEnd);
  else
    setState(State::waitForVsyncStart);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameLayoutDetector::onNextLine()
{
  const uInt32 frameLines = layout() == FrameLayout::ntsc ? Metrics::frameLinesNTSC : Metrics::frameLinesPAL;

  switch (myState) {
    case State::waitForVsyncStart:
      // We start counting the number of "lines spent while waiting for vsync start" from
      // the "ideal" frame size (corrected by the three scanlines spent in vsync).
      if (myCurrentFrameTotalLines > frameLines - 3 || myTotalFrames == 0)
        ++myLinesWaitingForVsyncToStart;

      if (myLinesWaitingForVsyncToStart > Metrics::waitForVsync) setState(State::waitForVsyncEnd);

      break;

    case State::waitForVsyncEnd:
      if (++myLinesWaitingForVsyncToStart > Metrics::waitForVsync) setState(State::waitForVsyncStart);

      break;

    default:
      throw runtime_error("cannot happen");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameLayoutDetector::setState(State state)
{
  if (state == myState) return;

  myState = state;
  myLinesWaitingForVsyncToStart = 0;

  switch (myState) {
    case State::waitForVsyncEnd:
      break;

    case State::waitForVsyncStart:
      finalizeFrame();
      notifyFrameStart();
      break;

    default:
      throw runtime_error("cannot happen");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameLayoutDetector::finalizeFrame()
{
  notifyFrameComplete();

  if (myTotalFrames <= Metrics::initialGarbageFrames) return;

  // Calculate the delta between scanline count and the sweet spot for the respective
  // frame layouts
  const uInt32
    deltaNTSC = abs(Int32(myCurrentFrameFinalLines) - Int32(frameLinesNTSC)),
    deltaPAL =  abs(Int32(myCurrentFrameFinalLines) - Int32(frameLinesPAL));

  // Does the scanline count fall into one of our tolerance windows? -> use it
  if (std::min(deltaNTSC, deltaPAL) <= Metrics::tvModeDetectionTolerance)
    layout(deltaNTSC <= deltaPAL ? FrameLayout::ntsc : FrameLayout::pal);
  else if (
  // If scanline count is odd and lies between the PAL and NTSC windows we assume
  // it is NTSC (it would cause color loss on PAL CRTs)
    (myCurrentFrameFinalLines < frameLinesPAL) &&
    (myCurrentFrameFinalLines > frameLinesNTSC) &&
    (myCurrentFrameFinalLines % 2)
  )
    layout(FrameLayout::ntsc);
  else
  // Take the nearest layout if all else fails
    layout(deltaNTSC <= deltaPAL ? FrameLayout::ntsc : FrameLayout::pal);

  switch (layout()) {
    case FrameLayout::ntsc:
      ++myNtscFrames;
      break;

    case FrameLayout::pal:
      ++myPalFrames;
      break;

    default:
      throw runtime_error("cannot happen");
  }
}
