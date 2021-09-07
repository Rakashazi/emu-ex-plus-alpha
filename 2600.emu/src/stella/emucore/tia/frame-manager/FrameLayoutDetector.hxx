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

#ifndef TIA_FRAME_LAYOUT_DETECTOR
#define TIA_FRAME_LAYOUT_DETECTOR

#include "FrameLayout.hxx"
#include "AbstractFrameManager.hxx"

/**
 * This frame manager performs frame layout autodetection. It counts the scanlines
 * in each frame and assigns guesses the frame layout from this.
 */
class FrameLayoutDetector: public AbstractFrameManager {
  public:

    FrameLayoutDetector();

  public:

    /**
     * Return the detected frame layout.
     */
    FrameLayout detectedLayout() const;

  protected:

    /**
     * Hook into vsync changes.
     */
    void onSetVsync() override;

    /**
     * Hook into reset.
     */
    void onReset() override;

    /**
     * Hook into line changes.
     */
    void onNextLine() override;

  private:

    /**
     * This frame manager only tracks frame boundaries, so we have only two states.
     */
    enum class State {
      // Wait for VSYNC to be enabled.
      waitForVsyncStart,

      // Wait for VSYNC to be disabled.
      waitForVsyncEnd
    };


  private:

    /**
     * Change state and change internal state accordingly.
     */
    void setState(State state);

    /**
     * Finalize the current frame and guess frame layout from the scanline count.
     */
    void finalizeFrame();

  private:

    /**
     * The current state.
     */
    State myState{State::waitForVsyncStart};

    /**
     * The total number of frames detected as the respective frame layout.
     */
    uInt32 myNtscFrames{0}, myPalFrames{0};

    /**
     * We count the number of scanlines we spend waiting for vsync to be
     * toggled. If a threshold is exceeded, we force the transition.
     */
    uInt32 myLinesWaitingForVsyncToStart{0};

  private:

    FrameLayoutDetector(const FrameLayoutDetector&) = delete;
    FrameLayoutDetector(FrameLayoutDetector&&) = delete;
    FrameLayoutDetector& operator=(const FrameLayoutDetector&) = delete;
    FrameLayoutDetector& operator=(FrameLayoutDetector&&) = delete;

};

#endif // TIA_FRAME_LAYOUT_DETECTOR
