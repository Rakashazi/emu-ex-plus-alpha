#pragma once

#include <imagine/io/IO.hh>

unsigned hasROMExtension(const char *name);
int loadArchive(void *buff, unsigned bytes, const char *path, FS::FileString &nameInArchive);
