#pragma once

// Based on code by Unicode Inc.
// TODO: look into using ICU

#include <util/ansiTypes.h>

namespace UTF
{

/* Some fundamental constants */
static const uint32 UNI_REPLACEMENT_CHAR = 0x0000FFFD;
static const uint32 UNI_MAX_BMP = 0x0000FFFF;
static const uint32 UNI_MAX_UTF16 = 0x0010FFFF;
static const uint32 UNI_MAX_UTF32 = 0x7FFFFFFF;
static const uint32 UNI_MAX_LEGAL_UTF32 = 0x0010FFFF;

static const uint32 UNI_SUR_HIGH_START = 0xD800;
static const uint32 UNI_SUR_HIGH_END = 0xDBFF;
static const uint32 UNI_SUR_LOW_START = 0xDC00;
static const uint32 UNI_SUR_LOW_END = 0xDFFF;

/*
 * Index into the table below with the first byte of a UTF-8 sequence to
 * get the number of trailing bytes that are supposed to follow it.
 * Note that *legal* UTF-8 values can't have 4 or 5-bytes. The table is
 * left as-is for anyone who may want to do such conversion, which was
 * allowed in earlier algorithms.
 */
static const uint trailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

/*
 * Magic values subtracted from a buffer value during UTF8 conversion.
 * This table contains as many values as there might be trailing bytes
 * in a UTF-8 sequence.
 */
static const uint32 offsetsFromUTF8[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL,
		     0x03C82080UL, 0xFA082080UL, 0x82082080UL };

/*
 * Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
 * into the first byte, depending on how many bytes follow.  There are
 * as many entries in this table as there are UTF-8 sequence types.
 * (I.e., one byte sequence, two byte... etc.). Remember that sequencs
 * for *legal* UTF-8 will be 4 or fewer bytes total.
 */
static const uint firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };


/*
 * Utility routine to tell whether a sequence of bytes is legal UTF-8.
 * This must be called with the length pre-determined by the first byte.
 * If not calling this from ConvertUTF8to*, then the length can be set by:
 *  length = trailingBytesForUTF8[*source]+1;
 * and the sequence is illegal right away if there aren't that many bytes
 * available.
 * If presented with a length > 4, this returns false.  The Unicode
 * definition of UTF-8 goes up to 4-byte sequences.
 */

static bool isLegalUTF8(const uint8 *source, int length) {
		uint8 a;
    const uint8 *srcptr = source+length;
    switch (length) {
    default: return false;
	/* Everything else falls through when "true"... */
    case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 2: if ((a = (*--srcptr)) > 0xBF) return false;

	switch (*source) {
	    /* no fall-through in this inner switch */
	    case 0xE0: if (a < 0xA0) return false; break;
	    case 0xED: if (a > 0x9F) return false; break;
	    case 0xF0: if (a < 0x90) return false; break;
	    case 0xF4: if (a > 0x8F) return false; break;
	    default:   if (a < 0x80) return false;
	}

    case 1: if (*source >= 0x80 && *source < 0xC2) return false;
    }
    if (*source > 0xF4) return false;
    return true;
}

typedef enum {
	conversionOK, 		/* conversion successful */
	sourceExhausted,	/* partial character in source, but hit end */
	targetExhausted,	/* insuff. room in target for conversion */
	sourceIllegal,		/* source sequence is illegal/malformed */
	reachedNullChar
} ConversionResult;


typedef enum {
	strictConversion = 0,
	lenientConversion
} ConversionFlags;

static ConversionResult ConvertUTF8toUTF32 (
	const uint8** sourceStart, const uint8* sourceEnd,
	ConversionFlags flags, uint &c)
{
	ConversionResult result = conversionOK;
	const uint8* source = *sourceStart;
	// sourceEnd of nullptr is ignored
	if((sourceEnd && source < sourceEnd) || !sourceEnd)
	{
		if(*source == '\0')
			return reachedNullChar;
		uint ch = 0;
		auto extraBytesToRead = trailingBytesForUTF8[*source];
		if(sourceEnd && source + extraBytesToRead >= sourceEnd)
		{
			result = sourceExhausted; goto DONE;
		}
		/* Do this check whether lenient or strict */
		if (! isLegalUTF8(source, extraBytesToRead+1))
		{
			result = sourceIllegal;
			goto DONE;
		}

		// The cases all fall through. See "Note A" below.
		switch (extraBytesToRead)
		{
			case 5: ch += *source++; ch <<= 6;
			case 4: ch += *source++; ch <<= 6;
			case 3: ch += *source++; ch <<= 6;
			case 2: ch += *source++; ch <<= 6;
			case 1: ch += *source++; ch <<= 6;
			case 0: ch += *source++;
		}
		ch -= offsetsFromUTF8[extraBytesToRead];

		if (ch <= UNI_MAX_LEGAL_UTF32)
		{
			/*
			 * UTF-16 surrogate values are illegal in UTF-32, and anything
			 * over Plane 17 (> 0x10FFFF) is illegal.
			 */
			if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END)
			{
				if (flags == strictConversion)
				{
					source -= (extraBytesToRead+1); /* return to the illegal value itself */
					result = sourceIllegal;
					goto DONE;
				}
				else
				{
					c = UNI_REPLACEMENT_CHAR;
				}
			}
			else
			{
				c = ch;
			}
		}
		else
		{
			/* i.e., ch > UNI_MAX_LEGAL_UTF32 */
			result = sourceIllegal;
			c = UNI_REPLACEMENT_CHAR;
		}
	}
	else
	{
		result = sourceExhausted;
	}

	DONE:
	*sourceStart = source;
	return result;
}

// convert from a source without an end bound
static ConversionResult ConvertUTF8toUTF32 (
	const uint8** sourceStart,
	ConversionFlags flags, uint &c)
{
	return ConvertUTF8toUTF32(sourceStart, nullptr, flags, c);
}

}
