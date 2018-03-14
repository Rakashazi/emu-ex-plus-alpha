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

#ifndef TIA_YSTART_DETECTOR
#define TIA_YSTART_DETECTOR

#include "AbstractFrameManager.hxx"

/**
 * This frame manager detects ystart from the first line with vblank = off.
 */

class YStartDetector: public AbstractFrameManager {

  public:

    YStartDetector() = default;

  public:

    /**
     * Getter for the detected ystart value
     */
    uInt32 detectedYStart() const;

    /**
     * We require frame layout to be set from outside.
     */
    void setLayout(FrameLayout layout) override { this->layout(layout); }

  protected:

    /**
     * We need to track vsync changes.
     */
    void onSetVsync() override;

    /**
     * Reset hook.
     */
    void onReset() override;

    /**
     * The workhorse.
     */
    void onNextLine() override;

  private:

    /**
     * Our various states.
     */
    enum State {
      // Wait for vsync on
      waitForVsyncStart,

      // Wait for vsync off
      waitForVsyncEnd,

      // Wait for the visible frame to start
      waitForFrameStart
    };

    /**
     * Have we settled on a frame start?
     */
    enum VblankMode {
      // We have settled on a frame start and have some hysteresis before we return to floating
      locked,

      // We are actively looking for the frame to start
      floating
    };

  private:

    /**
     * Side effects for state transitions.
     */
    void setState(State state);

    /**
     * Perform detection and decide whether the frame starts now.
     */
    bool shouldTransitionToFrame();

  private:

    /**
     * State.
     */
    State myState;

    /**
     * locked / floating
     */
    VblankMode myVblankMode;

    /**
     * Counts the scanlines that we wait for vsync to start.
     */
    uInt32 myLinesWaitingForVsyncToStart;

    /**
     * The number of lines we are currently waiting for the frame to start (and vblank to end).
     */
    uInt32 myCurrentVblankLines;

    /**
     * The number of vblank lines on the last frame.
     */
    uInt32 myLastVblankLines;

    /**
     * Count "vblank violations" in fixed mode (the number of consecutive frames where ystart
     * differs from the previously detected value). Once a trip point is reached, we transition
     * back to floating mode.
     */
    uInt32 myVblankViolations;

    /**
     * The number of frames in floating mode with stable ystart. Once a trip point is reacted,
     * we transition to fixed mode
     */
    uInt32 myStableVblankFrames;

    /**
     * Tracks deviations from the determined ystart value during a fixed mode frame in order to
     * avoid double counting.
     */
    bool myVblankViolated;

  private:

    YStartDetector(const YStartDetector&) = delete;
    YStartDetector(YStartDetector&&) = delete;
    YStartDetector& operator=(const YStartDetector&) = delete;
    YStartDetector& operator=(YStartDetector&&) = delete;
};

#endif // TIA_YSTART_DETECTOR
