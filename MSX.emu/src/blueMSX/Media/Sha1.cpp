// $Id: Sha1.cpp,v 1.3 2007-05-22 06:23:17 dvik Exp $

/*
Based on:
100% free public domain implementation of the SHA-1 algorithm
by Dominik Reichl <Dominik.Reichl@tiscali.de>

Refactored in C++ style as part of openMSX
by Maarten ter Huurne and Wouter Vermaelen.

=== Test Vectors (from FIPS PUB 180-1) ===

"abc"
A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D

"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
84983E44 1C3BD26E BAAE4AA1 F95129E5 E54670F1

A million repetitions of "a"
34AA973C D4C4DAA4 F61EEB2B DBAD2731 6534016F
*/

#include <cassert>
#include <cstdio>
#include <string.h>
#include "Sha1.h"

using std::string;

// Rotate x bits to the left
inline static UInt32 rol32(UInt32 value, int bits)
{
	return (value << bits) | (value >> (32 - bits));
}

class WorkspaceBlock {
private:
	UInt32 data[16];

	UInt32 next0(int i)
	{
#ifdef __BIG_ENDIAN__
        return data[i];
#else
		return data[i] = (rol32(data[i], 24) & 0xFF00FF00)
			            | (rol32(data[i],  8) & 0x00FF00FF);
#endif
	}
	UInt32 next(int i)
	{
		return data[i & 15] = rol32(
			data[(i + 13) & 15] ^ data[(i + 8) & 15] ^
			data[(i +  2) & 15] ^ data[ i      & 15]
			, 1);
	}

public:
	WorkspaceBlock(const UInt8 buffer[64]);

	// SHA-1 rounds
	void r0(UInt32 v, UInt32& w, UInt32 x, UInt32 y, UInt32& z, int i)
	{
		z += ((w & (x ^ y)) ^ y) + next0(i) + 0x5A827999 + rol32(v, 5);
		w = rol32(w, 30);
	}
	void r1(UInt32 v, UInt32& w, UInt32 x, UInt32 y, UInt32& z, int i)
	{
		z += ((w & (x ^ y)) ^ y) + next(i) + 0x5A827999 + rol32(v, 5);
		w = rol32(w, 30);
	}
	void r2(UInt32 v, UInt32& w, UInt32 x, UInt32 y, UInt32& z, int i)
	{
		z += (w ^ x ^ y) + next(i) + 0x6ED9EBA1 + rol32(v, 5);
		w = rol32(w, 30);
	}
	void r3(UInt32 v, UInt32& w, UInt32 x, UInt32 y, UInt32& z, int i)
	{
		z += (((w | x) & y) | (w & x)) + next(i) + 0x8F1BBCDC + rol32(v, 5);
		w = rol32(w, 30);
	}
	void r4(UInt32 v, UInt32& w, UInt32 x, UInt32 y, UInt32& z, int i)
	{
		z += (w ^ x ^ y) + next(i) + 0xCA62C1D6 + rol32(v, 5);
		w = rol32(w, 30);
	}
};

WorkspaceBlock::WorkspaceBlock(const UInt8 buffer[64])
{
	memcpy(data, buffer, 64);
}


SHA1::SHA1()
{
	// SHA1 initialization constants
	m_state[0] = 0x67452301;
	m_state[1] = 0xEFCDAB89;
	m_state[2] = 0x98BADCFE;
	m_state[3] = 0x10325476;
	m_state[4] = 0xC3D2E1F0;

	m_count = 0;
}

SHA1::~SHA1()
{
}

