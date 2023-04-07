#pragma once

void S9xPrintf(const char* msg, ...) __attribute__ ((format (printf, 1, 2)));

#define printf S9xPrintf
#define remove removeFileHelper
#define fopen fopenHelper
