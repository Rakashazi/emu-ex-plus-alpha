#ifndef VICE_ZFILE_H
#error must be included as part of zfile.h
#endif

VICE_API FILE *zfile_fopen(const char *name, const char *mode);
VICE_API off_t archdep_file_size(FILE *stream);
