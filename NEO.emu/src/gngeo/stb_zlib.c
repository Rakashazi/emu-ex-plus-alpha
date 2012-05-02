// public domain zlib decode    v0.2  Sean Barrett 2006-11-18
//    simple implementation
//      - all input must be provided in an upfront buffer
//      - all output is written to a single output buffer (can malloc/realloc)
//    performance
//      - fast huffman
// implementation:

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if !defined(HAVE_LIBZ) || !defined(HAVE_MMAP)

#include "stb_zlib.h"
#ifndef STBI_NO_STDIO
#include <stdio.h>
#endif
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <stdarg.h>

#ifndef _MSC_VER
  #ifdef __cplusplus
  #define __forceinline inline
  #else
  #define __forceinline
  #endif
#endif


// should produce compiler error if size is wrong
typedef unsigned char validate_uint32[sizeof(uint32)==4];
// this is not threadsafe
/*
static char *failure_reason;

char *stbi_failure_reason(void)
{
   return failure_reason;
}
*/
static int e(char *str)
{
	//failure_reason = str;
	printf("GNZLIB Failure %s\n",str);
	exit(1);
	return 0;
}
#ifdef STBI_NO_FAILURE_STRINGS
   #define e(x,y)  0
#elif defined(STBI_FAILURE_USERMSG)
   #define e(x,y)  e(y)
#else
   #define e(x,y)  e(x)
#endif

#define epf(x,y)   ((float *) (e(x,y)?NULL:NULL))
#define epuc(x,y)  ((unsigned char *) (e(x,y)?NULL:NULL))



__forceinline static int bitreverse16(int n)
{
  n = ((n & 0xAAAA) >>  1) | ((n & 0x5555) << 1);
  n = ((n & 0xCCCC) >>  2) | ((n & 0x3333) << 2);
  n = ((n & 0xF0F0) >>  4) | ((n & 0x0F0F) << 4);
  n = ((n & 0xFF00) >>  8) | ((n & 0x00FF) << 8);
  return n;
}

__forceinline static int bit_reverse(int v, int bits)
{
   assert(bits <= 16);
   // to bit reverse n bits, reverse 16 and shift
   // e.g. 11 bits, bit reverse and shift away 5
   return bitreverse16(v) >> (16-bits);
}

static int zbuild_huffman(zhuffman *z, uint8 *sizelist, int num)
{
   int i,k=0;
   int code, next_code[16], sizes[17];

   // DEFLATE spec for generating codes
   memset(sizes, 0, sizeof(sizes));
   memset(z->fast, 255, sizeof(z->fast));
   for (i=0; i < num; ++i) 
      ++sizes[sizelist[i]];
   sizes[0] = 0;
   for (i=1; i < 16; ++i)
      assert(sizes[i] <= (1 << i));
   code = 0;
   for (i=1; i < 16; ++i) {
      next_code[i] = code;
      z->firstcode[i] = (uint16) code;
      z->firstsymbol[i] = (uint16) k;
      code = (code + sizes[i]);
      if (sizes[i])
         if (code-1 >= (1 << i)) return e("bad codelengths","Corrupt JPEG");
      z->maxcode[i] = code << (16-i); // preshift for inner loop
      code <<= 1;
      k += sizes[i];
   }
   z->maxcode[16] = 0x10000; // sentinel
   for (i=0; i < num; ++i) {
      int s = sizelist[i];
      if (s) {
         int c = next_code[s] - z->firstcode[s] + z->firstsymbol[s];
         z->size[c] = (uint8)s;
         z->value[c] = (uint16)i;
         if (s <= ZFAST_BITS) {
            int k = bit_reverse(next_code[s],s);
            while (k < (1 << ZFAST_BITS)) {
               z->fast[k] = (uint16) c;
               k += (1 << s);
            }
         }
         ++next_code[s];
      }
   }
   return 1;
}

// zlib-from-memory implementation for PNG reading
//    because PNG allows splitting the zlib stream arbitrarily,
//    and it's annoying structurally to have PNG call ZLIB call PNG,
//    we require PNG read all the IDATs and combine them into a single
//    memory buffer


__forceinline static int zget8(zbuf *z)
{
	uint8 a;
	if (z->totread >= z->totsize) return 0;
	if (z->zbuffer) {
		z->totread++;
		return *z->zbuffer++;
	}

	fread(&a,1,1,z->zf);
	z->totread++;
	return a;
}

static void fill_bits(zbuf *z)
{
	//printf("Fillbits\n");
   do {
      assert(z->code_buffer < (1U << z->num_bits));
      z->code_buffer |= zget8(z) << z->num_bits;
      z->num_bits += 8;
   } while (z->num_bits <= 24);
}

