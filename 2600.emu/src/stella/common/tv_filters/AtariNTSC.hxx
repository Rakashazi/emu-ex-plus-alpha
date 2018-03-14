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

/*
 * Atari TIA NTSC video filter
 * Based on nes_ntsc 0.2.2. http://www.slack.net/~ant
 *
 * Copyright (C) 2006-2009 Shay Green. This module is free software; you
 * can redistribute it and/or modify it under the terms of the GNU Lesser
 * General Public License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version. This
 * module is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details. You should have received a copy of the GNU Lesser General Public
 * License along with this module; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
  The class is basically a thin wrapper around atari_ntsc_xxx structs
  and methods, so that the rest of the codebase isn't affected by
  updated versions of Blargg code.
*/

#ifndef ATARI_NTSC_HXX
#define ATARI_NTSC_HXX

#include <cmath>
#include <thread>

#include "bspf.hxx"

class AtariNTSC
{
  public:
    enum {
      palette_size = 256,
      entry_size = 2 * 14,
    };

    // By default, threading is turned off
    AtariNTSC() { enableThreading(false); }

    // Image parameters, ranging from -1.0 to 1.0. Actual internal values shown
    // in parenthesis and should remain fairly stable in future versions.
    struct Setup
    {
      // Basic parameters
      double hue;        // -1 = -180 degrees     +1 = +180 degrees
      double saturation; // -1 = grayscale (0.0)  +1 = oversaturated colors (2.0)
      double contrast;   // -1 = dark (0.5)       +1 = light (1.5)
      double brightness; // -1 = dark (0.5)       +1 = light (1.5)
      double sharpness;  // edge contrast enhancement/blurring

      // Advanced parameters
      double gamma;      // -1 = dark (1.5)       +1 = light (0.5)
      double resolution; // image resolution
      double artifacts;  // artifacts caused by color changes
      double fringing;   // color artifacts caused by brightness changes
      double bleed;      // color bleed (color resolution reduction)
    };

    // Video format presets
    static const Setup TV_Composite; // color bleeding + artifacts
    static const Setup TV_SVideo;    // color bleeding only
    static const Setup TV_RGB;       // crisp image
    static const Setup TV_Bad;       // badly adjusted TV

    // Initializes and adjusts parameters.
    void initialize(const Setup& setup, const uInt8* palette);
    void initializePalette(const uInt8* palette);

    // Set up threading
    void enableThreading(bool enable);

    // Set phosphor palette, for use in Blargg + phosphor mode
    void setPhosphorPalette(uInt8 palette[256][256]) {
      memcpy(myPhosphorPalette, palette, 256 * 256);
    }

    // Filters one or more rows of pixels. Input pixels are 8-bit Atari
    // palette colors.
    //  In_row_width is the number of pixels to get to the next input row.
    //  Out_pitch is the number of *bytes* to get to the next output row.
    void render(const uInt8* atari_in, const uInt32 in_width, const uInt32 in_height,
                void* rgb_out, const uInt32 out_pitch, uInt32* rgb_in = nullptr);

    // Number of input pixels that will fit within given output width.
    // Might be rounded down slightly; use outWidth() on result to find
    // rounded value.
    static constexpr uInt32 inWidth( uInt32 out_width ) {
      return (((out_width-5) / PIXEL_out_chunk - 1) * PIXEL_in_chunk + 1);
    }

    // Number of output pixels written by blitter for given input width.
    // Width might be rounded down slightly; use inWidth() on result to
    // find rounded value. Guaranteed not to round 160 down at all.
    static constexpr uInt32 outWidth(uInt32 in_width) {
      return ((((in_width) - 1) / PIXEL_in_chunk + 1)* PIXEL_out_chunk) + 5;
    }

  private:
    // Threaded rendering
    void renderThread(const uInt8* atari_in, const uInt32 in_width,
      const uInt32 in_height, const uInt32 numThreads, const uInt32 threadNum, void* rgb_out, const uInt32 out_pitch);
    void renderWithPhosphorThread(const uInt8* atari_in, const uInt32 in_width,
      const uInt32 in_height, const uInt32 numThreads, const uInt32 threadNum, uInt32* rgb_in, void* rgb_out, const uInt32 out_pitch);

    /**
      Used to calculate an averaged color for the 'phosphor' effect.

      @param c  RGB Color 1 (current frame)
      @param cp RGB Color 2 (previous frame)

      @return  Averaged value of the two RGB colors
    */
    uInt32 getRGBPhosphor(const uInt32 c, const uInt32 cp) const;

  private:
    enum {
      PIXEL_in_chunk  = 2,   // number of input pixels read per chunk
      PIXEL_out_chunk = 7,   // number of output pixels generated per chunk
      NTSC_black      = 0,   // palette index for black

      alignment_count = 2,
      burst_count     = 1,
      rescale_in      = 8,
      rescale_out     = 7,

      burst_size  = entry_size / burst_count,
      kernel_half = 16,
      kernel_size = kernel_half * 2 + 1,
      gamma_size  = 256,

      rgb_builder = ((1 << 21) | (1 << 11) | (1 << 1)),
      rgb_kernel_size = burst_size / alignment_count,
      rgb_bits = 8,
      rgb_unit = (1 << rgb_bits),
      rgb_bias = rgb_unit * 2 * rgb_builder,

      std_decoder_hue = 0,
      ext_decoder_hue = std_decoder_hue + 15
    };

    #define artifacts_mid   1.5f
    #define artifacts_max   2.5f
    #define fringing_mid    1.0f
    #define fringing_max    2.0f
    #define rgb_offset      (rgb_unit * 2 + 0.5f)

    #undef PI
    #define PI 3.14159265358979323846f
    #define LUMA_CUTOFF 0.20

