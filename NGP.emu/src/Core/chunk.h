#include "neopop.h"

#define MKTAG(a, b, c, d)	(((a)<<24)|((b)<<16)|((c)<<8)|(d))

#define TAG_NGPS	MKTAG('N','G','P','S')
#define TAG_SNAP	MKTAG('S','N','A','P')
#define TAG_HIST	MKTAG('H','I','S','T')
#define TAG_EVNT	MKTAG('E','V','N','T')
#define TAG_TIME	MKTAG('T','I','M','E')
#define TAG_ROM		MKTAG('R','O','M',' ')
#define TAG_ROMH	MKTAG('R','O','M','H')
#define TAG_RAM		MKTAG('R','A','M',' ')
#define TAG_FLSH	MKTAG('F','L','S','H')
#define TAG_REGS	MKTAG('R','E','G','S')
#define TAG_RST		MKTAG('R','S','T',' ')
#define TAG_EOD		MKTAG('E','O','D',' ')

#define OPT_ROM		0x0001
#define OPT_ROMH	0x0002
#define OPT_FLSH	0x0004
#define OPT_TIME	0x0008
#define OPT_RAM		0x0010
#define OPT_REGS	0x0020

bool read_chunk(FILE *, uint32 *, uint32 *);
bool read_header(FILE *);
bool read_SNAP(FILE *, uint32);

bool write_header(FILE *);
bool write_EOD(FILE *);
bool write_SNAP(FILE *, int);