__forceinline static unsigned int zreceive(zbuf *z, int n)
{
   unsigned int k;
   if (z->num_bits < n) fill_bits(z);
   k = z->code_buffer & ((1 << n) - 1);
   z->code_buffer >>= n;
   z->num_bits -= n;
   return k;   
}

__forceinline static int zhuffman_decode(zbuf *a, zhuffman *z)
{
   int b,s,k;
   if (a->num_bits < 16) fill_bits(a);
   b = z->fast[a->code_buffer & ZFAST_MASK];
   if (b < 0xffff) {
      s = z->size[b];
      a->code_buffer >>= s;
      a->num_bits -= s;
      return z->value[b];
   }

   // not resolved by fast table, so compute it the slow way
   // use jpeg approach, which requires MSbits at top
   k = bit_reverse(a->code_buffer, 16);
   for (s=ZFAST_BITS+1; ; ++s)
      if (k < z->maxcode[s])
         break;
   if (s == 16) return -1; // invalid code!
   // code size is s, so:
   b = (k >> (16-s)) - z->firstcode[s] + z->firstsymbol[s];
   assert(z->size[b] == s);
   a->code_buffer >>= s;
   a->num_bits -= s;
   return z->value[b];
}

static int length_base[31] = {
   3,4,5,6,7,8,9,10,11,13,
   15,17,19,23,27,31,35,43,51,59,
   67,83,99,115,131,163,195,227,258,0,0 };

static int length_extra[31]= 
{ 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0,0 };

static int dist_base[32] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0};

