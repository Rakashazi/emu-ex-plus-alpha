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
// $Id: FBSurface.hxx 3131 2015-01-01 03:49:32Z stephena $
//============================================================================

#ifndef FBSURFACE_HXX
#define FBSURFACE_HXX

class FrameBuffer;
class TIASurface;

#include "bspf.hxx"
#include "Font.hxx"
#include "Rect.hxx"

/**
  This class is basically a thin wrapper around the video toolkit 'surface'
  structure.  We do it this way so the actual video toolkit won't be dragged
  into the depths of the codebase.  All drawing is done into FBSurfaces,
  which are then drawn into the FrameBuffer.  Each FrameBuffer-derived class
  is responsible for extending an FBSurface object suitable to the
  FrameBuffer type.

  @author  Stephen Anthony
*/

// Text alignment modes for drawString()
enum TextAlignment {
  kTextAlignLeft,
  kTextAlignCenter,
  kTextAlignRight
};
// Line types for drawing rectangular frames
enum FrameStyle {
  kSolidLine,
  kDashLine
};

class FBSurface
{
//  friend class TIASurface;

  public:
    /**
      Creates a new FBSurface object

      @param data  If non-null, the data values to use as a static surface
    */
    FBSurface();

    /**
      Destructor
    */
    virtual ~FBSurface() { }

    /**
      This method returns the surface pixel pointer and pitch, which are
      used when one wishes to modify the surface pixels directly.
    */
    inline void basePtr(uInt32*& pixels, uInt32& pitch) const
    {
      pixels = myPixels;
      pitch = myPitch;
    }

    /**
      This method is called to get a copy of the specified ARGB data from
      the behind-the-scenes surface.

      @param buffer  A copy of the pixel data in ARGB8888 format
      @param pitch   The pitch (in bytes) for the pixel data
      @param rect    The bounding rectangle for the buffer
    */
    void readPixels(uInt8* buffer, uInt32 pitch, const GUI::Rect& rect) const;

    //////////////////////////////////////////////////////////////////////////
    // Note:  The drawing primitives below will work, but do not take
    //        advantage of any acceleration whatsoever.  The methods are
    //        marked as 'virtual' so that child classes can choose to
    //        implement them more efficiently.
    //////////////////////////////////////////////////////////////////////////

    /**
      This method should be called to draw a horizontal line.

      @param x      The first x coordinate
      @param y      The y coordinate
      @param x2     The second x coordinate
      @param color  The color of the line
    */
    virtual void hLine(uInt32 x, uInt32 y, uInt32 x2, uInt32 color);

    /**
      This method should be called to draw a vertical line.

      @param x      The x coordinate
      @param y      The first y coordinate
      @param y2     The second y coordinate
      @param color  The color of the line
    */
    virtual void vLine(uInt32 x, uInt32 y, uInt32 y2, uInt32 color);

