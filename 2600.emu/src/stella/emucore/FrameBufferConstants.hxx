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

#ifndef FRAMEBUFFER_CONSTANTS_HXX
#define FRAMEBUFFER_CONSTANTS_HXX

#include "TIAConstants.hxx"
#include "bspf.hxx"

// Minimum size for a framebuffer window
namespace FBMinimum {
  static constexpr uInt32 Width = TIAConstants::viewableWidth * 2;
  static constexpr uInt32 Height = TIAConstants::viewableHeight * 2;
}

// Return values for initialization of framebuffer window
enum class FBInitStatus {
  Success,
  FailComplete,
  FailTooLarge,
  FailNotSupported
};

enum class BufferType {
  None,
  Launcher,
  Emulator,
  Debugger
};

enum class ScalingInterpolation {
  none,
  sharp,
  blur
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

// Colors indices to use for the various GUI elements
// Abstract away what a color actually is, so we can easily change it in
// the future, if necessary
using ColorId = uInt32;
static constexpr ColorId
  // *** Base colors ***
  kColor = 256,
  kBGColor = 257,
  kBGColorLo = 258,
  kBGColorHi = 259,
  kShadowColor = 260,
  // *** Text colors ***
  kTextColor = 261,
  kTextColorHi = 262,
  kTextColorEm = 263,
  kTextColorInv = 264,
  // *** UI elements(dialog and widgets) ***
  kDlgColor = 265,
  kWidColor = 266,
  kWidColorHi = 267,
  kWidFrameColor = 268,
  // *** Button colors ***
  kBtnColor = 269,
  kBtnColorHi = 270,
  kBtnBorderColor = 271,
  kBtnBorderColorHi = 272,
  kBtnTextColor = 273,
  kBtnTextColorHi = 274,
  // *** Checkbox colors ***
  kCheckColor = 275,
  // *** Scrollbar colors ***
  kScrollColor = 276,
  kScrollColorHi = 277,
  // *** Debugger colors ***
  kDbgChangedColor = 278,
  kDbgChangedTextColor = 279,
  kDbgColorHi = 280,
  kDbgColorRed = 281, // Note: this must be < 0x11e (286)! (see PromptWidget::putcharIntern)
  // *** Slider colors ***
  kSliderColor = 282,
  kSliderColorHi = 283,
  kSliderBGColor = 284,
  kSliderBGColorHi = 285,
  kSliderBGColorLo = 286,
  // *** Other colors ***
  kColorInfo = 287,
  kColorTitleBar = 288,
  kColorTitleText = 289,
  kNumColors = 290,
  kNone = 0  // placeholder to represent default/no color
;

// Palette for normal TIA, UI and both combined
using PaletteArray = std::array<uInt32, kColor>;
using UIPaletteArray = std::array<uInt32, kNumColors-kColor>;
using FullPaletteArray = std::array<uInt32, kNumColors>;

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
