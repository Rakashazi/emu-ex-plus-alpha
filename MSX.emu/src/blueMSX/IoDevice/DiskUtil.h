/*************************************************************/
/**                                                         **/
/**                diskwork.h                               **/
/**                                                         **/
/** Common definitions for programs working with disk-images**/
/**                                                         **/
/**                                                         **/
/** Copyright (c) Arnold Metselaar 1996                     **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#ifndef O_BINARY
#define O_BINARY 0
#endif

#define usint unsigned short int
#define byte unsigned char

#define EOF_FAT 0xFFF /* signals EOF in FAT */

typedef struct de {
  char d_fname[8];
  char d_ext[3];
  char d_attrib;
  char d_reserv[10]; /* unused */
  byte d_time[2];     /* byte-ordering of ints is machine-dependent */
  byte d_date[2];
  byte d_first[2];
  byte d_size[4];
} DirEntry;

/* macros to change DirEntries */
#define setsh(x,y) {x[0]=y;x[1]=y>>8;}
#define setlg(x,y) {x[0]=y;x[1]=y>>8;x[2]=y>>16;x[3]=y>>24;}

/* macros to read DirEntries */
#define rdsh(x) (x[0]+(x[1]<<8))
#define rdlg(x) (x[0]+(x[1]<<8)+(x[2]<<16)+(x[3]<<24))

#define PError(x) { fprintf(stderr,"%s: ",progname); perror(x); }

#define seclen 512   /* length of sector */
#define cluslen 1024 /* length of cluster */

void *xalloc(size_t len)
{ 
  void *p;

  if (!(p=malloc(len))) 
    {
      puts("Out of memory\n"); exit(2);
    }
  return p;
}

extern byte *FAT;

/* read FAT-entry from FAT in memory */
usint ReadFAT(usint clnr)
{ register byte *P;

  P=FAT+(clnr*3)/2;
  return (clnr&1)? (P[0]>>4)+(P[1]<<4) : P[0]+((P[1]&0x0F)<<8);
}

/* write an entry to FAT in memory */
void WriteFAT(usint clnr, usint val)
{ register byte *P;

  P=FAT+(clnr*3)/2;
  if (clnr&1)
    { 
      P[0]=(P[0]&0x0F)+(val<<4);
      P[1]=val>>4;
    }
  else
    {
      P[0]=val;
      P[1]=(P[1]&0xF0)+((val>>8)&0x0F);
    }
}