static int dist_extra[32] =
{ 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

static int compute_huffman_codes(zbuf *a)
{
   static uint8 length_dezigzag[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
   zhuffman z_codelength;
   uint8 lencodes[286+32+137];//padding for maximum single op
   uint8 codelength_sizes[19];
   int i,n;

   int hlit  = zreceive(a,5) + 257;
   int hdist = zreceive(a,5) + 1;
   int hclen = zreceive(a,4) + 4;

   memset(codelength_sizes, 0, sizeof(codelength_sizes));
   for (i=0; i < hclen; ++i) {
      int s = zreceive(a,3);
      codelength_sizes[length_dezigzag[i]] = (uint8) s;
   }
   if (!zbuild_huffman(&z_codelength, codelength_sizes, 19)) return 0;

   n = 0;
   while (n < hlit + hdist) {
      int c = zhuffman_decode(a, &z_codelength);
      assert(c >= 0 && c < 19);
      if (c < 16)
         lencodes[n++] = (uint8) c;
      else if (c == 16) {
         c = zreceive(a,2)+3;
         memset(lencodes+n, lencodes[n-1], c);
         n += c;
      } else if (c == 17) {
         c = zreceive(a,3)+3;
         memset(lencodes+n, 0, c);
         n += c;
      } else {
         assert(c == 18);
         c = zreceive(a,7)+11;
         memset(lencodes+n, 0, c);
         n += c;
      }
   }
   if (n != hlit+hdist) return e("bad codelengths","Corrupt PNG");
   if (!zbuild_huffman(&a->z_length, lencodes, hlit)) return 0;
   if (!zbuild_huffman(&a->z_distance, lencodes+hlit, hdist)) return 0;
   return 1;
}

// @TODO: should statically initialize these for optimal thread safety
static uint8 default_length[288], default_distance[32];
static void init_defaults(void)
{
   int i;   // use <= to match clearly with spec
   //printf("Init defaults length\n");
   for (i=0; i <= 143; ++i)     default_length[i]   = 8;
   for (   ; i <= 255; ++i)     default_length[i]   = 9;
   for (   ; i <= 279; ++i)     default_length[i]   = 7;
   for (   ; i <= 287; ++i)     default_length[i]   = 8;

   for (i=0; i <=  31; ++i)     default_distance[i] = 5;
}

/* Circular buffer routine */
static inline void push_cbuf(zbuf *a,uint8 data) {
	//printf("Push cbuf %d %d\n",a->cb_pos,data);
	a->cbuf[a->cb_pos++]=data;
	if (a->cb_pos>=32*1024) a->cb_pos=0;
}
static inline uint8 pop_cbuf(zbuf *a) {
	uint8 data=a->cbuf[a->cb_pos++];
	if (a->cb_pos>=32*1024) a->cb_pos=0;
	return data;
}
static inline uint8 pop_cbuf_from_dist(zbuf *a,int dist) {
	int npos=a->cb_pos-dist;
	//uint8 data;

	if (npos<0) npos+=(32*1024);
	//printf("Pop cbuf %d\n",npos);
	return a->cbuf[npos];
	//if (a->cb_pos>=32*1024) a->cb_pos=0;
}

static int parse_zlib_header(zbuf *a)
{
   int cmf   = zget8(a);
   int cm    = cmf & 15;
   /* int cinfo = cmf >> 4; */
   int flg   = zget8(a);
   if ((cmf*256+flg) % 31 != 0) return e("bad zlib header","Corrupt PNG"); // zlib spec
   if (flg & 32) return e("no preset dict","Corrupt PNG"); // preset dictionary not allowed in png
   if (cm != 8) return e("bad compression","Corrupt PNG"); // DEFLATE required for png
   // window = 1 << (8 + cinfo)... but who cares, we fully buffer output

   return 1;
}

static int parse_huffman_block(zbuf *a,int maxlen)
{
	int totdec=0;//a->zout-a->zout_start;
	//printf("TODEC=%d %d\n",totdec,maxlen);
	if (a->left>0) {
		//printf("a->left=%d a->dist=%d\n",a->left,a->dist);
		while (a->left--) {
			uint8 o=pop_cbuf_from_dist(a,a->dist);
			push_cbuf(a,o);
			*a->zout++ = o;
			totdec++;
						
			if (totdec==maxlen) {
				//printf("  Dyn block max read reach %d %d %d\n",a->type,a->left,a->dist);
				return maxlen;
			}
		}
	}
	for(;;) {
		int z = zhuffman_decode(a, &a->z_length);
		//printf("Z=%d\n",z);
		if (z < 256) {
			if (z < 0) return e("bad huffman code","Corrupt PNG"); // error in huffman codes
			//printf("z=%d %d %d\n",z,totdec,a->zout-a->zout_start);
			*a->zout++ = (char) z;

			push_cbuf(a,z);
			totdec++;
			if (totdec==maxlen) {
				//printf("  Dyn block max read reach %d %d\n",a->type,a->left);
				return maxlen;
			}
		} else {
			uint8 *p;
			int len,dist;
			if (z == 256) {
				//printf("End of huffman block\n");
				a->type=-1; /* New block ahead */
				return totdec;
			}
			if (a->left<=0) {
				//printf("Start of dict copy %d\n",a->left);
				z -= 257;
				len = length_base[z];
				if (length_extra[z]) len += zreceive(a, length_extra[z]);
				z = zhuffman_decode(a, &a->z_distance);
				if (z < 0) return e("bad huffman code","Corrupt ZIP");
				dist = dist_base[z];
				if (dist_extra[z]) dist += zreceive(a, dist_extra[z]);
				//if (a->zout - a->zout_start < dist) return e("bad dist","Corrupt PNG");
				if (dist>32*1024) {printf("Bad dist\n");return 0;}
				//p = (uint8 *) (a->zout - dist);
				a->left=len;
				a->dist=dist;
				//printf("End of Start of dict copy %d\n",a->left);
			}
			//printf("a->left=%d\n",a->left);
			while (a->left--) {
				uint8 o=pop_cbuf_from_dist(a,a->dist);
				push_cbuf(a,o);
				*a->zout++ = o;
				totdec++;

				if (totdec==maxlen) {
					//printf("  Dyn block max read reach %d %d %d\n",a->type,a->left,a->dist);
					return maxlen;
				}
			}
		}
	}
}

static int copy_uncompressed_block(zbuf *a,int maxlen)
{
	int len=(a->left>maxlen?maxlen:a->left);
	int i=len;
	uint8 data;
	//printf("BBBBBBBBBBBBb\n");
	while(i) {
		if (a->zbuffer) { /* Stream from memory */
			push_cbuf(a,*a->zbuffer);
			*a->zout++ = *a->zbuffer++;
		} else {  /* Stream from file */
			fread(&data,1,1,a->zf);
			push_cbuf(a,data);
			*a->zout++ = data;
		}
		i--;
		a->totread++;
	}
	a->left-=len;if (a->left<=0) a->type=-1;
	return len;
}

static int parse_header(zbuf *a) {
	uint8 header[4];
	int len,nlen,k;
	switch(a->type) {
	case 0: 
		//printf("CCCCCCCCCCCcccccc\n");
/* Read uncompressed block header */
		if (a->num_bits & 7)
			zreceive(a, a->num_bits & 7); // discard
		// drain the bit-packed data into header
		k = 0;
		while (a->num_bits > 0) {
			header[k++] = (uint8) (a->code_buffer & 255); // wtf this warns?
			a->code_buffer >>= 8;
			a->num_bits -= 8;
		}
		assert(a->num_bits == 0);
		// now fill header the normal way
		while (k < 4)
			header[k++] = (uint8) zget8(a);
		len  = header[1] * 256 + header[0];
		nlen = header[3] * 256 + header[2];
		printf("len %d nlen %d\n",len,nlen);
		if (nlen != (len ^ 0xffff)) return e("zlib corrupt (uncompressed block)","Corrupt ZIP");
		if (a->totread + len > a->totsize) return e("read past buffer","Corrupt ZIP");
		a->left=len;
		break;
	case 1:
		// use fixed code lengths

		if (!default_distance[31]) init_defaults();
		if (!zbuild_huffman(&a->z_length  , default_length  , 288)) return 0;
		if (!zbuild_huffman(&a->z_distance, default_distance,  32)) return 0;
		//printf("Fixed code length\n");
		
		break;
	case 2:
		// Dynamic huffman code lenght
		if (!compute_huffman_codes(a)) return 0;
		break;
	case 3:
		/* erreur */
		return 0;
	}
}

/* Create a zbuf struct, suitable to decode a zlib stream 
   The stream come from ibuffer if not null, f otherwise.
   ilen is the size of the stream 
   return a zbuf struct, NULL on error
*/
zbuf *stbi_zlib_create_zbuf(const char *ibuffer,FILE *f,int ilen) {
	zbuf *a=malloc(sizeof(zbuf));

	/* Circular buf init */
	a->cbuf=malloc(32*1024);
	a->cb_pos=0;

	/* Block def */
	a->final=0;
	a->type=-1;

	/* How many left since last call */
	a->left=0;
	
	if (!ibuffer && !f) return NULL;
	a->zbuffer = (uint8 *) ibuffer;
	a->zf=f;
	a->zbuffer_end = (uint8 *) ibuffer + ilen;
	a->totread=0;
	a->totsize=ilen;

	a->num_bits = 0;
	a->code_buffer = 0;

	return a;
}

/* Decode max olen byte from the stream pointed by a
   return how many byte have been readed, -1 on error or end of stream;
 */
int stbi_zlib_decode_noheader_stream(zbuf *a,char *obuffer, int olen) {
	int readed=0;
	int totread=0;
	int l;
	if(!a) return -1;
	//printf("TOTRED %d TOTSIZE %d %d\n",a->totread,a->totsize,a->num_bits);
	if (a->totread>=a->totsize) return -1;

	a->zout_start=obuffer;
	a->zout=obuffer;
	int todo=olen;
	do {
		if (a->type==-1) { /* start */
			a->final = zreceive(a,1);
			a->type = zreceive(a,2);
			//printf("Begining of a block type %d\n",a->type);
			parse_header(a);
		}
	
		switch(a->type) {
		case 0: /* Uncompressed block */
			readed=copy_uncompressed_block(a,todo);
			totread+=readed;
			
			break;
		case 1: /* Huffman block (fixed) */
		case 2: /* Huffman block (dyn, calculated in parse_header() ) */
			//printf(" .Dyn huffman block\n");
			readed=parse_huffman_block(a,todo);
			totread+=readed;
			break;
		case 3:
			/* error */
			break;
		}

		todo-=readed;
		//printf(".--End one pass readed=%d totread=%d todo=%d type=%d\n",readed,totread,todo,a->type);
	} while(!a->final && totread<olen);
	//printf(". End all pass readed=%d olen=%d type=%d\n",readed,olen,a->type);
	return totread;
}

char *stbi_zlib_decode_malloc(char const *buffer, int len, int *outlen) {
	zbuf *z=stbi_zlib_create_zbuf(buffer,NULL,len);
	int readed,totread=0;
	char *buf;
	int guesssize=len*20;

	printf("stbi_zlib_decode_malloc %p %d\n",buffer,guesssize);
	buf=malloc(guesssize);
	if (!buf || !z) return NULL;

	if (!parse_zlib_header(z)) return NULL;

	while((readed=stbi_zlib_decode_noheader_stream(z,buf,guesssize))!=-1) {
		//readed=stbi_zlib_decode_noheader_stream(z,buf,guesssize);
		//printf("1block %d %d z->num_bits %d\n",totread,readed,z->num_bits);
		totread+=readed;
		buf=realloc(buf,guesssize+totread);
	}
	//printf("Readed %d \n",totread);
	buf=realloc(buf,totread);
	*outlen=totread;
	return buf;
}

#endif