    /**
      This method should be called to draw a filled rectangle.

      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the area
      @param h      The height of the area
      @param color  The fill color of the rectangle
    */
    virtual void fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                          uInt32 color);

    /**
      This method should be called to draw the specified character.

      @param font   The font to use to draw the character
      @param c      The character to draw
      @param x      The x coordinate
      @param y      The y coordinate
      @param color  The color of the character
    */
    virtual void drawChar(const GUI::Font& font, uInt8 c, uInt32 x, uInt32 y,
                          uInt32 color);

    /**
      This method should be called to draw the bitmap image.

      @param bitmap The data to draw
      @param x      The x coordinate
      @param y      The y coordinate
      @param color  The color of the bitmap
      @param h      The height of the data image
    */
    virtual void drawBitmap(uInt32* bitmap, uInt32 x, uInt32 y, uInt32 color,
                            uInt32 h = 8);

    /**
      This method should be called to convert and copy a given row of pixel
      data into a FrameBuffer surface.  The pixels must already be in the
      format used by the surface.

      @param data     The data in uInt8 R/G/B format
      @param row      The row of the surface the data should be placed in
      @param rowbytes The number of bytes in row of 'data'
    */
    virtual void drawPixels(uInt32* data, uInt32 x, uInt32 y, uInt32 numpixels);

    /**
      This method should be called to draw a rectangular box with sides
      at the specified coordinates.

      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the box
      @param h      The height of the box
      @param colorA Lighter color for outside line.
      @param colorB Darker color for inside line.
    */
    virtual void box(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                     uInt32 colorA, uInt32 colorB);

    /**
      This method should be called to draw a framed rectangle with
      several different possible styles.

      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the area
      @param h      The height of the area
      @param color  The color of the surrounding frame
      @param style  The 'FrameStyle' to use for the surrounding frame
    */
    virtual void frameRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                           uInt32 color, FrameStyle style = kSolidLine);

    /**
      This method should be called to draw the specified string.

      @param font   The font to draw the string with
      @param str    The string to draw
      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the string area
      @param h      The height of the string area
      @param color  The color of the text
      @param align  The alignment of the text in the string width area
      @param deltax 
      @param useEllipsis  Whether to use '...' when the string is too long
    */
    virtual void drawString(
        const GUI::Font& font, const string& s, int x, int y, int w,
        uInt32 color, TextAlignment align = kTextAlignLeft,
        int deltax = 0, bool useEllipsis = true);

    /**
      This method should be called to indicate that the surface has been
      modified, and should be redrawn at the next interval.
    */
    virtual void setDirty() { }

    //////////////////////////////////////////////////////////////////////////
    // Note:  The following methods are FBSurface-specific, and must be
    //        implemented in child classes.
    //
    //  For the following, 'src' indicates the actual data buffer area
    //  (non-scaled) and 'dst' indicates the rendered area (possibly scaled).
    //////////////////////////////////////////////////////////////////////////

    /**
      These methods answer the current *real* dimensions of the specified
      surface.
    */
    virtual uInt32 width() const = 0;
    virtual uInt32 height() const = 0;

    /**
      These methods answer the current *rendering* dimensions of the
      specified surface.
    */
    virtual const GUI::Rect& srcRect() const = 0;
    virtual const GUI::Rect& dstRect() const = 0;

    /**
      These methods set the origin point and width/height for the
      specified service.  They are defined as separate x/y and w/h
      methods since these items are sometimes set separately.
    */
    virtual void setSrcPos(uInt32 x, uInt32 y)  = 0;
    virtual void setSrcSize(uInt32 w, uInt32 h) = 0;
    virtual void setDstPos(uInt32 x, uInt32 y)  = 0;
    virtual void setDstSize(uInt32 w, uInt32 h) = 0;

    /**
      This method should be called to enable/disable showing the surface
      (ie, if hidden it will not be drawn under any circumstances.
    */
    virtual void setVisible(bool visible) = 0;

    /**
      This method should be called to translate the given coordinates
      to the (destination) surface coordinates.

      @param x  X coordinate to translate
      @param y  Y coordinate to translate
    */
    virtual void translateCoords(Int32& x, Int32& y) const = 0;

    /**
      This method should be called to draw the surface to the screen.
      It will return true if rendering actually occurred.
    */
    virtual bool render() = 0;

    /**
      This method should be called to reset the surface to empty
      pixels / colour black.
    */
    virtual void invalidate() = 0;

    /**
      This method should be called to free any resources being used by
      the surface.
    */
    virtual void free() = 0;

    /**
      This method should be called to reload the surface data/state.
      It will normally be called after free().
    */
    virtual void reload() = 0;

    /**
      This method should be called to resize the surface to the
      given dimensions and reload data/state.  The surface is not
      modified if it is larger than the given dimensions.
    */
    virtual void resize(uInt32 width, uInt32 height) = 0;

    /**
      The rendering attributes that can be modified for this texture.
      These probably can only be implemented in child FBSurfaces where
      the specific functionality actually exists.
    */
    struct Attributes {
      bool smoothing;    // Scaling is smoothed or blocky
      bool blending;     // Blending is enabled
      uInt32 blendalpha; // Alpha to use in blending mode (0-100%)
    };

    /**
      Get the currently applied attributes.
    */
    Attributes& attributes() { return myAttributes; }

    /**
      The child class chooses which (if any) of the actual attributes
      can be applied.

      @param immediate  Whether to re-initialize the surface immediately
                        with the new attributes, or wait until manually
                        reloaded
    */
    virtual void applyAttributes(bool immediate = true) = 0;

    static void setPalette(const uInt32* palette) { myPalette = palette; }

  private:
    // Copy constructor and assignment operator not supported
    FBSurface(const FBSurface&);
    FBSurface& operator = (const FBSurface&);

  protected:
    static const uInt32* myPalette;
    uInt32* myPixels;
    uInt32 myPitch;

    Attributes myAttributes;
};

#endif