void SHA1::transform(const UInt8 buffer[64])
{
	WorkspaceBlock block(buffer);

	// Copy m_state[] to working vars
	UInt32 a = m_state[0];
	UInt32 b = m_state[1];
	UInt32 c = m_state[2];
	UInt32 d = m_state[3];
	UInt32 e = m_state[4];

	// 4 rounds of 20 operations each. Loop unrolled
	block.r0(a,b,c,d,e, 0); block.r0(e,a,b,c,d, 1); block.r0(d,e,a,b,c, 2);
	block.r0(c,d,e,a,b, 3); block.r0(b,c,d,e,a, 4); block.r0(a,b,c,d,e, 5);
	block.r0(e,a,b,c,d, 6); block.r0(d,e,a,b,c, 7); block.r0(c,d,e,a,b, 8);
	block.r0(b,c,d,e,a, 9); block.r0(a,b,c,d,e,10); block.r0(e,a,b,c,d,11); 
	block.r0(d,e,a,b,c,12); block.r0(c,d,e,a,b,13); block.r0(b,c,d,e,a,14);
	block.r0(a,b,c,d,e,15); block.r1(e,a,b,c,d,16); block.r1(d,e,a,b,c,17);
	block.r1(c,d,e,a,b,18); block.r1(b,c,d,e,a,19); block.r2(a,b,c,d,e,20);
	block.r2(e,a,b,c,d,21); block.r2(d,e,a,b,c,22); block.r2(c,d,e,a,b,23);
	block.r2(b,c,d,e,a,24); block.r2(a,b,c,d,e,25); block.r2(e,a,b,c,d,26);
	block.r2(d,e,a,b,c,27); block.r2(c,d,e,a,b,28); block.r2(b,c,d,e,a,29);
	block.r2(a,b,c,d,e,30); block.r2(e,a,b,c,d,31); block.r2(d,e,a,b,c,32);
	block.r2(c,d,e,a,b,33); block.r2(b,c,d,e,a,34); block.r2(a,b,c,d,e,35);
	block.r2(e,a,b,c,d,36); block.r2(d,e,a,b,c,37); block.r2(c,d,e,a,b,38);
	block.r2(b,c,d,e,a,39); block.r3(a,b,c,d,e,40); block.r3(e,a,b,c,d,41);
	block.r3(d,e,a,b,c,42); block.r3(c,d,e,a,b,43); block.r3(b,c,d,e,a,44);
	block.r3(a,b,c,d,e,45); block.r3(e,a,b,c,d,46); block.r3(d,e,a,b,c,47);
	block.r3(c,d,e,a,b,48); block.r3(b,c,d,e,a,49); block.r3(a,b,c,d,e,50);
	block.r3(e,a,b,c,d,51); block.r3(d,e,a,b,c,52); block.r3(c,d,e,a,b,53);
	block.r3(b,c,d,e,a,54); block.r3(a,b,c,d,e,55); block.r3(e,a,b,c,d,56);
	block.r3(d,e,a,b,c,57); block.r3(c,d,e,a,b,58); block.r3(b,c,d,e,a,59);
	block.r4(a,b,c,d,e,60); block.r4(e,a,b,c,d,61); block.r4(d,e,a,b,c,62);
	block.r4(c,d,e,a,b,63); block.r4(b,c,d,e,a,64); block.r4(a,b,c,d,e,65);
	block.r4(e,a,b,c,d,66); block.r4(d,e,a,b,c,67); block.r4(c,d,e,a,b,68);
	block.r4(b,c,d,e,a,69); block.r4(a,b,c,d,e,70); block.r4(e,a,b,c,d,71);
	block.r4(d,e,a,b,c,72); block.r4(c,d,e,a,b,73); block.r4(b,c,d,e,a,74);
	block.r4(a,b,c,d,e,75); block.r4(e,a,b,c,d,76); block.r4(d,e,a,b,c,77);
	block.r4(c,d,e,a,b,78); block.r4(b,c,d,e,a,79);
	
	// Add the working vars back into m_state[]
	m_state[0] += a;
	m_state[1] += b;
	m_state[2] += c;
	m_state[3] += d;
	m_state[4] += e;
}

// Use this function to hash in binary data and strings
void SHA1::update(const UInt8* data, unsigned len)
{
	assert(digest.empty());
	UInt32 j = (UInt32)((m_count >> 3) & 63);

	m_count += len << 3;

	UInt32 i;
	if ((j + len) > 63) {
		memcpy(&m_buffer[j], data, (i = 64 - j));
		transform(m_buffer);
		for (; i + 63 < len; i += 64) {
			transform(&data[i]);
		}
		j = 0;
	} else {
		i = 0;
	}
	memcpy(&m_buffer[j], &data[i], len - i);
}

void SHA1::finalize()
{
	UInt8 finalcount[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	for (int i = 0; i < 8; i++) {
		finalcount[i] = static_cast<UInt8>(m_count >> ((7 - i) * 8));
	}

	update((const UInt8*)"\200", 1);
	while ((m_count & 504) != 448) {
		update((const UInt8*)"\0", 1);
	}
	update(finalcount, 8); // cause a transform()

	char s[41];
	for (int i = 0; i < 20; ++i) {
		sprintf(s + i * 2, "%02x", static_cast<UInt8>(
			m_state[i >> 2] >> ((3 - (i & 3)) * 8)));
	}
	digest = string(s, 40);
}

const string& SHA1::hex_digest()
{
	if (digest.empty()) {
		finalize();
	}
	assert(!digest.empty());
	return digest;
}
