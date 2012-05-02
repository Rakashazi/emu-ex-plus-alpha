#pragma once

#include <stdio.h>

long file_size(FILE *f)
{
	long savedPos = ftell(f);
	fseek(f, 0, SEEK_END );
	long size = ftell(f);
	fseek(f, savedPos, SEEK_SET );
	return(size);
}

static char *file_whenceToStr(int whence)
{
	switch(whence)
	{
		case SEEK_CUR: return "Cur";
		case SEEK_SET: return "Set";
		case SEEK_END: return "End";
		default: return "";
	}
}
