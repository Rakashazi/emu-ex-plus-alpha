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

#include <cmath>

#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "Settings.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "TIA.hxx"
#include "TIASurface.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIASurface::TIASurface(OSystem& system)
  : myOSystem(system),
    myFB(system.frameBuffer()),
    myTIA(nullptr),
    myFilter(Filter::Normal),
    myUsePhosphor(false),
    myPhosphorPercent(0.60f),
    myScanlinesEnabled(false),
    myPalette(nullptr)
{
  // Load NTSC filter settings
  myNTSCFilter.loadConfig(myOSystem.settings());

  // Create a surface for the TIA image and scanlines; we'll need them eventually
  myTiaSurface = myFB.allocateSurface(AtariNTSC::outWidth(kTIAW), kTIAH);

  // Generate scanline data, and a pre-defined scanline surface
  uInt32 scanData[kScanH];
  for(int i = 0; i < kScanH; i+=2)
  {
    scanData[i]   = 0x00000000;
    scanData[i+1] = 0xff000000;
  }
  mySLineSurface = myFB.allocateSurface(1, kScanH, scanData);

  // Base TIA surface for use in taking snapshots in 1x mode
  myBaseTiaSurface = myFB.allocateSurface(kTIAW*2, kTIAH);

  memset(myRGBFramebuffer, 0, sizeof(myRGBFramebuffer));

  // Enable/disable threading in the NTSC TV effects renderer
  myNTSCFilter.enableThreading(myOSystem.settings().getBool("threads"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::initialize(const Console& console, const VideoMode& mode)
{
  myTIA = &(console.tia());

  myTiaSurface->setDstPos(mode.image.x(), mode.image.y());
  myTiaSurface->setDstSize(mode.image.width(), mode.image.height());
  mySLineSurface->setDstPos(mode.image.x(), mode.image.y());
  mySLineSurface->setDstSize(mode.image.width(), mode.image.height());

  // Phosphor mode can be enabled either globally or per-ROM
  bool p_enable = myOSystem.settings().getString("tv.phosphor") == "always" ||
      console.properties().get(Display_Phosphor) == "YES";
  int p_blend = atoi(console.properties().get(Display_PPBlend).c_str());
  enablePhosphor(p_enable, p_blend);
  setNTSC(NTSCFilter::Preset(myOSystem.settings().getInt("tv.filter")), false);

  // Scanline repeating is sensitive to non-integral vertical resolution,
  // so rounding is performed to eliminate it
  // This won't be 100% accurate, but non-integral scaling isn't 100%
  // accurate anyway
  mySLineSurface->setSrcSize(1, int(2 * float(mode.image.height()) /
    floor((float(mode.image.height()) / myTIA->height()) + 0.5)));

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
void TIASurface::setPalette(const uInt32* tia_palette, const uInt32* rgb_palette)
{
  myPalette = tia_palette;

  // The NTSC filtering needs access to the raw RGB data, since it calculates
  // its own internal palette
  myNTSCFilter.setTIAPalette(rgb_palette);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FBSurface& TIASurface::baseSurface(GUI::Rect& rect) const
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
uInt32 TIASurface::pixel(uInt32 idx, uInt8 shift)
{
  return myPalette[*(myTIA->frameBuffer() + idx) | shift];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::setNTSC(NTSCFilter::Preset preset, bool show)
{
  ostringstream buf;
  if(preset == NTSCFilter::PRESET_OFF)
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
  if(ntscEnabled())
  {
    uInt32 intensity = enableScanlines(amount);
    buf << "Scanline intensity at " << intensity  << "%";
    myOSystem.settings().setValue("tv.scanlines", intensity);
  }
  else
    buf << "Scanlines only available in TV filtering mode";

  myFB.showMessage(buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::toggleScanlineInterpolation()
{
  ostringstream buf;
  if(ntscEnabled())
  {
    bool enable = !myOSystem.settings().getBool("tv.scaninter");
    enableScanlineInterpolation(enable);
    buf << "Scanline interpolation " << (enable ? "enabled" : "disabled");
    myOSystem.settings().setValue("tv.scaninter", enable);
  }
  else
    buf << "Scanlines only available in TV filtering mode";

  myFB.showMessage(buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TIASurface::enableScanlines(int relative, int absolute)
{
  FBSurface::Attributes& attr = mySLineSurface->attributes();
  if(relative == 0)  attr.blendalpha = absolute;
  else               attr.blendalpha += relative;
  attr.blendalpha = std::min(100u, attr.blendalpha);

  mySLineSurface->applyAttributes();
  mySLineSurface->setDirty();

  return attr.blendalpha;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::enableScanlineInterpolation(bool enable)
{
  FBSurface::Attributes& attr = mySLineSurface->attributes();
  attr.smoothing = enable;
  mySLineSurface->applyAttributes();
  mySLineSurface->setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::enablePhosphor(bool enable, int blend)
{
  myUsePhosphor = enable;
  if(blend >= 0)
    myPhosphorPercent = blend / 100.0;
  myFilter = Filter(enable ? uInt8(myFilter) | 0x01 : uInt8(myFilter) & 0x10);

  myTiaSurface->setDirty();
  mySLineSurface->setDirty();
  memset(myRGBFramebuffer, 0, sizeof(myRGBFramebuffer));

  // Precalculate the average colors for the 'phosphor' effect
  if(myUsePhosphor)
  {
    for(Int16 c = 255; c >= 0; c--)
      for(Int16 p = 255; p >= 0; p--)
        myPhosphorPalette[c][p] = getPhosphor(c, p);

    myNTSCFilter.setPhosphorPalette(myPhosphorPalette);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt32 TIASurface::getRGBPhosphor(const uInt32 c, const uInt32 p) const
{
  #define TO_RGB(color, red, green, blue) \
    const uInt8 red = color >> 16; const uInt8 green = color >> 8; const uInt8 blue = color;

  TO_RGB(c, rc, gc, bc);
  TO_RGB(p, rp, gp, bp);

  // Mix current calculated frame with previous displayed frame
  const uInt8 rn = myPhosphorPalette[rc][rp];
  const uInt8 gn = myPhosphorPalette[gc][gp];
  const uInt8 bn = myPhosphorPalette[bc][bp];

  return (rn << 16) | (gn << 8) | bn;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::enableNTSC(bool enable)
{
  myFilter = Filter(enable ? uInt8(myFilter) | 0x10 : uInt8(myFilter) & 0x01);

  // Normal vs NTSC mode uses different source widths
  myTiaSurface->setSrcSize(enable ?
      AtariNTSC::outWidth(kTIAW) : uInt32(kTIAW), myTIA->height());

  FBSurface::Attributes& tia_attr = myTiaSurface->attributes();
  tia_attr.smoothing = myOSystem.settings().getBool("tia.inter");
  myTiaSurface->applyAttributes();

  myScanlinesEnabled = enable;
  FBSurface::Attributes& sl_attr = mySLineSurface->attributes();
  sl_attr.smoothing  = myOSystem.settings().getBool("tv.scaninter");
  sl_attr.blending   = myScanlinesEnabled;
  sl_attr.blendalpha = myOSystem.settings().getInt("tv.scanlines");
  mySLineSurface->applyAttributes();

  myTiaSurface->setDirty();
  mySLineSurface->setDirty();
  memset(myRGBFramebuffer, 0, sizeof(myRGBFramebuffer));
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
      buf << myNTSCFilter.getPreset() << ", scanlines=" << attr.blendalpha << "/"
          << (attr.smoothing ? "inter" : "nointer");
      break;
    case Filter::BlarggPhosphor:
      buf << myNTSCFilter.getPreset() << ", phosphor, scanlines="
          << attr.blendalpha << "/" << (attr.smoothing ? "inter" : "nointer");
      break;
  }
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::render()
{
  uInt32 width  = myTIA->width();
  uInt32 height = myTIA->height();

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
      uInt32* rgbIn = myRGBFramebuffer;

      uInt32 bufofs = 0, screenofsY = 0, pos;
      for(uInt32 y = height; y ; --y)
      {
        pos = screenofsY;
        for(uInt32 x = width / 2; x ; --x)
        {
          // Store back into displayed frame buffer (for next frame)
          rgbIn[bufofs] = out[pos++] = getRGBPhosphor(myPalette[tiaIn[bufofs]], rgbIn[bufofs]);
          bufofs++;
          rgbIn[bufofs] = out[pos++] = getRGBPhosphor(myPalette[tiaIn[bufofs]], rgbIn[bufofs]);
          bufofs++;
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
      myNTSCFilter.render(myTIA->frameBuffer(), width, height, out, outPitch << 2, myRGBFramebuffer);
      break;
    }
  }

  // Draw TIA image
  myTiaSurface->setDirty();
  myTiaSurface->render();

  // Draw overlaying scanlines
  if(myScanlinesEnabled)
  {
    mySLineSurface->setDirty();
    mySLineSurface->render();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::reRender()
{
  uInt32 width = myTIA->width();
  uInt32 height = myTIA->height();
  uInt32 pos = 0;
  uInt32 *outPtr, outPitch;

  myTiaSurface->basePtr(outPtr, outPitch);

  switch (myFilter)
  {
    // for non-phosphor modes, render the frame again
    case Filter::Normal:
    case Filter::BlarggNormal:
      render();
      break;
    // for phosphor modes, copy the phosphor framebuffer
    case Filter::Phosphor:
      for (uInt32 y = height; y; --y)
      {
        memcpy(outPtr, myRGBFramebuffer + pos, width);
        outPtr += outPitch;
        pos += width;
      }
      break;
    case Filter::BlarggPhosphor:
      memcpy(outPtr, myRGBFramebuffer, height * outPitch << 2);
      break;
  }

  if (myUsePhosphor)
  {
    // Draw TIA image
    myTiaSurface->setDirty();
    myTiaSurface->render();

    // Draw overlaying scanlines
    if (myScanlinesEnabled)
    {
      mySLineSurface->setDirty();
      mySLineSurface->render();
    }
  }
}
