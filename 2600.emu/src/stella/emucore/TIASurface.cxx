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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: TIASurface.cxx 3131 2015-01-01 03:49:32Z stephena $
//============================================================================

#include <cmath>

#include "FrameBuffer.hxx"
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
    myFilterType(kNormal),
    myUsePhosphor(false),
    myPhosphorBlend(77),
    myScanlinesEnabled(false),
    myPalette(nullptr)
{
  // Load NTSC filter settings
  myNTSCFilter.loadConfig(myOSystem.settings());

  // Create a surface for the TIA image and scanlines; we'll need them eventually
  myTiaSurface = myFB.allocateSurface(ATARI_NTSC_OUT_WIDTH(kTIAW), kTIAH);

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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::initialize(const Console& console, const VideoMode& mode)
{
  myTIA = &(console.tia());

  myTiaSurface->setDstPos(mode.image.x(), mode.image.y());
  myTiaSurface->setDstSize(mode.image.width(), mode.image.height());
  mySLineSurface->setDstPos(mode.image.x(), mode.image.y());
  mySLineSurface->setDstSize(mode.image.width(), mode.image.height());

  bool p_enable = console.properties().get(Display_Phosphor) == "YES";
  int p_blend = atoi(console.properties().get(Display_PPBlend).c_str());
  enablePhosphor(p_enable, p_blend);
  setNTSC((NTSCFilter::Preset)myOSystem.settings().getInt("tv.filter"), false);

  // Scanline repeating is sensitive to non-integral vertical resolution,
  // so rounding is performed to eliminate it
  // This won't be 100% accurate, but non-integral scaling isn't 100%
  // accurate anyway
  mySLineSurface->setSrcSize(1, int(2 * float(mode.image.height()) /
    floor(((float)mode.image.height() / myTIA->height()) + 0.5)));

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

  // Set palette for phosphor effect
  for(int i = 0; i < 256; ++i)
  {
    for(int j = 0; j < 256; ++j)
    {
      uInt8 ri = (rgb_palette[i] >> 16) & 0xff;
      uInt8 gi = (rgb_palette[i] >> 8) & 0xff;
      uInt8 bi = rgb_palette[i] & 0xff;
      uInt8 rj = (rgb_palette[j] >> 16) & 0xff;
      uInt8 gj = (rgb_palette[j] >> 8) & 0xff;
      uInt8 bj = rgb_palette[j] & 0xff;

      Uint8 r = (Uint8) getPhosphor(ri, rj);
      Uint8 g = (Uint8) getPhosphor(gi, gj);
      Uint8 b = (Uint8) getPhosphor(bi, bj);

      myPhosphorPalette[i][j] = myFB.mapRGB(r, g, b);
    }
  }

  // The NTSC filtering needs access to the raw RGB data, since it calculates
  // its own internal palette
  myNTSCFilter.setTIAPalette(*this, rgb_palette);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FBSurface& TIASurface::baseSurface(GUI::Rect& rect)
{
  uInt32 tiaw = myTIA->width(), width = tiaw*2, height = myTIA->height();
  rect.setBounds(0, 0, width, height);

  // Fill the surface with pixels from the TIA, scaled 2x horizontally
  uInt32 *buf_ptr, pitch;
  myBaseTiaSurface->basePtr(buf_ptr, pitch);

  for(uInt32 y = 0; y < height; ++y)
  {
    for(uInt32 x = 0; x < tiaw; ++x)
    {
      uInt32 pixel = myFB.tiaSurface().pixel(y*tiaw+x);
      *buf_ptr++ = pixel;
      *buf_ptr++ = pixel;
    }
  }

  return *myBaseTiaSurface;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TIASurface::pixel(uInt32 idx, uInt8 shift) const
{
  uInt8 c = *(myTIA->currentFrameBuffer() + idx) | shift;
  uInt8 p = *(myTIA->previousFrameBuffer() + idx) | shift;

  return (!myUsePhosphor ? myPalette[c] : myPhosphorPalette[c][p]);
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
  myOSystem.settings().setValue("tv.filter", (int)preset);

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
  attr.blendalpha = BSPF_max(0u, attr.blendalpha);
  attr.blendalpha = BSPF_min(100u, attr.blendalpha);

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
  myUsePhosphor   = enable;
  myPhosphorBlend = blend;
  myFilterType = FilterType(enable ? myFilterType | 0x01 : myFilterType & 0x10);
  myTiaSurface->setDirty();
  mySLineSurface->setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIASurface::getPhosphor(uInt8 c1, uInt8 c2) const
{
  if(c2 > c1)
    BSPF_swap(c1, c2);

  return ((c1 - c2) * myPhosphorBlend)/100 + c2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::enableNTSC(bool enable)
{
  myFilterType = FilterType(enable ? myFilterType | 0x10 : myFilterType & 0x01);

  // Normal vs NTSC mode uses different source widths
  myTiaSurface->setSrcSize(enable ? ATARI_NTSC_OUT_WIDTH(160) : 160, myTIA->height());

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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIASurface::effectsInfo() const
{
  const FBSurface::Attributes& attr = mySLineSurface->attributes();

  ostringstream buf;
  switch(myFilterType)
  {
    case kNormal:
      buf << "Disabled, normal mode";
      break;
    case kPhosphor:
      buf << "Disabled, phosphor mode";
      break;
    case kBlarggNormal:
      buf << myNTSCFilter.getPreset() << ", scanlines=" << attr.blendalpha << "/"
          << (attr.smoothing ? "inter" : "nointer");
      break;
    case kBlarggPhosphor:
      buf << myNTSCFilter.getPreset() << ", phosphor, scanlines="
          << attr.blendalpha << "/" << (attr.smoothing ? "inter" : "nointer");
      break;
  }
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASurface::render()
{
  // Copy the mediasource framebuffer to the RGB texture
  // In hardware rendering mode, it's faster to just assume that the screen
  // is dirty and always do an update

  uInt8* currentFrame  = myTIA->currentFrameBuffer();
  uInt8* previousFrame = myTIA->previousFrameBuffer();
  uInt32 width         = myTIA->width();
  uInt32 height        = myTIA->height();

  uInt32 *buffer, pitch;
  myTiaSurface->basePtr(buffer, pitch);

  // TODO - Eventually 'phosphor' won't be a separate mode, and will become
  //        a post-processing filter by blending several frames.
  switch(myFilterType)
  {
    case kNormal:
    {
      uInt32 bufofsY    = 0;
      uInt32 screenofsY = 0;
      for(uInt32 y = 0; y < height; ++y)
      {
        uInt32 pos = screenofsY;
        for(uInt32 x = 0; x < width; ++x)
          buffer[pos++] = (uInt32) myPalette[currentFrame[bufofsY + x]];

        bufofsY    += width;
        screenofsY += pitch;
      }
      break;
    }
    case kPhosphor:
    {
      uInt32 bufofsY    = 0;
      uInt32 screenofsY = 0;
      for(uInt32 y = 0; y < height; ++y)
      {
        uInt32 pos = screenofsY;
        for(uInt32 x = 0; x < width; ++x)
        {
          const uInt32 bufofs = bufofsY + x;
          buffer[pos++] = (uInt32)
            myPhosphorPalette[currentFrame[bufofs]][previousFrame[bufofs]];
        }
        bufofsY    += width;
        screenofsY += pitch;
      }
      break;
    }
    case kBlarggNormal:
    {
      myNTSCFilter.blit_single(currentFrame, width, height,
                               buffer, pitch << 2);
      break;
    }
    case kBlarggPhosphor:
    {
      myNTSCFilter.blit_double(currentFrame, previousFrame, width, height,
                               buffer, pitch << 2);
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
