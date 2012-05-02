/* Tile drawing template
   use RENAME to set the name of the function
   use PUTPIXEL(dest,src) to set the putpixel function/macro
*/


static __inline__ void RENAME(draw)(unsigned int tileno,int sx,int sy,int zx,int zy,
					 int color,int xflip,int yflip,unsigned char *bmp)
{
    unsigned int *gfxdata,myword;
    int y;
    unsigned char col;
    unsigned short *br;
    unsigned int *paldata=(unsigned int *)&current_pc_pal[16*color];
    char *l_y_skip;
    int l; // Line skipping counter
#ifdef DEBUG_VIDEO
    int buf_w=544-zx;
    int buf_w_yflip=544+zx;
#else
    int buf_w=(buffer->pitch>>1)-zx;
    int buf_w_yflip=(buffer->pitch>>1)+zx;
#endif
    tileno=tileno%memory.nb_of_tiles;
   
    gfxdata = (unsigned int *)&memory.rom.tiles.p[ tileno<<7];

    /* y zoom table */
    if(zy==16)
        l_y_skip=full_y_skip;
    else
        l_y_skip=dda_y_skip;

    if (zx==16) {
        if (xflip) {
            l=0;
            if (yflip) {
#ifdef DEBUG_VIDEO
                br= (unsigned short *)bmp+((zy-1)+sy)*544+sx;
#else
                br= (unsigned short *)bmp+((zy-1)+sy)*(buffer->pitch>>1)+sx;
#endif
                for(y=0;y<zy;y++) {
                    gfxdata+=l_y_skip[l]<<1;
		    if (gfxdata[1] || gfxdata[0]) {
			    myword = gfxdata[1];
			    col=(myword>>0)&0xf; if (col) PUTPIXEL(br[0],paldata[col]);
			    col=(myword>>4)&0xf; if (col) PUTPIXEL(br[1],paldata[col]);
			    col=(myword>>8)&0xf; if (col) PUTPIXEL(br[2],paldata[col]);
			    col=(myword>>12)&0xf; if (col) PUTPIXEL(br[3],paldata[col]);
			    col=(myword>>16)&0xf; if (col) PUTPIXEL(br[4],paldata[col]);
			    col=(myword>>20)&0xf; if (col) PUTPIXEL(br[5],paldata[col]);
			    col=(myword>>24)&0xf; if (col) PUTPIXEL(br[6],paldata[col]);
			    col=(myword>>28)&0xf; if (col) PUTPIXEL(br[7],paldata[col]);
			    myword = gfxdata[0];
			    col=(myword>>0)&0xf; if (col) PUTPIXEL(br[8],paldata[col]);
			    col=(myword>>4)&0xf; if (col) PUTPIXEL(br[9],paldata[col]);
			    col=(myword>>8)&0xf; if (col) PUTPIXEL(br[10],paldata[col]);
			    col=(myword>>12)&0xf; if (col) PUTPIXEL(br[11],paldata[col]);
			    col=(myword>>16)&0xf; if (col) PUTPIXEL(br[12],paldata[col]);
			    col=(myword>>20)&0xf; if (col) PUTPIXEL(br[13],paldata[col]);
			    col=(myword>>24)&0xf; if (col) PUTPIXEL(br[14],paldata[col]);
			    col=(myword>>28)&0xf; if (col) PUTPIXEL(br[15],paldata[col]);
		    }
#ifdef DEBUG_VIDEO
                    br-=544;
#else
                    br-=(buffer->pitch>>1);
#endif
                    l++;
                }
            } else {
#ifdef DEBUG_VIDEO
                br= (unsigned short *)bmp+(sy)*544+sx;
#else
                br= (unsigned short *)bmp+(sy)*(buffer->pitch>>1)+sx;
#endif
                for(y=0;y<zy;y++) {
                    
                    gfxdata+=l_y_skip[l]<<1;
		    if (gfxdata[1] || gfxdata[0]) {
                    myword = gfxdata[1];
                    col=(myword>>0)&0xf; if (col) PUTPIXEL(br[0],paldata[col]);
                    col=(myword>>4)&0xf; if (col) PUTPIXEL(br[1],paldata[col]);
                    col=(myword>>8)&0xf; if (col) PUTPIXEL(br[2],paldata[col]);
                    col=(myword>>12)&0xf; if (col) PUTPIXEL(br[3],paldata[col]);
                    col=(myword>>16)&0xf; if (col) PUTPIXEL(br[4],paldata[col]);
                    col=(myword>>20)&0xf; if (col) PUTPIXEL(br[5],paldata[col]);
                    col=(myword>>24)&0xf; if (col) PUTPIXEL(br[6],paldata[col]);
                    col=(myword>>28)&0xf; if (col) PUTPIXEL(br[7],paldata[col]);
                    myword = gfxdata[0];
                    col=(myword>>0)&0xf; if (col) PUTPIXEL(br[8],paldata[col]);
                    col=(myword>>4)&0xf; if (col) PUTPIXEL(br[9],paldata[col]);
                    col=(myword>>8)&0xf; if (col) PUTPIXEL(br[10],paldata[col]);
                    col=(myword>>12)&0xf; if (col) PUTPIXEL(br[11],paldata[col]);
                    col=(myword>>16)&0xf; if (col) PUTPIXEL(br[12],paldata[col]);
                    col=(myword>>20)&0xf; if (col) PUTPIXEL(br[13],paldata[col]);
                    col=(myword>>24)&0xf; if (col) PUTPIXEL(br[14],paldata[col]);
                    col=(myword>>28)&0xf; if (col) PUTPIXEL(br[15],paldata[col]);
		    }
#ifdef DEBUG_VIDEO
                    br+=544;
#else
                    br+=(buffer->pitch>>1);
#endif
                    l++;
		
                }
            }
        }else {
            l=0;
            if (yflip) {
#ifdef DEBUG_VIDEO
                br= (unsigned short *)bmp+((zy-1)+sy)*544+sx;
#else
                br= (unsigned short *)bmp+((zy-1)+sy)*(buffer->pitch>>1)+sx;
#endif
                for(y=0;y<zy;y++) {
                    gfxdata+=l_y_skip[l]<<1;
		    if (gfxdata[1] || gfxdata[0]) {
                    myword = gfxdata[0];
                    col=(myword>>28)&0xf; if (col) PUTPIXEL(br[0],paldata[col]);
                    col=(myword>>24)&0xf; if (col) PUTPIXEL(br[1],paldata[col]);
                    col=(myword>>20)&0xf; if (col) PUTPIXEL(br[2],paldata[col]);
                    col=(myword>>16)&0xf; if (col) PUTPIXEL(br[3],paldata[col]);
                    col=(myword>>12)&0xf; if (col) PUTPIXEL(br[4],paldata[col]);
                    col=(myword>>8)&0xf; if (col) PUTPIXEL(br[5],paldata[col]);
                    col=(myword>>4)&0xf; if (col) PUTPIXEL(br[6],paldata[col]);
                    col=(myword>>0)&0xf; if (col) PUTPIXEL(br[7],paldata[col]);
	      
                    myword = gfxdata[1];
                    col=(myword>>28)&0xf; if (col) PUTPIXEL(br[8],paldata[col]);
                    col=(myword>>24)&0xf; if (col) PUTPIXEL(br[9],paldata[col]);
                    col=(myword>>20)&0xf; if (col) PUTPIXEL(br[10],paldata[col]);
                    col=(myword>>16)&0xf; if (col) PUTPIXEL(br[11],paldata[col]);
                    col=(myword>>12)&0xf; if (col) PUTPIXEL(br[12],paldata[col]);
                    col=(myword>>8)&0xf; if (col) PUTPIXEL(br[13],paldata[col]);
                    col=(myword>>4)&0xf; if (col) PUTPIXEL(br[14],paldata[col]);
                    col=(myword>>0)&0xf; if (col) PUTPIXEL(br[15],paldata[col]);
		    }
                    l++;
#ifdef DEBUG_VIDEO
                    br-=544;
#else
                    br-=(buffer->pitch>>1);
#endif
                }
            } else {
#ifdef DEBUG_VIDEO
                br= (unsigned short *)bmp+(sy)*544+sx;
#else
                br= (unsigned short *)bmp+(sy)*(buffer->pitch>>1)+sx;
#endif
                for(y=0;y<zy;y++) {
                    gfxdata+=l_y_skip[l]<<1;
		    if (gfxdata[1] || gfxdata[0]) {
                    myword = gfxdata[0];
                    col=(myword>>28)&0xf; if (col) PUTPIXEL(br[0],paldata[col]);
                    col=(myword>>24)&0xf; if (col) PUTPIXEL(br[1],paldata[col]);
                    col=(myword>>20)&0xf; if (col) PUTPIXEL(br[2],paldata[col]);
                    col=(myword>>16)&0xf; if (col) PUTPIXEL(br[3],paldata[col]);
                    col=(myword>>12)&0xf; if (col) PUTPIXEL(br[4],paldata[col]);
                    col=(myword>>8)&0xf; if (col) PUTPIXEL(br[5],paldata[col]);
                    col=(myword>>4)&0xf; if (col) PUTPIXEL(br[6],paldata[col]);
                    col=(myword>>0)&0xf; if (col) PUTPIXEL(br[7],paldata[col]);
	      
                    myword = gfxdata[1];
                    col=(myword>>28)&0xf; if (col) PUTPIXEL(br[8],paldata[col]);
                    col=(myword>>24)&0xf; if (col) PUTPIXEL(br[9],paldata[col]);
                    col=(myword>>20)&0xf; if (col) PUTPIXEL(br[10],paldata[col]);
                    col=(myword>>16)&0xf; if (col) PUTPIXEL(br[11],paldata[col]);
                    col=(myword>>12)&0xf; if (col) PUTPIXEL(br[12],paldata[col]);
                    col=(myword>>8)&0xf; if (col) PUTPIXEL(br[13],paldata[col]);
                    col=(myword>>4)&0xf; if (col) PUTPIXEL(br[14],paldata[col]);
                    col=(myword>>0)&0xf; if (col) PUTPIXEL(br[15],paldata[col]);
		    }
                    l++;
#ifdef DEBUG_VIDEO
                    br+=544;
#else
                    br+=(buffer->pitch>>1);
#endif
                }
            }
        }
    }else { // zx!=16
        if (xflip) {
            l=0;
            if (yflip) {
#ifdef DEBUG_VIDEO
                br= (unsigned short *)bmp+((zy-1)+sy)*544+sx;
#else
                br= (unsigned short *)bmp+((zy-1)+sy)*(buffer->pitch>>1)+sx;
#endif
                for(y=0;y<zy;y++) {
                    gfxdata+=l_y_skip[l]<<1;
                    myword = gfxdata[1];
                    if (dda_x_skip[ 0]) {if ((col=((myword>>0)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[ 1]) {if  ((col=((myword>>4)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[ 2]) {if  ((col=((myword>>8)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[ 3]) {if  ((col=((myword>>12)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[ 4]) {if  ((col=((myword>>16)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[ 5]) {if  ((col=((myword>>20)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[ 6]) {if  ((col=((myword>>24)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[ 7]) {if  ((col=((myword>>28)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}

                    myword = gfxdata[0];
                    if (dda_x_skip[ 8]) {if  ((col=((myword>>0)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[ 9]) {if  ((col=((myword>>4)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[10]) {if  ((col=((myword>>8)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[11]) {if  ((col=((myword>>12)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[12]) {if  ((col=((myword>>16)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[13]) {if  ((col=((myword>>20)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[14]) {if  ((col=((myword>>24)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[15]) {if  ((col=((myword>>28)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    br-=buf_w_yflip;
                    l++;
                }
            } else {
#ifdef DEBUG_VIDEO
                br= (unsigned short *)bmp+(sy)*544+sx;
#else
                br= (unsigned short *)bmp+(sy)*(buffer->pitch>>1)+sx;
#endif
                for(y=0;y<zy;y++) {
                    gfxdata+=l_y_skip[l]<<1;
                    myword = gfxdata[1];
                    if (dda_x_skip[ 0]) {if  ((col=((myword>>0)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 1]) {if  ((col=((myword>>4)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 2]) {if  ((col=((myword>>8)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 3]) {if  ((col=((myword>>12)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 4]) {if  ((col=((myword>>16)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 5]) {if  ((col=((myword>>20)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 6]) {if  ((col=((myword>>24)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 7]) {if  ((col=((myword>>28)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 

                    myword = gfxdata[0];
                    if (dda_x_skip[ 8]) {if  ((col=((myword>>0)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 9]) {if  ((col=((myword>>4)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[10]) {if  ((col=((myword>>8)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[11]) {if  ((col=((myword>>12)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[12]) {if  ((col=((myword>>16)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[13]) {if  ((col=((myword>>20)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[14]) {if  ((col=((myword>>24)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[15]) {if  ((col=((myword>>28)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 

                    br+=buf_w;
                    l++;
                }
            }
        }else {
            l=0;
            if (yflip) {
#ifdef DEBUG_VIDEO
                br= (unsigned short *)bmp+((zy-1)+sy)*544+sx;
#else
                br= (unsigned short *)bmp+((zy-1)+sy)*(buffer->pitch>>1)+sx;
#endif
                for(y=0;y<zy;y++) {

                    gfxdata+=l_y_skip[l]<<1;
                    myword = gfxdata[0];
                    if (dda_x_skip[ 0]) {if ((col=((myword>>28)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 1]) {if ((col=((myword>>24)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 2]) {if ((col=((myword>>20)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 3]) {if ((col=((myword>>16)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 4]) {if ((col=((myword>>12)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 5]) {if ((col=((myword>>8)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 6]) {if ((col=((myword>>4)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 7]) {if ((col=((myword>>0)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
	      
                    myword = gfxdata[1];
                    if (dda_x_skip[ 8]) {if ((col=((myword>>28)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 9]) {if ((col=((myword>>24)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[10]) {if ((col=((myword>>20)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[11]) {if ((col=((myword>>16)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[12]) {if ((col=((myword>>12)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[13]) {if ((col=((myword>>8)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[14]) {if ((col=((myword>>4)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[15]) {if ((col=((myword>>0)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    l++; 
                    br-=buf_w_yflip;
                }
            } else {
#ifdef DEBUG_VIDEO
                br= (unsigned short *)bmp+(sy)*544+sx;
#else
                br= (unsigned short *)bmp+(sy)*(buffer->pitch>>1)+sx;
#endif
                for(y=0;y<zy;y++) {

                    gfxdata+=l_y_skip[l]<<1;
                    myword = gfxdata[0];
                    if (dda_x_skip[ 0]) {if ((col=((myword>>28)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 1]) {if ((col=((myword>>24)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 2]) {if ((col=((myword>>20)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 3]) {if ((col=((myword>>16)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 4]) {if ((col=((myword>>12)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 5]) {if ((col=((myword>>8)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 6]) {if ((col=((myword>>4)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 7]) {if ((col=((myword>>0)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
	      
                    myword = gfxdata[1];
                    if (dda_x_skip[ 8]) {if ((col=((myword>>28)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 9]) {if ((col=((myword>>24)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[10]) {if ((col=((myword>>20)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[11]) {if ((col=((myword>>16)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[12]) {if ((col=((myword>>12)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[13]) {if ((col=((myword>>8)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[14]) {if ((col=((myword>>4)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[15]) {if ((col=((myword>>0)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    l++;
                    br+=buf_w;
                }
            }
        }
    }
}

static inline void RENAME(draw_scanline)(unsigned int tileno,int yoffs,int sx,int line,int zx,
					 int color,int xflip,unsigned char *bmp)
{
    unsigned int *gfxdata,myword;
    unsigned char col;
    unsigned short *br;
    unsigned int *paldata=(unsigned int *)&current_pc_pal[16*color];

    tileno=tileno%memory.nb_of_tiles;
    gfxdata = (unsigned int *)&memory.rom.tiles.p[ (tileno<<7)];
    gfxdata+=(yoffs<<1);
  
    if (gfxdata[1]+gfxdata[0]==0) return;

    if (zx==16) {
        if (xflip)
        {
#ifdef DEBUG_VIDEO
            br= (unsigned short *)bmp+(line)*(512+32)+sx;
#else
            br= (unsigned short *)bmp+(line)*(buffer->pitch>>1)+sx;
#endif

            myword = gfxdata[1];
            col=(myword>>0)&0xf; if (col) PUTPIXEL(*br,paldata[col]); br++;
            col=(myword>>4)&0xf; if (col) PUTPIXEL(*br,paldata[col]); br++;
            col=(myword>>8)&0xf; if (col) PUTPIXEL(*br,paldata[col]); br++;
            col=(myword>>12)&0xf; if (col) PUTPIXEL(*br,paldata[col]); br++;
            col=(myword>>16)&0xf; if (col) PUTPIXEL(*br,paldata[col]); br++;
            col=(myword>>20)&0xf; if (col) PUTPIXEL(*br,paldata[col]); br++;
            col=(myword>>24)&0xf; if (col) PUTPIXEL(*br,paldata[col]); br++;
            col=(myword>>28)&0xf; if (col) PUTPIXEL(*br,paldata[col]); br++;

            myword = gfxdata[0];
            col=(myword>>0)&0xf; if (col) PUTPIXEL(*br,paldata[col]); br++;
            col=(myword>>4)&0xf; if (col) PUTPIXEL(*br,paldata[col]); br++;
            col=(myword>>8)&0xf; if (col) PUTPIXEL(*br,paldata[col]); br++;
            col=(myword>>12)&0xf; if (col) PUTPIXEL(*br,paldata[col]); br++;
            col=(myword>>16)&0xf; if (col) PUTPIXEL(*br,paldata[col]); br++;
            col=(myword>>20)&0xf; if (col) PUTPIXEL(*br,paldata[col]); br++;
            col=(myword>>24)&0xf; if (col) PUTPIXEL(*br,paldata[col]); br++;
            col=(myword>>28)&0xf; if (col) PUTPIXEL(*br,paldata[col]); br++;
        }else {
#ifdef DEBUG_VIDEO
            br= (unsigned short *)bmp+(line)*(512+32)+sx;
#else
            br= (unsigned short *)bmp+(line)*(buffer->pitch>>1)+sx;
#endif
            myword = gfxdata[0];
            col=(myword>>28)&0xf; if (col) PUTPIXEL(*br,paldata[col]);br++; 
            col=(myword>>24)&0xf; if (col) PUTPIXEL(*br,paldata[col]);br++; 
            col=(myword>>20)&0xf; if (col) PUTPIXEL(*br,paldata[col]);br++; 
            col=(myword>>16)&0xf; if (col) PUTPIXEL(*br,paldata[col]);br++; 
            col=(myword>>12)&0xf; if (col) PUTPIXEL(*br,paldata[col]);br++; 
            col=(myword>>8)&0xf; if (col) PUTPIXEL(*br,paldata[col]);br++; 
            col=(myword>>4)&0xf; if (col) PUTPIXEL(*br,paldata[col]);br++; 
            col=(myword>>0)&0xf; if (col) PUTPIXEL(*br,paldata[col]);br++; 
	      
            myword = gfxdata[1];
            col=(myword>>28)&0xf; if (col) PUTPIXEL(*br,paldata[col]);br++; 
            col=(myword>>24)&0xf; if (col) PUTPIXEL(*br,paldata[col]);br++; 
            col=(myword>>20)&0xf; if (col) PUTPIXEL(*br,paldata[col]);br++; 
            col=(myword>>16)&0xf; if (col) PUTPIXEL(*br,paldata[col]);br++; 
            col=(myword>>12)&0xf; if (col) PUTPIXEL(*br,paldata[col]);br++; 
            col=(myword>>8)&0xf; if (col) PUTPIXEL(*br,paldata[col]);br++; 
            col=(myword>>4)&0xf; if (col) PUTPIXEL(*br,paldata[col]);br++; 
            col=(myword>>0)&0xf; if (col) PUTPIXEL(*br,paldata[col]);br++; 
        }
    }else { // zx!=16
        if (xflip) {
#ifdef DEBUG_VIDEO
            br= (unsigned short *)bmp+(line)*(512+32)+sx;
#else
            br= (unsigned short *)bmp+(line)*(buffer->pitch>>1)+sx;
#endif
            myword = gfxdata[1];
            if (dda_x_skip[ 0]) {if  ((col=((myword>>0)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[ 1]) {if  ((col=((myword>>4)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[ 2]) {if  ((col=((myword>>8)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[ 3]) {if  ((col=((myword>>12)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[ 4]) {if  ((col=((myword>>16)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[ 5]) {if  ((col=((myword>>20)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[ 6]) {if  ((col=((myword>>24)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[ 7]) {if  ((col=((myword>>28)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 

            myword = gfxdata[0];
            if (dda_x_skip[ 8]) {if  ((col=((myword>>0)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[ 9]) {if  ((col=((myword>>4)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[10]) {if  ((col=((myword>>8)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[11]) {if  ((col=((myword>>12)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[12]) {if  ((col=((myword>>16)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[13]) {if  ((col=((myword>>20)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[14]) {if  ((col=((myword>>24)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[15]) {if  ((col=((myword>>28)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
	  
        }else {
#ifdef DEBUG_VIDEO
            br= (unsigned short *)bmp+(line)*(512+32)+sx;
#else
            br= (unsigned short *)bmp+(line)*(buffer->pitch>>1)+sx;
#endif

            myword = gfxdata[0];
            if (dda_x_skip[ 0]) {if ((col=((myword>>28)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[ 1]) {if ((col=((myword>>24)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[ 2]) {if ((col=((myword>>20)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[ 3]) {if ((col=((myword>>16)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[ 4]) {if ((col=((myword>>12)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[ 5]) {if ((col=((myword>>8)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[ 6]) {if ((col=((myword>>4)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[ 7]) {if ((col=((myword>>0)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
	      
            myword = gfxdata[1];
            if (dda_x_skip[ 8]) {if ((col=((myword>>28)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[ 9]) {if ((col=((myword>>24)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[10]) {if ((col=((myword>>20)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[11]) {if ((col=((myword>>16)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[12]) {if ((col=((myword>>12)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[13]) {if ((col=((myword>>8)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[14]) {if ((col=((myword>>4)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
            if (dda_x_skip[15]) {if ((col=((myword>>0)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
	  
        }
    }
}


#undef RENAME
#undef PUTPIXEL
