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

#ifndef TIA_BALL
#define TIA_BALL

#include "Serializable.hxx"
#include "bspf.hxx"

class TIA;

class Ball : public Serializable
{
  public:

    Ball(uInt32 collisionMask);

  public:

    void setTIA(TIA* tia) { myTIA = tia; }

    void reset();

    void enabl(uInt8 value);

    void hmbl(uInt8 value);

    void resbl(uInt8 counter);

    void ctrlpf(uInt8 value);

    void vdelbl(uInt8 value);

    void toggleCollisions(bool enabled);

    void toggleEnabled(bool enabled);

    void setColor(uInt8 color);

    void setDebugColor(uInt8 color);
    void enableDebugColors(bool enabled);

    void applyColorLoss();

    void startMovement();

    bool movementTick(uInt32 clock, bool apply);

    void tick(bool isReceivingMclock = true);

    bool isOn() const { return (collision & 0x8000); }
    uInt8 getColor() const { return myColor; }

    void shuffleStatus();

    uInt8 getPosition() const;
    void setPosition(uInt8 newPosition);

    bool getENABLOld() const { return myIsEnabledOld; }
    bool getENABLNew() const { return myIsEnabledNew; }

    void setENABLOld(bool enabled);

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;
    string name() const override { return "TIA_Ball"; }

  public:

    uInt32 collision;

  private:

    void updateEnabled();
    void applyColors();

  private:

    uInt32 myCollisionMaskDisabled;
    uInt32 myCollisionMaskEnabled;

    uInt8 myColor;
    uInt8 myObjectColor, myDebugColor;
    bool myDebugEnabled;

    bool myIsEnabledOld;
    bool myIsEnabledNew;
    bool myIsEnabled;
    bool myIsSuppressed;
    bool myIsDelaying;

    uInt8 myHmmClocks;
    uInt8 myCounter;
    bool myIsMoving;
    uInt8 myWidth;
    uInt8 myEffectiveWidth;
    uInt8 myLastMovementTick;

    bool myIsRendering;
    Int8 myRenderCounter;

    TIA* myTIA;

  private:
    Ball() = delete;
    Ball(const Ball&) = delete;
    Ball(Ball&&) = delete;
    Ball& operator=(const Ball&) = delete;
    Ball& operator=(Ball&&);
};

#endif // TIA_BALL
