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

#include "Missile.hxx"
#include "DrawCounterDecodes.hxx"
#include "TIA.hxx"

enum Count: Int8 {
  renderCounterOffset = -4
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Missile::Missile(uInt32 collisionMask)
  : myCollisionMaskDisabled(collisionMask),
    myCollisionMaskEnabled(0xFFFF),
    myIsSuppressed(false),
    myDecodesOffset(0)
{
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::reset()
{
  myDecodes = DrawCounterDecodes::get().missileDecodes()[myDecodesOffset];
  myIsEnabled = false;
  myEnam = false;
  myResmp = 0;
  myHmmClocks = 0;
  myCounter = 0;
  myIsMoving = false;
  myLastMovementTick = 0;
  myWidth = 1;
  myEffectiveWidth = 1;
  myIsRendering = false;
  myRenderCounter = 0;
  myColor = myObjectColor = myDebugColor = 0;
  myDebugEnabled = false;
  collision = myCollisionMaskDisabled;

  updateEnabled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::enam(uInt8 value)
{
  const auto oldEnam = myEnam;

  myEnam = (value & 0x02) > 0;

  if (oldEnam != myEnam) myTIA->flushLineCache();

  updateEnabled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::hmm(uInt8 value)
{
  myHmmClocks = (value >> 4) ^ 0x08;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::resm(uInt8 counter, bool hblank)
{
  myCounter = counter;

  if (myIsRendering) {
    if (myRenderCounter < 0) {
      myRenderCounter = Count::renderCounterOffset + (counter - 157);

    } else {
      // The following is an effective description of the behavior of missile width after a
      // RESMx during draw. It would be much simpler without the HBLANK cases :)

      switch (myWidth) {
        case 8:
          myRenderCounter = (counter - 157) + ((myRenderCounter >= 4) ? 4 : 0);
          break;

        case 4:
          myRenderCounter = (counter - 157);
          break;

        case 2:
          if (hblank) myIsRendering = myRenderCounter > 1;
          else if (myRenderCounter == 0) myRenderCounter++;

          break;

        default:
          if (hblank) myIsRendering = myRenderCounter > 0;

          break;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::resmp(uInt8 value, const Player& player)
{
  const uInt8 resmp = value & 0x02;

  if (resmp == myResmp) return;

  myTIA->flushLineCache();

  myResmp = resmp;

  if (!myResmp)
    myCounter = player.getRespClock();

  updateEnabled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::toggleCollisions(bool enabled)
{
  myCollisionMaskEnabled = enabled ? 0xFFFF : (0x8000 | myCollisionMaskDisabled);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::toggleEnabled(bool enabled)
{
  myIsSuppressed = !enabled;
  updateEnabled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::nusiz(uInt8 value)
{
  static constexpr uInt8 ourWidths[] = { 1, 2, 4, 8 };

  myDecodesOffset = value & 0x07;
  myWidth = ourWidths[(value & 0x30) >> 4];
  myDecodes = DrawCounterDecodes::get().missileDecodes()[myDecodesOffset];

  if (myIsRendering && myRenderCounter >= myWidth)
    myIsRendering = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::startMovement()
{
  myIsMoving = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Missile::movementTick(uInt8 clock, uInt8 hclock, bool apply)
{
  myLastMovementTick = myCounter;

  if (clock == myHmmClocks) myIsMoving = false;

  if (myIsMoving && apply) tick(hclock);

  return myIsMoving;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::tick(uInt8 hclock)
{
  const bool render =
    myIsRendering &&
    (myRenderCounter >= 0 || (myIsMoving && myRenderCounter == -1 && myWidth < 4 && ((hclock + 1) % 4 == 3))) &&
    myIsEnabled;

  collision = render ? myCollisionMaskEnabled : myCollisionMaskDisabled;

  if (myDecodes[myCounter] && !myResmp) {
    myIsRendering = true;
    myRenderCounter = Count::renderCounterOffset;
  } else if (myIsRendering) {

      if (myIsMoving && myRenderCounter == -1) {

        switch ((hclock + 1) % 4) {
          case 3:
            myEffectiveWidth = myWidth == 1 ? 2 : myWidth;
            if (myWidth < 4) myRenderCounter++;
            break;

          case 2:
            myEffectiveWidth = 0;
            break;

          default:
            myEffectiveWidth = myWidth;
            break;
        }
      }

      if (++myRenderCounter >= (myIsMoving ? myEffectiveWidth : myWidth)) myIsRendering = false;
  }

  if (++myCounter >= 160) myCounter = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::setColor(uInt8 color)
{
  if (color != myObjectColor && myIsEnabled)  myTIA->flushLineCache();

  myObjectColor = color;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::setDebugColor(uInt8 color)
{
  myTIA->flushLineCache();
  myDebugColor = color;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::enableDebugColors(bool enabled)
{
  myTIA->flushLineCache();
  myDebugEnabled = enabled;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::applyColorLoss()
{
  myTIA->flushLineCache();
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::updateEnabled()
{
  myIsEnabled = !myIsSuppressed && myEnam && !myResmp;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::applyColors()
{
  if (!myDebugEnabled)
  {
    if (myTIA->colorLossActive()) myObjectColor |= 0x01;
    else                          myObjectColor &= 0xfe;
    myColor = myObjectColor;
  }
  else
    myColor = myDebugColor;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Missile::getPosition() const
{
  // position =
  //    current playfield x +
  //    (current counter - 156 (the decode clock of copy 0)) +
  //    clock count after decode until first pixel +
  //    1 (it'll take another cycle after the decode for the rendter counter to start ticking)
  //
  // The result may be negative, so we add 160 and do the modulus
  //
  // Mind the sign of renderCounterOffset: it's defined negative above
  return (317 - myCounter - Count::renderCounterOffset + myTIA->getPosition()) % 160;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::setPosition(uInt8 newPosition)
{
  myTIA->flushLineCache();

  // See getPosition for an explanation
  myCounter = (317 - newPosition - Count::renderCounterOffset + myTIA->getPosition()) % 160;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Missile::save(Serializer& out) const
{
  try
  {
    out.putString(name());

    out.putInt(collision);
    out.putInt(myCollisionMaskDisabled);
    out.putInt(myCollisionMaskEnabled);

    out.putBool(myIsEnabled);
    out.putBool(myIsSuppressed);
    out.putBool(myEnam);
    out.putByte(myResmp);

    out.putByte(myHmmClocks);
    out.putByte(myCounter);
    out.putBool(myIsMoving);
    out.putByte(myWidth);
    out.putByte(myEffectiveWidth);
    out.putByte(myLastMovementTick);

    out.putBool(myIsRendering);
    out.putByte(myRenderCounter);

    out.putByte(myDecodesOffset);

    out.putByte(myColor);
    out.putByte(myObjectColor);  out.putByte(myDebugColor);
    out.putBool(myDebugEnabled);
  }
  catch(...)
  {
    cerr << "ERROR: TIA_Missile::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Missile::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    collision = in.getInt();
    myCollisionMaskDisabled = in.getInt();
    myCollisionMaskEnabled = in.getInt();

    myIsEnabled = in.getBool();
    myIsSuppressed = in.getBool();
    myEnam = in.getBool();
    myResmp = in.getByte();

    myHmmClocks = in.getByte();
    myCounter = in.getByte();
    myIsMoving = in.getBool();
    myWidth = in.getByte();
    myEffectiveWidth = in.getByte();
    myLastMovementTick = in.getByte();

    myIsRendering = in.getBool();
    myRenderCounter = in.getByte();

    myDecodesOffset = in.getByte();
    myDecodes = DrawCounterDecodes::get().missileDecodes()[myDecodesOffset];

    myColor = in.getByte();
    myObjectColor = in.getByte();  myDebugColor = in.getByte();
    myDebugEnabled = in.getBool();

    applyColors();
  }
  catch(...)
  {
    cerr << "ERROR: TIA_Missile::load" << endl;
    return false;
  }

  return true;
}
