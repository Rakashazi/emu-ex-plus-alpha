#pragma once

#include <util/builtins.h>

#define ASCII_LF 0xA
#define ASCII_CR 0xD

#ifdef __cplusplus

template <class T>
T baseNamePos(T path);

#endif

BEGIN_C_DECLS

void baseName(const char *path, char *pathOut);
void baseNameInPlace(char *path);
void dirName(const char *path, char *pathOut);
void dirNameInPlace(char *path);

END_C_DECLS
