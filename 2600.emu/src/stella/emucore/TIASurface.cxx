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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <cmath>

#include "FBSurface.hxx"
#include "Settings.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "TIA.hxx"
#include "PNGLibrary.hxx"
#include "TIASurface.hxx"

namespace {
  FrameBuffer::ScalingInterpolation interpolationModeFromSettings(const Settings& settings)
  {
    return settings.getBool("tia.inter") ?
      FrameBuffer::ScalingInterpolation::blur :
      FrameBuffer::ScalingInterpolation::sharp;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIASurface::TIASurface(OSystem& system)
  : myOSystem(system),
    myFB(system.frameBuffer())
{
  // Load NTSC filter settings
  myNTSCFilter.loadConfig(myOSystem.settings());

  // Create a surface for the TIA image and scanlines; we'll need them eventually
  myTiaSurface = myFB.allocateSurface(
    AtariNTSC::outWidth(TIAConstants::frameBufferWidth),
    TIAConstants::frameBufferHeight,
    interpolationModeFromSettings(myOSystem.settings())
  );

  // Generate scanline data, and a pre-defined scanline surface
  constexpr uInt32 scanHeight = TIAConstants::frameBufferHeight * 2;
  std::array<uInt32, scanHeight> scanData;
  for(uInt32 i = 0; i < scanHeight; i += 2)
  {
    scanData[i]   = 0x00000000;
    scanData[i+1] = 0xff000000;
  }
  mySLineSurface = myFB.allocateSurface(1, scanHeight, interpolationModeFromSettings(myOSystem.settings()), scanData.data());

  // Base TIA surface for use in taking snapshots in 1x mode
  myBaseTiaSurface = myFB.allocateSurface(TIAConstants::frameBufferWidth*2,
                                          TIAConstants::frameBufferHeight);

  myRGBFramebuffer.fill(0);

  // Enable/disable threading in the NTSC TV effects renderer
  myNTSCFilter.enableThreading(myOSystem.settings().getBool("threads"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::initialize(const Console& console,
                            const FrameBuffer::VideoMode& mode)
{
  myTIA = &(console.tia());

  myTiaSurface->setDstPos(mode.image.x(), mode.image.y());
  myTiaSurface->setDstSize(mode.image.w(), mode.image.h());
  mySLineSurface->setDstPos(mode.image.x(), mode.image.y());
  mySLineSurface->setDstSize(mode.image.w(), mode.image.h());

  // Phosphor mode can be enabled either globally or per-ROM
  int p_blend = 0;
  bool enable = false;

  if(myOSystem.settings().getString("tv.phosphor") == "always")
  {
    p_blend = myOSystem.settings().getInt("tv.phosblend");
    enable = true;
  }
  else
  {
    p_blend = stoi(console.properties().get(PropType::Display_PPBlend));
    enable = console.properties().get(PropType::Display_Phosphor) == "YES";
  }
  enablePhosphor(enable, p_blend);

  setNTSC(NTSCFilter::Preset(myOSystem.settings().getInt("tv.filter")), false);

#if 0
cerr << "INITIALIZE:\n"
     << "TIA:\n"
     << "src: " << myTiaSurface->srcRect() << endl
     << "dst: " << myTiaSurface->dstRect() << endl
     << endl;
cerr << "SLine:\n"
     << "src: " << mySLineSurface->srcRect() << endl
     << "dst: " << mySLineSurface->dstRect() << endl
     << endl;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::setPalette(const PaletteArray& tia_palette,
                            const PaletteArray& rgb_palette)
{
  myPalette = tia_palette;

  // The NTSC filtering needs access to the raw RGB data, since it calculates
  // its own internal palette
  myNTSCFilter.setPalette(rgb_palette);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FBSurface& TIASurface::baseSurface(Common::Rect& rect) const
{
  uInt32 tiaw = myTIA->width(), width = tiaw * 2, height = myTIA->height();
  rect.setBounds(0, 0, width, height);

  // Get Blargg buffer and width
  uInt32 *blarggBuf, blarggPitch;
  myTiaSurface->basePtr(blarggBuf, blarggPitch);
  double blarggXFactor = double(blarggPitch) / width;
  bool useBlargg = ntscEnabled();

  // Fill the surface with pixels from the TIA, scaled 2x horizontally
  uInt32 *buf_ptr, pitch;
  myBaseTiaSurface->basePtr(buf_ptr, pitch);

  for(uInt32 y = 0; y < height; ++y)
  {
    for(uInt32 x = 0; x < width; ++x)
    {
      if (useBlargg)
        *buf_ptr++ = blarggBuf[y * blarggPitch + uInt32(nearbyint(x * blarggXFactor))];
      else
        *buf_ptr++ = myPalette[*(myTIA->frameBuffer() + y * tiaw + x / 2)];
    }
  }

  return *myBaseTiaSurface;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TIASurface::mapIndexedPixel(uInt8 indexedColor, uInt8 shift)
{
  return myPalette[indexedColor | shift];
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::setNTSC(NTSCFilter::Preset preset, bool show)
{
  ostringstream buf;
  if(preset == NTSCFilter::Preset::OFF)
  {
    enableNTSC(false);
    buf << "TV filtering disabled";
  }
  else
  {
    enableNTSC(true);
    const string& mode = myNTSCFilter.setPreset(preset);
    buf << "TV filtering (" << mode << " mode)";
  }
  myOSystem.settings().setValue("tv.filter", int(preset));

  if(show) myFB.showMessage(buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::setScanlineIntensity(int amount)
{
  ostringstream buf;
  uInt32 intensity = enableScanlines(amount);
  buf << "Scanline intensity at " << intensity  << "%";
  myOSystem.settings().setValue("tv.scanlines", intensity);

  myFB.showMessage(buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TIASurface::enableScanlines(int relative, int absolute)
{
  FBSurface::Attributes& attr = mySLineSurface->attributes();
  if(relative == 0)  attr.blendalpha = absolute;
  else               attr.blendalpha += relative;
  attr.blendalpha = std::max(0, Int32(attr.blendalpha));
  attr.blendalpha = std::min(100U, attr.blendalpha);

  mySLineSurface->applyAttributes();

  return attr.blendalpha;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::enablePhosphor(bool enable, int blend)
{
  if(myPhosphorHandler.initialize(enable, blend))
  {
    myFilter = Filter(enable ? uInt8(myFilter) | 0x01 : uInt8(myFilter) & 0x10);
    myRGBFramebuffer.fill(0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::enableNTSC(bool enable)
{
  myFilter = Filter(enable ? uInt8(myFilter) | 0x10 : uInt8(myFilter) & 0x01);

  uInt32 surfaceWidth = enable ?
    AtariNTSC::outWidth(TIAConstants::frameBufferWidth) : TIAConstants::frameBufferWidth;

  if (surfaceWidth != myTiaSurface->srcRect().w() || myTIA->height() != myTiaSurface->srcRect().h()) {
    myTiaSurface->setSrcSize(surfaceWidth, myTIA->height());

    myTiaSurface->invalidate();
  }

  mySLineSurface->setSrcSize(1, 2 * myTIA->height());

  myScanlinesEnabled = myOSystem.settings().getInt("tv.scanlines") > 0;
  FBSurface::Attributes& sl_attr = mySLineSurface->attributes();
  sl_attr.blending   = myScanlinesEnabled;
  sl_attr.blendalpha = myOSystem.settings().getInt("tv.scanlines");
  mySLineSurface->applyAttributes();

  myRGBFramebuffer.fill(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIASurface::effectsInfo() const
{
  const FBSurface::Attributes& attr = mySLineSurface->attributes();

  ostringstream buf;
  switch(myFilter)
  {
    case Filter::Normal:
      buf << "Disabled, normal mode";
      break;
    case Filter::Phosphor:
      buf << "Disabled, phosphor mode";
      break;
    case Filter::BlarggNormal:
      buf << myNTSCFilter.getPreset() << ", scanlines=" << attr.blendalpha;
      break;
    case Filter::BlarggPhosphor:
      buf << myNTSCFilter.getPreset() << ", phosphor, scanlines="
          << attr.blendalpha;
      break;
  }

  buf << ", inter=" << (myOSystem.settings().getBool("tia.inter") ? "enabled" : "disabled");

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt32 TIASurface::averageBuffers(uInt32 bufOfs)
{
  const uInt32 c = myRGBFramebuffer[bufOfs];
  const uInt32 p = myPrevRGBFramebuffer[bufOfs];

  // Split into RGB values
  const uInt8 rc = static_cast<uInt8>(c >> 16),
              gc = static_cast<uInt8>(c >> 8),
              bc = static_cast<uInt8>(c),
              rp = static_cast<uInt8>(p >> 16),
              gp = static_cast<uInt8>(p >> 8),
              bp = static_cast<uInt8>(p);

  // Mix current calculated buffer with previous calculated buffer (50:50)
  const uInt8 rn = (rc + rp) / 2;
  const uInt8 gn = (gc + gp) / 2;
  const uInt8 bn = (bc + bp) / 2;

  // return averaged value
  return (rn << 16) | (gn << 8) | bn;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::render()
{
  uInt32 width = myTIA->width(), height = myTIA->height();

  uInt32 *out, outPitch;
  myTiaSurface->basePtr(out, outPitch);

  switch(myFilter)
  {
    case Filter::Normal:
    {
      uInt8* tiaIn = myTIA->frameBuffer();

      uInt32 bufofs = 0, screenofsY = 0, pos;
      for(uInt32 y = 0; y < height; ++y)
      {
        pos = screenofsY;
        for (uInt32 x = width / 2; x; --x)
        {
          out[pos++] = myPalette[tiaIn[bufofs++]];
          out[pos++] = myPalette[tiaIn[bufofs++]];
        }
        screenofsY += outPitch;
      }
      break;
    }

    case Filter::Phosphor:
    {
      uInt8*  tiaIn = myTIA->frameBuffer();
      uInt32* rgbIn = myRGBFramebuffer.data();

      if (mySaveSnapFlag)
        std::copy_n(myRGBFramebuffer.begin(), width * height,
                    myPrevRGBFramebuffer.begin());

      uInt32 bufofs = 0, screenofsY = 0, pos;
      for(uInt32 y = height; y ; --y)
      {
        pos = screenofsY;
        for(uInt32 x = width / 2; x ; --x)
        {
          // Store back into displayed frame buffer (for next frame)
          rgbIn[bufofs] = out[pos++] = PhosphorHandler::getPixel(myPalette[tiaIn[bufofs]], rgbIn[bufofs]);
          ++bufofs;
          rgbIn[bufofs] = out[pos++] = PhosphorHandler::getPixel(myPalette[tiaIn[bufofs]], rgbIn[bufofs]);
          ++bufofs;
        }
        screenofsY += outPitch;
      }
      break;
    }

    case Filter::BlarggNormal:
    {
      myNTSCFilter.render(myTIA->frameBuffer(), width, height, out, outPitch << 2);
      break;
    }

    case Filter::BlarggPhosphor:
    {
      if(mySaveSnapFlag)
        std::copy_n(myRGBFramebuffer.begin(), height * outPitch,
                    myPrevRGBFramebuffer.begin());

      myNTSCFilter.render(myTIA->frameBuffer(), width, height, out, outPitch << 2, myRGBFramebuffer.data());
      break;
    }
  }

  // Draw TIA image
  myTiaSurface->render();

  // Draw overlaying scanlines
  if(myScanlinesEnabled)
    mySLineSurface->render();

  if(mySaveSnapFlag)
  {
    mySaveSnapFlag = false;
  #ifdef PNG_SUPPORT
    myOSystem.png().takeSnapshot();
  #endif
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::renderForSnapshot()
{
  // TODO: This is currently called from PNGLibrary::takeSnapshot() only
  // Therefore the code could be simplified.
  // At some point, we will probably merge some of the functionality.
  // Furthermore, toggling the variable 'mySaveSnapFlag' in different places
  // is brittle, especially since rendering can happen in a different thread.

  uInt32 width = myTIA->width();
  uInt32 height = myTIA->height();
  uInt32 pos = 0;
  uInt32 *outPtr, outPitch;

  myTiaSurface->basePtr(outPtr, outPitch);

  mySaveSnapFlag = false;
  switch (myFilter)
  {
    // For non-phosphor modes, render the frame again
    case Filter::Normal:
    case Filter::BlarggNormal:
      render();
      break;

    // For phosphor modes, copy the phosphor framebuffer
    case Filter::Phosphor:
    {
      uInt32 bufofs = 0, screenofsY = 0;
      for(uInt32 y = height; y; --y)
      {
        pos = screenofsY;
        for(uInt32 x = width / 2; x; --x)
        {
          outPtr[pos++] = averageBuffers(bufofs++);
          outPtr[pos++] = averageBuffers(bufofs++);
        }
        screenofsY += outPitch;
      }
      break;
    }

    case Filter::BlarggPhosphor:
      uInt32 bufofs = 0;
      for(uInt32 y = height; y; --y)
        for(uInt32 x = outPitch; x; --x)
          outPtr[pos++] = averageBuffers(bufofs++);
      break;
  }

  if(myPhosphorHandler.phosphorEnabled())
  {
    // Draw TIA image
    myTiaSurface->render();

    // Draw overlaying scanlines
    if(myScanlinesEnabled)
      mySLineSurface->render();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::updateSurfaceSettings()
{
  myTiaSurface->setScalingInterpolation(interpolationModeFromSettings(myOSystem.settings()));
  mySLineSurface->setScalingInterpolation(
      interpolationModeFromSettings(myOSystem.settings())
  );
}
