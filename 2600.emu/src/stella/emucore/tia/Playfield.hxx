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

#ifndef TIA_PLAYFIELD
#define TIA_PLAYFIELD

#include "Serializable.hxx"
#include "bspf.hxx"

class TIA;

class Playfield : public Serializable
{
  public:
    Playfield(uInt32 collisionMask);

  public:

    void setTIA(TIA* tia) { myTIA = tia; }

    void reset();

    void pf0(uInt8 value);

    void pf1(uInt8 value);

    void pf2(uInt8 value);

    void ctrlpf(uInt8 value);

    void toggleEnabled(bool enabled);

    void toggleCollisions(bool enabled);

    void setColor(uInt8 color);

    void setColorP0(uInt8 color);

    void setColorP1(uInt8 color);

    void setDebugColor(uInt8 color);
    void enableDebugColors(bool enabled);

    void applyColorLoss();

    void tick(uInt32 x);

    bool isOn() const { return (collision & 0x8000); }
    uInt8 getColor() const;

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;
    string name() const override { return "TIA_Playfield"; }

  public:

    uInt32 collision;

  private:

    enum ColorMode: uInt8 {normal, score};

  private:

    void applyColors();
    void updatePattern();

  private:

    uInt32 myCollisionMaskDisabled;
    uInt32 myCollisionMaskEnabled;

    bool myIsSuppressed;

    uInt8 myColorLeft;
    uInt8 myColorRight;
    uInt8 myColorP0;
    uInt8 myColorP1;
    uInt8 myObjectColor, myDebugColor;
    bool myDebugEnabled;

    ColorMode myColorMode;

    uInt32 myPattern;
    uInt32 myEffectivePattern;
    bool myRefp;
    bool myReflected;

    uInt8 myPf0;
    uInt8 myPf1;
    uInt8 myPf2;

    uInt32 myX;

    TIA* myTIA;

  private:
    Playfield() = delete;
    Playfield(const Playfield&) = delete;
    Playfield(Playfield&&) = delete;
    Playfield& operator=(const Playfield&) = delete;
    Playfield& operator=(Playfield&&) = delete;
};

#endif // TIA_PLAYFIELD
