#pragma once
#include <imagine/io/IO.hh>
#include <cstdio>

// Functions to wrap basic stdio functionality

int fgetc(IO &stream);

size_t fwrite(void *ptr, size_t size, size_t nmemb, IO &stream);

size_t fread(void *ptr, size_t size, size_t count, IO &stream);

int fseek(IO &stream, long int offset, int origin);

long int ftell(IO &stream);

int fclose(IO &stream);
