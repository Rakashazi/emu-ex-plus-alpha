#pragma once
#include <imagine/io/IO.hh>
#include <cstdio>

// Functions to wrap basic stdio functionality

namespace IG
{

int fgetc(IG::IO &stream);

size_t fwrite(void *ptr, size_t size, size_t nmemb, IG::IO &stream);

size_t fread(void *ptr, size_t size, size_t count, IG::IO &stream);

int fseek(IG::IO &stream, long int offset, int origin);

long int ftell(IG::IO &stream);

int fclose(IG::IO &stream);

}
