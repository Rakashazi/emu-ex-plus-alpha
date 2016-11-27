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
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: atari_ntsc.hxx 3239 2015-12-29 19:22:46Z stephena $
//============================================================================

/* Atari TIA, CTIA, GTIA and MARIA NTSC video filter */

#ifndef ATARI_NTSC_H
#define ATARI_NTSC_H

typedef unsigned char atari_ntsc_in_t;
typedef unsigned int atari_ntsc_out_t;

#ifdef __cplusplus
  extern "C" {
#endif

/* Image parameters, ranging from -1.0 to 1.0. Actual internal values shown
   in parenthesis and should remain fairly stable in future versions. */
typedef struct atari_ntsc_setup_t
{
  /* Basic parameters */
  double hue;        /* -1 = -180 degrees     +1 = +180 degrees */
  double saturation; /* -1 = grayscale (0.0)  +1 = oversaturated colors (2.0) */
  double contrast;   /* -1 = dark (0.5)       +1 = light (1.5) */
  double brightness; /* -1 = dark (0.5)       +1 = light (1.5) */
  double sharpness;  /* edge contrast enhancement/blurring */

  /* Advanced parameters */
  double gamma;      /* -1 = dark (1.5)       +1 = light (0.5) */
  double resolution; /* image resolution */
  double artifacts;  /* artifacts caused by color changes */
  double fringing;   /* color artifacts caused by brightness changes */
  double bleed;      /* color bleed (color resolution reduction) */
  float const* decoder_matrix; /* optional RGB decoder matrix, 6 elements */
} atari_ntsc_setup_t;

/* Video format presets */
extern atari_ntsc_setup_t const atari_ntsc_composite; /* color bleeding + artifacts */
extern atari_ntsc_setup_t const atari_ntsc_svideo;    /* color bleeding only */
extern atari_ntsc_setup_t const atari_ntsc_rgb;       /* crisp image */
extern atari_ntsc_setup_t const atari_ntsc_bad;       /* badly adjusted TV */

enum { atari_ntsc_palette_size = 129 * 128 };

/* Initializes and adjusts parameters. Can be called multiple times on the same
   atari_ntsc_t object. Can pass NULL for either parameter. */
typedef struct atari_ntsc_t atari_ntsc_t;
void atari_ntsc_init( atari_ntsc_t* ntsc, atari_ntsc_setup_t const* setup,
                      atari_ntsc_in_t const* palette );

/* Filters one or more rows of pixels. Input pixels are 8-bit Atari palette colors.
   In_row_width is the number of pixels to get to the next input row. Out_pitch
   is the number of *bytes* to get to the next output row. */
void atari_ntsc_blit_single( atari_ntsc_t const* ntsc,
    atari_ntsc_in_t const* atari_in,
    long in_row_width, int in_width, int in_height,
    void* rgb_out, long out_pitch );
void atari_ntsc_blit_double( atari_ntsc_t const* ntsc,
    atari_ntsc_in_t const* atari_in1, atari_ntsc_in_t const* atari_in2,
    long in_row_width, int in_width, int in_height,
    void* rgb_out, long out_pitch );

/* Number of output pixels written by blitter for given input width. Width might
   be rounded down slightly; use ATARI_NTSC_IN_WIDTH() on result to find rounded
   value. Guaranteed not to round 160 down at all. */
#define ATARI_NTSC_OUT_WIDTH( in_width ) \
  ((((in_width) - 1) / atari_ntsc_in_chunk + 1)* atari_ntsc_out_chunk)

/* Number of input pixels that will fit within given output width. Might be
rounded down slightly; use ATARI_NTSC_OUT_WIDTH() on result to find rounded
value. */
#define ATARI_NTSC_IN_WIDTH( out_width ) \
  (((out_width) / atari_ntsc_out_chunk - 1) * atari_ntsc_in_chunk + 1)


/* Interface for user-defined custom blitters. */

enum { atari_ntsc_in_chunk  = 2  }; /* number of input pixels read per chunk */
enum { atari_ntsc_out_chunk = 7  }; /* number of output pixels generated per chunk */
enum { atari_ntsc_black     = 0  }; /* palette index for black */

/* Begins outputting row and starts two pixels. First pixel will be cut off a bit.
   Use atari_ntsc_black for unused pixels. Declares variables, so must be before first
   statement in a block (unless you're using C++). */
#define ATARI_NTSC_BEGIN_ROW( ntsc, pixel0, pixel1 ) \
	ATARI_NTSC_BEGIN_ROW_6_( pixel0, pixel1, ATARI_NTSC_ENTRY_, ntsc )

/* Begins input pixel */
#define ATARI_NTSC_COLOR_IN( in_index, ntsc, color_in ) \
	ATARI_NTSC_COLOR_IN_( in_index, color_in, ATARI_NTSC_ENTRY_, ntsc )

/* Generates output in the specified 32-bit format (x = junk bits).
    native: xxxRRRRR RRRxxGGG GGGGGxxB BBBBBBBx (native internal format)
    8888:   00000000 RRRRRRRR GGGGGGGG BBBBBBBB (8-8-8-8 32-bit ARGB)
*/
#define ATARI_NTSC_RGB_OUT_8888( index, rgb_out ) {\
  atari_ntsc_rgb_t raw_ =\
    kernel0  [index       ] + kernel1  [(index+10)%7+14] +\
    kernelx0 [(index+7)%14] + kernelx1 [(index+ 3)%7+14+7];\
  ATARI_NTSC_CLAMP_( raw_, 0 );\
  rgb_out = (raw_>>5 & 0x00FF0000)|(raw_>>3 & 0x0000FF00)|(raw_>>1 & 0x000000FF);\
}

