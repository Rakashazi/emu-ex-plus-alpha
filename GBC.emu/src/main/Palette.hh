#include <imagine/util/ansiTypes.h>

struct GBPalette
{
	uint bg[4], sp1[4], sp2[4];
};

GBPalette const *findGbcTitlePal(char const *title);