    uInt32 myColorTable[palette_size][entry_size];
    uInt8 myPhosphorPalette[256][256];

    // Rendering threads
    unique_ptr<std::thread[]> myThreads;
    // Number of rendering and total threads
    uInt32 myWorkerThreads, myTotalThreads;

    struct init_t
    {
      float to_rgb [burst_count * 6];
      float to_float [gamma_size];
      float contrast;
      float brightness;
      float artifacts;
      float fringing;
      float kernel [rescale_out * kernel_size * 2];
    };
    init_t myImpl;

    struct pixel_info_t
    {
      int offset;
      float negate;
      float kernel [4];
    };
    static const pixel_info_t atari_ntsc_pixels[alignment_count];

    static const float default_decoder[6];

    void init(init_t& impl, const Setup& setup);
    void initFilters(init_t& impl, const Setup& setup);
    // Generate pixel at all burst phases and column alignments
    void genKernel(init_t& impl, float y, float i, float q, uInt32* out);

    // Begins outputting row and starts two pixels. First pixel will be cut
    // off a bit.  Use atari_ntsc_black for unused pixels.
    #define ATARI_NTSC_BEGIN_ROW( pixel0, pixel1 ) \
    	unsigned const atari_ntsc_pixel0_ = (pixel0);\
    	uInt32 const* kernel0  = myColorTable[atari_ntsc_pixel0_];\
    	unsigned const atari_ntsc_pixel1_ = (pixel1);\
    	uInt32 const* kernel1  = myColorTable[atari_ntsc_pixel1_];\
    	uInt32 const* kernelx0;\
    	uInt32 const* kernelx1 = kernel0

    // Begins input pixel
    #define ATARI_NTSC_COLOR_IN( index, color ) {\
    	unsigned color_;\
    	kernelx##index = kernel##index;\
    	kernel##index = (color_ = (color), myColorTable[color_]);\
    }

    // Generates output in the specified 32-bit format (x = junk bits).
    //  native: xxxRRRRR RRRxxGGG GGGGGxxB BBBBBBBx (native internal format)
    //  8888:   00000000 RRRRRRRR GGGGGGGG BBBBBBBB (8-8-8-8 32-bit ARGB)
    #define ATARI_NTSC_RGB_OUT_8888( index, rgb_out ) {\
      uInt32 raw_ =\
        kernel0  [index       ] + kernel1  [(index+10)%7+14] +\
        kernelx0 [(index+7)%14] + kernelx1 [(index+ 3)%7+14+7];\
      ATARI_NTSC_CLAMP_( raw_, 0 );\
      rgb_out = (raw_>>5 & 0x00FF0000)|(raw_>>3 & 0x0000FF00)|(raw_>>1 & 0x000000FF);\
    }

    // Common ntsc macros
    #define atari_ntsc_clamp_mask     (rgb_builder * 3 / 2)
    #define atari_ntsc_clamp_add      (rgb_builder * 0x101)
    #define ATARI_NTSC_CLAMP_( io, shift ) {\
    	uInt32 sub = (io) >> (9-(shift)) & atari_ntsc_clamp_mask;\
    	uInt32 clamp = atari_ntsc_clamp_add - sub;\
    	io |= clamp;\
    	clamp -= sub;\
    	io &= clamp;\
    }

    // Kernel generation
    #define ROTATE_IQ( i, q, sin_b, cos_b ) {\
      float t;\
      t = i * cos_b - q * sin_b;\
      q = i * sin_b + q * cos_b;\
      i = t;\
    }
    #define RGB_TO_YIQ( r, g, b, y, i ) (\
      (y = (r) * 0.299f + (g) * 0.587f + (b) * 0.114f),\
      (i = (r) * 0.595716f - (g) * 0.274453f - (b) * 0.321263f),\
      ((r) * 0.211456f - (g) * 0.522591f + (b) * 0.311135f)\
    )
    #define YIQ_TO_RGB( y, i, q, to_rgb, type, r, g ) (\
      r = type(y + to_rgb [0] * i + to_rgb [1] * q),\
      g = type(y + to_rgb [2] * i + to_rgb [3] * q),\
      type(y + to_rgb [4] * i + to_rgb [5] * q)\
    )
    #ifndef PACK_RGB
      #define PACK_RGB( r, g, b ) ((r) << 21 | (g) << 11 | (b) << 1)
    #endif

    #define PIXEL_OFFSET_( ntsc, scaled ) \
      (kernel_size / 2 + ntsc + (scaled != 0) + (rescale_out - scaled) % rescale_out + \
          (kernel_size * 2 * scaled))

    #define PIXEL_OFFSET( ntsc, scaled ) \
      PIXEL_OFFSET_( ((ntsc) - (scaled) / rescale_out * rescale_in),\
          (((scaled) + rescale_out * 10) % rescale_out) ),\
      (1.0f - (((ntsc) + 100) & 2))

    #define DISTRIBUTE_ERROR( a, b, c ) {\
      uInt32 fourth = (error + 2 * rgb_builder) >> 2;\
      fourth &= (rgb_bias >> 1) - rgb_builder;\
      fourth -= rgb_bias >> 2;\
      out [a] += fourth;\
      out [b] += fourth;\
      out [c] += fourth;\
      out [i] += error - (fourth * 3);\
    }

    #define RGB_PALETTE_OUT( rgb, out_ )\
    {\
      unsigned char* out = (out_);\
      uInt32 clamped = (rgb);\
      ATARI_NTSC_CLAMP_( clamped, (8 - rgb_bits) );\
      out [0] = (unsigned char) (clamped >> 21);\
      out [1] = (unsigned char) (clamped >> 11);\
      out [2] = (unsigned char) (clamped >>  1);\
    }
};

#endif