/* private */
enum { atari_ntsc_entry_size = 2 * 14 };
typedef unsigned long atari_ntsc_rgb_t;
struct atari_ntsc_t {
	atari_ntsc_rgb_t table [atari_ntsc_palette_size] [atari_ntsc_entry_size];
};

#define ATARI_NTSC_ENTRY_( ntsc, n ) (ntsc)->table [n]

/* common 3->7 ntsc macros */
#define ATARI_NTSC_BEGIN_ROW_6_( pixel0, pixel1, ENTRY, table ) \
	unsigned const atari_ntsc_pixel0_ = (pixel0);\
	atari_ntsc_rgb_t const* kernel0  = ENTRY( table, atari_ntsc_pixel0_ );\
	unsigned const atari_ntsc_pixel1_ = (pixel1);\
	atari_ntsc_rgb_t const* kernel1  = ENTRY( table, atari_ntsc_pixel1_ );\
	atari_ntsc_rgb_t const* kernelx0;\
	atari_ntsc_rgb_t const* kernelx1 = kernel0

/* common ntsc macros */
#define atari_ntsc_rgb_builder    ((1L << 21) | (1 << 11) | (1 << 1))
#define atari_ntsc_clamp_mask     (atari_ntsc_rgb_builder * 3 / 2)
#define atari_ntsc_clamp_add      (atari_ntsc_rgb_builder * 0x101)
#define ATARI_NTSC_CLAMP_( io, shift ) {\
	atari_ntsc_rgb_t sub = (io) >> (9-(shift)) & atari_ntsc_clamp_mask;\
	atari_ntsc_rgb_t clamp = atari_ntsc_clamp_add - sub;\
	io |= clamp;\
	clamp -= sub;\
	io &= clamp;\
}

#define ATARI_NTSC_COLOR_IN_( index, color, ENTRY, table ) {\
	unsigned color_;\
	kernelx##index = kernel##index;\
	kernel##index = (color_ = (color), ENTRY( table, color_ ));\
}

#ifdef __cplusplus
	}
#endif

#endif
