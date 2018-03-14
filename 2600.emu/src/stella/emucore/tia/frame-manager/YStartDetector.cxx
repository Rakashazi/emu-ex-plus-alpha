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

#include "YStartDetector.hxx"
#include "TIAConstants.hxx"

/**
 * Misc. numeric constants used in the algorithm.
 */
enum Metrics: uInt32 {
  // ideal world frame sizes
  frameLinesNTSC            = 262,
  frameLinesPAL             = 312,

  // the ideal vblank zone
  vblankNTSC                = 37,
  vblankPAL                 = 45,

  // number of scanlines to wait for vsync to start (exceeding after the ideal frame size) and stop
  waitForVsync              = 50,

  // max lines underscan
  maxUnderscan              = 10,

  // max lines deviations from detected ystart before we switch back to floating
  maxVblankViolations       = 2,

  // switch to fixed mode after this number of stable frames (+1)
  minStableVblankFrames     = 1,

  // no transitions to fixed mode will happend during those
  initialGarbageFrames      = TIAConstants::initialGarbageFrames
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 YStartDetector::detectedYStart() const
{
  return myLastVblankLines;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void YStartDetector::onReset()
{
  myState = State::waitForVsyncStart;
  myVblankMode = VblankMode::floating;
  myLinesWaitingForVsyncToStart = 0;
  myCurrentVblankLines = 0;
  myLastVblankLines = 0;
  myVblankViolations = 0;
  myStableVblankFrames = 0;
  myVblankViolated = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void YStartDetector::onSetVsync()
{
  if (myVsync)
    setState(State::waitForVsyncEnd);
  else
    setState(State::waitForFrameStart);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void YStartDetector::onNextLine()
{
  const uInt32 frameLines = layout() == FrameLayout::ntsc ? Metrics::frameLinesNTSC : Metrics::frameLinesPAL;

  switch (myState) {
    case State::waitForVsyncStart:
      // We start counting the number of "lines spent while waiting for vsync start" from
      // the "ideal" frame size (corrected by the three scanlines spent in vsync).
      if (myCurrentFrameTotalLines > frameLines - 3 || myTotalFrames == 0)
        myLinesWaitingForVsyncToStart++;

      if (myLinesWaitingForVsyncToStart > Metrics::waitForVsync) setState(State::waitForVsyncEnd);

      break;

    case State::waitForVsyncEnd:
      if (++myLinesWaitingForVsyncToStart > Metrics::waitForVsync) setState(State::waitForFrameStart);

      break;

    case State::waitForFrameStart:
      if (shouldTransitionToFrame()) setState(State::waitForVsyncStart);
      else myCurrentVblankLines++;

      break;

    default:
      throw runtime_error("cannot happen");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool YStartDetector::shouldTransitionToFrame()
{
  uInt32 vblankLines = layout() == FrameLayout::pal ? Metrics::vblankPAL : Metrics::vblankNTSC;

  // Are we free to transition as per vblank cycle?
  bool shouldTransition = myCurrentVblankLines + 1 >= (myVblank ? vblankLines : vblankLines - Metrics::maxUnderscan);

  // Do we **actually** transition? This depends on what mode we are in.
  bool transition = false;

  switch (myVblankMode) {
    // Floating mode: we still are looking for a stable frame start
    case VblankMode::floating:

      // Are we free to transition?
      if (shouldTransition) {
        // Is this same scanline in which the transition ocurred last frame?
        if (myTotalFrames > Metrics::initialGarbageFrames && myCurrentVblankLines == myLastVblankLines)
          // Yes? -> Increase the number of stable frames
          myStableVblankFrames++;
        else
          // No? -> Frame start shifted again, set the number of consecutive stable frames to zero
          myStableVblankFrames = 0;

        // Save the transition point for checking on it next frame
        myLastVblankLines = myCurrentVblankLines;

        // In floating mode, we transition whenever we can.
        transition = true;
      }

      // Transition to locked mode if we saw enough stable frames in a row.
      if (myStableVblankFrames >= Metrics::minStableVblankFrames) {
        myVblankMode = VblankMode::locked;
        myVblankViolations = 0;
      }

      break;

    // Locked mode: always transition at the same point, but check whether this is actually the
    // detected transition point and revert state if applicable
    case VblankMode::locked:

      // Have we reached the transition point?
      if (myCurrentVblankLines == myLastVblankLines) {

        // Are we free to transition per the algorithm and didn't observe an violation before?
        // (aka did the algorithm tell us to transition before reaching the actual line)
        if (shouldTransition && !myVblankViolated)
          // Reset the number of irregular frames (if any)
          myVblankViolations = 0;
        else {
          // Record a violation if it wasn't recorded before
          if (!myVblankViolated) myVblankViolations++;
          myVblankViolated = true;
        }

        // transition
        transition = true;
      // The algorithm tells us to transition although we haven't reached the trip line before
      } else if (shouldTransition) {
        // Record a violation if it wasn't recorded before
        if (!myVblankViolated) myVblankViolations++;
        myVblankViolated = true;
      }

      // Revert to floating mode if there were too many irregular frames in a row
      if (myVblankViolations > Metrics::maxVblankViolations) {
        myVblankMode = VblankMode::floating;
        myStableVblankFrames = 0;
      }

      break;

    default:
      throw runtime_error("cannot happen");
  }

    return transition;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void YStartDetector::setState(State state)
{
  if (state == myState) return;

  myState = state;
  myLinesWaitingForVsyncToStart = 0;

  switch (state) {
    case State::waitForVsyncEnd:
      break;

    case State::waitForVsyncStart:
      notifyFrameComplete();
      notifyFrameStart();
      break;

    case State::waitForFrameStart:
      myVblankViolated = false;
      myCurrentVblankLines = 0;
      break;

    default:
      throw new runtime_error("cannot happen");
  }
}
