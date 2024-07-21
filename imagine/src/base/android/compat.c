#include <stdlib.h>
#include <ctype.h>
#include <wctype.h>
#include <wchar.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>

// Implementation of missing libc functions when compiling with newer NDK headers
// and an old minimum SDK level, mostly from Bionic

int fstat64(int fd, struct stat64* buf) { return fstat(fd, (struct stat*)buf); }

void android_set_abort_message(const char*) {}

size_t __ctype_get_mb_cur_max(void) { return 1; }

float strtof(const char* nptr, char** endptr) {
  // N.B. Double-rounding makes this function incorrect for some inputs.
  double d = strtod(nptr, endptr);
  if (__builtin_isfinite(d) && __builtin_fabs(d) > FLT_MAX) {
    errno = ERANGE;
    return __builtin_copysign(__builtin_huge_valf(), d);
  }
  return __BIONIC_CAST(static_cast, float, d);
}

double atof(const char *nptr) { return (strtod(nptr, NULL)); }

int rand(void) { return (int)lrand48(); }

void srand(unsigned int __s) { srand48(__s); }

long random(void) { return lrand48(); }

void srandom(unsigned int __s) { srand48(__s); }

int grantpt(int __fd __attribute((unused))) {
  return 0; /* devpts does this all for us! */
}

int iswalnum_l(wint_t c, locale_t) {
  return iswalnum(c);
}

int iswalpha_l(wint_t c, locale_t) {
  return iswalpha(c);
}

int iswblank_l(wint_t c, locale_t) {
  return iswblank(c);
}

int iswcntrl_l(wint_t c, locale_t) {
  return iswcntrl(c);
}

int iswdigit_l(wint_t c, locale_t) {
  return iswdigit(c);
}

int iswgraph_l(wint_t c, locale_t) {
  return iswgraph(c);
}

int iswlower_l(wint_t c, locale_t) {
  return iswlower(c);
}

int iswprint_l(wint_t c, locale_t) {
  return iswprint(c);
}

int iswpunct_l(wint_t c, locale_t) {
  return iswpunct(c);
}

int iswspace_l(wint_t c, locale_t) {
  return iswspace(c);
}

int iswupper_l(wint_t c, locale_t) {
  return iswupper(c);
}

int iswxdigit_l(wint_t c, locale_t) {
  return iswxdigit(c);
}

wint_t towupper_l(wint_t c, locale_t) {
  return towupper(c);
}

wint_t towlower_l(wint_t c, locale_t) {
  return towlower(c);
}

int strcoll_l(const char *s1, const char *s2, locale_t) {
  return strcoll(s1, s2);
}

size_t strxfrm_l(char *dest, const char *src, size_t n, locale_t) {
  return strxfrm(dest, src, n);
}

size_t strftime_l(char *s, size_t max, const char *format, const struct tm *tm, locale_t) {
  return strftime(s, max, format, tm);
}

int wcscoll_l(const wchar_t *ws1, const wchar_t *ws2, locale_t) {
  return wcscoll(ws1, ws2);
}

size_t wcsxfrm_l(wchar_t *dest, const wchar_t *src, size_t n, locale_t) {
  return wcsxfrm(dest, src, n);
}
