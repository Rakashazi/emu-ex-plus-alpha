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

#ifndef FRAMEBUFFER_CONSTANTS_HXX
#define FRAMEBUFFER_CONSTANTS_HXX

// Return values for initialization of framebuffer window
enum class FBInitStatus {
  Success,
  FailComplete,
  FailTooLarge,
  FailNotSupported
};

// Positions for onscreen/overlaid messages
enum class MessagePosition {
  TopLeft,
  TopCenter,
  TopRight,
  MiddleLeft,
  MiddleCenter,
  MiddleRight,
  BottomLeft,
  BottomCenter,
  BottomRight
};

// TODO - make this 'enum class'
// Colors indices to use for the various GUI elements
enum {
  kColor = 256,
  kBGColor,
  kBGColorLo,
  kBGColorHi,
  kShadowColor,
  kTextColor,
  kTextColorHi,
  kTextColorEm,
  kTextColorInv,
  kDlgColor,
  kWidColor,
  kWidFrameColor,
  kBtnColor,
  kBtnColorHi,
  kBtnTextColor,
  kBtnTextColorHi,
  kCheckColor,
  kScrollColor,
  kScrollColorHi,
  kSliderColor,
  kSliderColorHi,
  kDbgChangedColor,
  kDbgChangedTextColor,
  kDbgColorHi,
  kDbgColorRed,
  kColorInfo,
  kNumColors
};

// Text alignment modes for drawString()
enum class TextAlign {
  Left,
  Center,
  Right
};

// Line types for drawing rectangular frames
enum class FrameStyle {
  Solid,
  Dashed
};

#endif // FRAMEBUFFER_CONSTANTS_HXX
