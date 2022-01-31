#ifndef VICE_SYSFILE_H
#error must be included as part of sysfile.h
#endif

VICE_API int sysfile_init(const char *emu_id);
VICE_API FILE *sysfile_open(const char *name, const char *subpath, char **complete_path_return, const char *open_mode);
VICE_API int sysfile_locate(const char *name, const char *subpath, char **complete_path_return);
VICE_API int sysfile_load(const char *name, const char *subpath, uint8_t *dest, int minsize, int maxsize);
