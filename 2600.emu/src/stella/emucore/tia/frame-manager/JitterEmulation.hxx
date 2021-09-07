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

#ifndef TIA_JITTER_EMULATION
#define TIA_JITTER_EMULATION

#include "bspf.hxx"
#include "Serializable.hxx"

class JitterEmulation: public Serializable {
  public:

    JitterEmulation();

  public:

    void reset();

    void frameComplete(uInt32 scanlineCount);

    void setJitterFactor(Int32 factor) { myJitterFactor = factor; }

    Int32 jitter() const { return myJitter; }

    void setYStart(uInt32 ystart) { myYStart = ystart; }

    /**
     * Save state.
     */
    bool save(Serializer& out) const override;

    /**
     * Restore state.
     */
    bool load(Serializer& in) override;

  private:

    void updateJitter(Int32 scanlineDifference);

  private:

    uInt32 myLastFrameScanlines{0};

    Int32 myStableFrameFinalLines{-1};

    uInt32 myStableFrames{0};

    uInt32 myStabilizationCounter{0};

    uInt32 myDestabilizationCounter{0};

    Int32 myJitter{0};

    Int32 myJitterFactor{0};

    uInt32 myYStart{0};

  private:

    JitterEmulation(const JitterEmulation&) = delete;
    JitterEmulation(JitterEmulation&&) = delete;
    JitterEmulation& operator=(const JitterEmulation&) = delete;
    JitterEmulation& operator=(JitterEmulation&&) = delete;
};

#endif // TIA_JITTER_EMULATION
