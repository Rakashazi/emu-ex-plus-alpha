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

// #define TIA_FRAMEMANAGER_DEBUG_LOG

#include <algorithm>

#include "FrameManager.hxx"

enum Metrics: uInt32 {
  vblankNTSC                    = 37,
  vblankPAL                     = 45,
  kernelNTSC                    = 192,
  kernelPAL                     = 228,
  overscanNTSC                  = 30,
  overscanPAL                   = 36,
  vsync                         = 3,
  maxLinesVsync                 = 50,
  visibleOverscan               = 20,
  initialGarbageFrames          = TIAConstants::initialGarbageFrames
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameManager::FrameManager() :
  myHeight(0),
  myYStart(0)
{
  onLayoutChange();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::onReset()
{
  myState = State::waitForVsyncStart;
  myLineInState = 0;
  myTotalFrames = 0;
  myVsyncLines = 0;
  myY = 0;

  myStableFrameLines = -1;
  myStableFrameHeightCountdown = 0;

  myJitterEmulation.reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::onNextLine()
{
  Int32 jitter;

  State previousState = myState;
  myLineInState++;

  switch (myState)
  {
    case State::waitForVsyncStart:
      if ((myCurrentFrameTotalLines > myFrameLines - 3) || myTotalFrames == 0)
        myVsyncLines++;

      if (myVsyncLines > Metrics::maxLinesVsync) setState(State::waitForFrameStart);

      break;

    case State::waitForVsyncEnd:
      if (++myVsyncLines > Metrics::maxLinesVsync)
        setState(State::waitForFrameStart);

      break;

    case State::waitForFrameStart:
      jitter =
        (myJitterEnabled && myTotalFrames > Metrics::initialGarbageFrames) ? myJitterEmulation.jitter() : 0;

      if (myLineInState >= (myYStart + jitter)) setState(State::frame);
      break;

    case State::frame:
      if (myLineInState >= myHeight)
      {
        myLastY = ystart() + myY;  // Last line drawn in this frame
        setState(State::waitForVsyncStart);
      }
      break;

    default:
      throw runtime_error("frame manager: invalid state");
  }

  if (myState == State::frame && previousState == State::frame) myY++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 FrameManager::missingScanlines() const
{
  if (myLastY == myYStart + myY)
    return 0;
  else {
    return myHeight - myY;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setYstart(uInt32 ystart)
{
  myYStart = ystart;
  myJitterEmulation.setYStart(ystart);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::onSetVsync()
{
  if (myState == State::waitForVsyncEnd) setState(State::waitForFrameStart);
  else setState(State::waitForVsyncEnd);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setState(FrameManager::State state)
{
  if (myState == state) return;

  myState = state;
  myLineInState = 0;

  switch (myState) {
    case State::waitForFrameStart:
      notifyFrameComplete();

      if (myTotalFrames > Metrics::initialGarbageFrames)
        myJitterEmulation.frameComplete(myCurrentFrameFinalLines);

      notifyFrameStart();

      myVsyncLines = 0;
      break;

    case State::frame:
      myVsyncLines = 0;
      myY = 0;
      break;

    default:
      break;
  }

  updateIsRendering();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::onLayoutChange()
{
  switch (layout())
  {
    case FrameLayout::ntsc:
      myVblankLines   = Metrics::vblankNTSC;
      myKernelLines   = Metrics::kernelNTSC;
      myOverscanLines = Metrics::overscanNTSC;
      break;

    case FrameLayout::pal:
      myVblankLines   = Metrics::vblankPAL;
      myKernelLines   = Metrics::kernelPAL;
      myOverscanLines = Metrics::overscanPAL;
      break;

    default:
      throw runtime_error("frame manager: invalid TV mode");
  }

  myFrameLines = Metrics::vsync + myVblankLines + myKernelLines + myOverscanLines;
  if (myFixedHeight == 0)
    myHeight = myKernelLines + Metrics::visibleOverscan;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setFixedHeight(uInt32 height)
{
  myFixedHeight = height;
  myHeight = myFixedHeight > 0 ? myFixedHeight : (myKernelLines + Metrics::visibleOverscan);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::updateIsRendering() {
  myIsRendering = myState == State::frame;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameManager::onSave(Serializer& out) const
{
  if (!myJitterEmulation.save(out)) return false;

  out.putInt(uInt32(myState));
  out.putInt(myLineInState);
  out.putInt(myVsyncLines);
  out.putInt(myY);
  out.putInt(myLastY);

  out.putInt(myVblankLines);
  out.putInt(myKernelLines);
  out.putInt(myOverscanLines);
  out.putInt(myFrameLines);
  out.putInt(myHeight);
  out.putInt(myFixedHeight);
  out.putInt(myYStart);

  out.putBool(myJitterEnabled);

  out.putInt(myStableFrameLines);
  out.putInt(myStableFrameHeightCountdown);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameManager::onLoad(Serializer& in)
{
  if (!myJitterEmulation.load(in)) return false;

  myState = State(in.getInt());
  myLineInState = in.getInt();
  myVsyncLines = in.getInt();
  myY = in.getInt();
  myLastY = in.getInt();

  myVblankLines = in.getInt();
  myKernelLines = in.getInt();
  myOverscanLines = in.getInt();
  myFrameLines = in.getInt();
  myHeight = in.getInt();
  myFixedHeight = in.getInt();
  myYStart = in.getInt();

  myJitterEnabled = in.getBool();

  myStableFrameLines = in.getInt();
  myStableFrameHeightCountdown = in.getInt();

  return true;
}
