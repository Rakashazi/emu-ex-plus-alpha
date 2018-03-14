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

#include "Player.hxx"
#include "DrawCounterDecodes.hxx"
#include "TIA.hxx"

enum Count: Int8 {
  renderCounterOffset = -5,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Player::Player(uInt32 collisionMask)
  : myCollisionMaskDisabled(collisionMask),
    myCollisionMaskEnabled(0xFFFF),
    myIsSuppressed(false),
    myDecodesOffset(0)
{
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::reset()
{
  myDecodes = DrawCounterDecodes::get().playerDecodes()[myDecodesOffset];
  myHmmClocks = 0;
  myCounter = 0;
  myIsMoving = false;
  myIsRendering = false;
  myRenderCounter = 0;
  myPatternOld = 0;
  myPatternNew = 0;
  myIsReflected = 0;
  myIsDelaying = false;
  myColor = myObjectColor = myDebugColor = 0;
  myDebugEnabled = false;
  collision = myCollisionMaskDisabled;
  mySampleCounter = 0;
  myDividerPending = 0;
  myDividerChangeCounter = -1;

  setDivider(1);
  updatePattern();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::grp(uInt8 pattern)
{
  const uInt8 oldPatternNew = myPatternNew;

  myPatternNew = pattern;

  if (!myIsDelaying && myPatternNew != oldPatternNew) {
    myTIA->flushLineCache();
    updatePattern();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::hmp(uInt8 value)
{
  myHmmClocks = (value >> 4) ^ 0x08;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::nusiz(uInt8 value, bool hblank)
{
  myDecodesOffset = value & 0x07;

  switch (myDecodesOffset) {
    case 5:
      myDividerPending = 2;
      break;

    case 7:
      myDividerPending = 4;
      break;

    default:
      myDividerPending = 1;
      break;
  }

  const uInt8* oldDecodes = myDecodes;

  myDecodes = DrawCounterDecodes::get().playerDecodes()[myDecodesOffset];

  if (
    myDecodes != oldDecodes &&
    myIsRendering &&
    (myRenderCounter - Count::renderCounterOffset) < 2 &&
    !myDecodes[(myCounter - myRenderCounter + Count::renderCounterOffset + 159) % 160]
  ) {
    myIsRendering = false;
  }

  if (myDividerPending == myDivider) return;

  // The following is an effective description of the effects of NUSIZ during
  // decode and rendering.

  if (myIsRendering) {
    Int8 delta = myRenderCounter - Count::renderCounterOffset;

    switch ((myDivider << 4) | myDividerPending) {
      case 0x12:
      case 0x14:
        if (hblank) {
          if (delta < 4)
            setDivider(myDividerPending);
          else
            myDividerChangeCounter = (delta < 5 ? 1 : 0);
        } else {
          if (delta < 3)
            setDivider(myDividerPending);
          else
            myDividerChangeCounter = 1;
        }

        break;

      case 0x21:
      case 0x41:
        if (delta < (hblank ? 4 : 3)) {
          setDivider(myDividerPending);
        } else if (delta < (hblank ? 6 : 5)) {
          setDivider(myDividerPending);
          myRenderCounter--;
        } else {
          myDividerChangeCounter = (hblank ? 0 : 1);
        }

        break;

      case 0x42:
      case 0x24:
        if (myRenderCounter < 1 || (hblank && (myRenderCounter % myDivider == 1)))
          setDivider(myDividerPending);
        else
          myDividerChangeCounter = (myDivider - (myRenderCounter - 1) % myDivider);
        break;

      default:
        // should never happen
        setDivider(myDividerPending);
        break;
    }

  } else {
    setDivider(myDividerPending);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::resp(uInt8 counter)
{
  myCounter = counter;

  // This tries to account for the effects of RESP during draw counter decode as
  // described in Andrew Towers' notes. Still room for tuning.'
  if (myIsRendering && (myRenderCounter - renderCounterOffset) < 4)
    myRenderCounter = renderCounterOffset + (counter - 157);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::refp(uInt8 value)
{
  const bool oldIsReflected = myIsReflected;

  myIsReflected = (value & 0x08) > 0;

  if (oldIsReflected != myIsReflected) {
    myTIA->flushLineCache();
    updatePattern();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::vdelp(uInt8 value)
{
  const bool oldIsDelaying = myIsDelaying;

  myIsDelaying = (value & 0x01) > 0;

  if (oldIsDelaying != myIsDelaying) {
    myTIA->flushLineCache();
    updatePattern();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::toggleEnabled(bool enabled)
{
  const bool oldIsSuppressed = myIsSuppressed;

  myIsSuppressed = !enabled;

  if (oldIsSuppressed != myIsSuppressed) updatePattern();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::toggleCollisions(bool enabled)
{
  myCollisionMaskEnabled = enabled ? 0xFFFF : (0x8000 | myCollisionMaskDisabled);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::setColor(uInt8 color)
{
  if (color != myObjectColor && myPattern) myTIA->flushLineCache();

  myObjectColor = color;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::setDebugColor(uInt8 color)
{
  myTIA->flushLineCache();
  myDebugColor = color;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::enableDebugColors(bool enabled)
{
  myTIA->flushLineCache();
  myDebugEnabled = enabled;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::applyColorLoss()
{
  myTIA->flushLineCache();
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::startMovement()
{
  myIsMoving = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Player::movementTick(uInt32 clock, bool apply)
{
  if (clock == myHmmClocks) {
    myIsMoving = false;
  }

  if (myIsMoving && apply) tick();

  return myIsMoving;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::tick()
{
  if (!myIsRendering || myRenderCounter < myRenderCounterTripPoint)
    collision = myCollisionMaskDisabled;
  else
    collision = (myPattern & (1 << mySampleCounter)) ? myCollisionMaskEnabled : myCollisionMaskDisabled;

  if (myDecodes[myCounter]) {
    myIsRendering = true;
    mySampleCounter = 0;
    myRenderCounter = Count::renderCounterOffset;
  } else if (myIsRendering) {
    myRenderCounter++;

    switch (myDivider) {
      case 1:
        if (myRenderCounter > 0)
          mySampleCounter++;

        if (myRenderCounter >= 0 && myDividerChangeCounter >= 0 && myDividerChangeCounter-- == 0)
          setDivider(myDividerPending);

        break;

      default:
        if (myRenderCounter > 1 && (((myRenderCounter - 1) % myDivider) == 0))
          mySampleCounter++;

        if (myRenderCounter > 0 && myDividerChangeCounter >= 0 && myDividerChangeCounter-- == 0)
          setDivider(myDividerPending);

        break;
    }

    if (mySampleCounter > 7) myIsRendering = false;
  }

  if (++myCounter >= 160) myCounter = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::shufflePatterns()
{
  const uInt8 oldPatternOld = myPatternOld;

  myPatternOld = myPatternNew;

  if (myIsDelaying && myPatternOld != oldPatternOld) {
    myTIA->flushLineCache();
    updatePattern();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Player::getRespClock() const
{
  switch (myDivider)
  {
    case 1:
      return (myCounter + 160 - 5) % 160;

    case 2:
      return (myCounter + 160 - 9) % 160;

    case 4:
      return (myCounter + 160 - 12) % 160;

    default:
      throw runtime_error("invalid width");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::setGRPOld(uInt8 pattern)
{
  myTIA->flushLineCache();

  myPatternOld = pattern;
  updatePattern();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::updatePattern()
{
  if (myIsSuppressed) {
    myPattern = 0;
    return;
  }

  myPattern = myIsDelaying ? myPatternOld : myPatternNew;

  if (!myIsReflected) {
    myPattern = (
      ((myPattern & 0x01) << 7) |
      ((myPattern & 0x02) << 5) |
      ((myPattern & 0x04) << 3) |
      ((myPattern & 0x08) << 1) |
      ((myPattern & 0x10) >> 1) |
      ((myPattern & 0x20) >> 3) |
      ((myPattern & 0x40) >> 5) |
      ((myPattern & 0x80) >> 7)
    );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::setDivider(uInt8 divider)
{
  myDivider = divider;
  myRenderCounterTripPoint = divider == 1 ? 0 : 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::applyColors()
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
uInt8 Player::getPosition() const
{
  // Wide players are shifted by one pixel to the right
  const uInt8 shift = myDivider == 1 ? 0 : 1;

  // position =
  //    current playfield x +
  //    (current counter - 156 (the decode clock of copy 0)) +
  //    clock count after decode until first pixel +
  //    shift (accounts for wide player shift) +
  //    1 (it'll take another cycle after the decode for the rendter counter to start ticking)
  //
  // The result may be negative, so we add 160 and do the modulus -> 317 = 156 + 160 + 1
  //
  // Mind the sign of renderCounterOffset: it's defined negative above
  return (317 - myCounter - Count::renderCounterOffset + shift + myTIA->getPosition()) % 160;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::setPosition(uInt8 newPosition)
{
  myTIA->flushLineCache();

  const uInt8 shift = myDivider == 1 ? 0 : 1;

  // See getPosition for an explanation
  myCounter = (317 - newPosition - Count::renderCounterOffset + shift + myTIA->getPosition()) % 160;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Player::save(Serializer& out) const
{
  try
  {
    out.putString(name());

    out.putInt(collision);
    out.putInt(myCollisionMaskDisabled);
    out.putInt(myCollisionMaskEnabled);

    out.putByte(myColor);
    out.putByte(myObjectColor);  out.putByte(myDebugColor);
    out.putBool(myDebugEnabled);

    out.putBool(myIsSuppressed);

    out.putByte(myHmmClocks);
    out.putByte(myCounter);
    out.putBool(myIsMoving);

    out.putBool(myIsRendering);
    out.putByte(myRenderCounter);
    out.putByte(myRenderCounterTripPoint);
    out.putByte(myDivider);
    out.putByte(myDividerPending);
    out.putByte(mySampleCounter);
    out.putByte(myDividerChangeCounter);

    out.putByte(myDecodesOffset);

    out.putByte(myPatternOld);
    out.putByte(myPatternNew);
    out.putByte(myPattern);

    out.putBool(myIsReflected);
    out.putBool(myIsDelaying);
  }
  catch(...)
  {
    cerr << "ERROR: TIA_Player::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Player::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    collision = in.getInt();
    myCollisionMaskDisabled = in.getInt();
    myCollisionMaskEnabled = in.getInt();

    myColor = in.getByte();
    myObjectColor = in.getByte();  myDebugColor = in.getByte();
    myDebugEnabled = in.getBool();

    myIsSuppressed = in.getBool();

    myHmmClocks = in.getByte();
    myCounter = in.getByte();
    myIsMoving = in.getBool();

    myIsRendering = in.getBool();
    myRenderCounter = in.getByte();
    myRenderCounterTripPoint = in.getByte();
    myDivider = in.getByte();
    myDividerPending = in.getByte();
    mySampleCounter = in.getByte();
    myDividerChangeCounter = in.getByte();

    myDecodesOffset = in.getByte();
    myDecodes = DrawCounterDecodes::get().playerDecodes()[myDecodesOffset];

    myPatternOld = in.getByte();
    myPatternNew = in.getByte();
    myPattern = in.getByte();

    myIsReflected = in.getBool();
    myIsDelaying = in.getBool();

    applyColors();
  }
  catch(...)
  {
    cerr << "ERROR: TIA_Player::load" << endl;
    return false;
  }

  return true;
}
