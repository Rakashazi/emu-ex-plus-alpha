#ifndef STB_ZLIB_H
#define STB_ZLIB_H

// ZLIB client - used by PNG, available for other purposes
#include <stdio.h>

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef   signed short  int16;
typedef unsigned int   uint32;
typedef   signed int    int32;
typedef unsigned int   uint;


// fast-way is faster to check than jpeg huffman, but slow way is slower
#define ZFAST_BITS  9 // accelerate all cases in default tables
#define ZFAST_MASK  ((1 << ZFAST_BITS) - 1)

// zlib-style huffman encoding
// (jpegs packs from left, zlib from right, so can't share code)
typedef struct
{
   uint16 fast[1 << ZFAST_BITS];
   uint16 firstcode[16];
   int maxcode[17];
   uint16 firstsymbol[16];
   uint8  size[288];
   uint16 value[288]; 
} zhuffman;

typedef struct
{
	uint8 *zbuffer, *zbuffer_end;
	FILE *zf;
	int totread,totsize;
	int num_bits;
	uint32 code_buffer;
	
	char *zout;
	char *zout_start;
	char *zout_end;
	int   z_expandable;
	
	uint8 *cbuf;
	uint32 cb_pos;
	int final,left,type,dist;

	zhuffman z_length, z_distance;
} zbuf;




char *stbi_zlib_decode_malloc(const char *buffer, int len, int *outlen);

zbuf *stbi_zlib_create_zbuf(const char *ibuffer,FILE *f, int ilen);
int   stbi_zlib_decode_noheader_stream(zbuf *a,char *obuffer, int olen);



#endif
