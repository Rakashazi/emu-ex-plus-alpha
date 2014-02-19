#pragma once

/* include/config.h.  Generated from config.h.in by configure.  */
/* include/config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

#ifdef CONFIG_BASE_PS3
	/* Define if we are compiling for PPC architectures. */
	#define ARCH_POWERPC 1

	/* Define if we are compiling with AltiVec usage. */
	#define ARCH_POWERPC_ALTIVEC 1
#endif

#if defined(__x86_64__) || defined(__i386__)
	/* Define if we are compiling for x86 architectures. */
	#define ARCH_X86 1
#endif

#define BLIP_BUFFER_FAST 1

/* Blip Buffer resample ratio accuracy. */
#define BLIP_BUFFER_ACCURACY 20

/* Blip Buffer maximum quality. */
#define BLIP_MAX_QUALITY 32

/* Blip Buffer phase bits count. */
#define BLIP_PHASE_BITS 8

/* Define to one of `_getb67', `GETB67', `getb67' for Cray-2 and Cray-YMP
   systems. This function is required for `alloca.c' support on those systems.
   */
/* #undef CRAY_STACKSEG_END */

/* Define to 1 if using `alloca.c'. */
/* #undef C_ALLOCA */

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
/* #undef ENABLE_NLS */

/* Define to 1 if you have `alloca', as a function or macro. */
#define HAVE_ALLOCA 1

/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).
   */
#define HAVE_ALLOCA_H 1

#ifndef CONFIG_BASE_PS3
	/* Define to 1 if you have the `asprintf' function. */
	#define HAVE_ASPRINTF 1
#endif

/* Define to 1 if the compiler understands __builtin_expect. */
#define HAVE_BUILTIN_EXPECT 1

#ifndef CONFIG_BASE_PS3
	/* Define to 1 if you have the `clock_gettime' function. */
	#define HAVE_CLOCK_GETTIME 1
#endif

/* Define to 1 if you have the `fcntl' function. */
#define HAVE_FCNTL 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `fwprintf' function. */
#define HAVE_FWPRINTF 1

/* Define if you have the 'intmax_t' type in <stdint.h> or <inttypes.h>. */
#define HAVE_INTMAX_T 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define if <inttypes.h> exists, doesn't clash with <sys/types.h>, and
   declares uintmax_t. */
#define HAVE_INTTYPES_H_WITH_UINTMAX 1

#if defined(CONFIG_BASE_PS3)
	#define CONFIG_SUPPORT_32BPP
#endif

#ifdef CONFIG_BASE_X11
	/* Define if we are compiling with libcdio support. */
	//#define HAVE_LIBCDIO 1
#endif

/* Define if we are compiling with libsndfile support. */
#define HAVE_LIBSNDFILE 1

/* Define to 1 if you have the `z' library (-lz). */
#define HAVE_LIBZ 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if the system has the type `long long int'. */
#define HAVE_LONG_LONG_INT 1

#ifndef CONFIG_BASE_PS3
	/* Define to 1 if you have the `madvise' function. */
	#define HAVE_MADVISE 1
#endif

/* Define to 1 if you have the `memcmp' function. */
#define HAVE_MEMCMP 1

/* Define to 1 if you have the `memcpy' function. */
#define HAVE_MEMCPY 1

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

#ifdef __x86_64__
	/* Define to 1 if you have the `mempcpy' function. */
	#define HAVE_MEMPCPY 1
#endif

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define to 1 if you have the `mkdir' function. */
#define HAVE_MKDIR 1

#ifndef CONFIG_BASE_PS3
	/* Define to 1 if you have a working `mmap' system call. */
	#define HAVE_MMAP 1

	/* Define to 1 if you have the `munmap' function. */
	#define HAVE_MUNMAP 1
#endif

/* Define if your printf() function supports format strings with positions. */
#define HAVE_POSIX_PRINTF 1

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define to 1 if you have the <stddef.h> header file. */
#define HAVE_STDDEF_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define if <stdint.h> exists, doesn't clash with <sys/types.h>, and declares
   uintmax_t. */
#define HAVE_STDINT_H_WITH_UINTMAX 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

#ifdef __x86_64__
	/* Define to 1 if you have the `strerror_r' function. */
	#define HAVE_STRERROR_R 1
#endif

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strtoul' function. */
#define HAVE_STRTOUL 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define if you have the 'uintmax_t' type in <stdint.h> or <inttypes.h>. */
#define HAVE_UINTMAX_T 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if the system has the type `unsigned long long int'. */
#define HAVE_UNSIGNED_LONG_LONG_INT 1

/* Define to 1 or 0, depending whether the compiler supports simple visibility
   declarations. */
#define HAVE_VISIBILITY 1

/* Define if you have the 'wchar_t' type. */
#define HAVE_WCHAR_T 1

/* Define to 1 if you have the `wcslen' function. */
#define HAVE_WCSLEN 1

/* Define if you have the 'wint_t' type. */
#define HAVE_WINT_T 1

/* Define to 1 if you have the `_mkdir' function. */
/* #undef HAVE__MKDIR */

#ifndef CONFIG_BASE_PS3
	/* Define on little-endian platforms. */
	#define LSB_FIRST 1
#endif

/* Mednafen version definition. */
#define MEDNAFEN_VERSION "0.9.32.1"

/* Mednafen version numeric. */
#define MEDNAFEN_VERSION_NUMERIC 0x000916

/* Define if config.h is present */
#define MINILZO_HAVE_CONFIG_H 1

/* Define if mkdir takes only one argument. */
/* #undef MKDIR_TAKES_ONE_ARG */

/* Define to use fixed-point MPC decoder. */
#define MPC_FIXED_POINT 1

#ifndef CONFIG_BASE_PS3
	/* Define on little-endian platforms. */
	#define MPC_LITTLE_ENDIAN 1
#endif

#ifdef CONFIG_BASE_PS3
	/* Define on big-endian platforms. */
	#define MSB_FIRST 1
#endif

/* Define if we are compiling with network play code. */
/* #undef NETWORK */

/* Name of package */
#define PACKAGE "mednafen"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME ""

/* Define to the full name and version of this package. */
#define PACKAGE_STRING ""

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME ""

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION ""

/* Defines the filesystem path-separator type. */
#define PSS_STYLE 1

/* The size of `double', as computed by sizeof. */
#define SIZEOF_DOUBLE 8

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of `long', as computed by sizeof. */
#ifdef __x86_64__
	#define SIZEOF_LONG 8
#else
	#define SIZEOF_LONG 4
#endif

/* The size of `long long', as computed by sizeof. */
#define SIZEOF_LONG_LONG 8

/* The size of `ptrdiff_t', as computed by sizeof. */
#ifdef __x86_64__
	#define SIZEOF_PTRDIFF_T 8
#else
	#define SIZEOF_PTRDIFF_T 4
#endif

/* The size of `short', as computed by sizeof. */
#define SIZEOF_SHORT 2

/* The size of `size_t', as computed by sizeof. */
#ifdef __x86_64__
	#define SIZEOF_SIZE_T 8
#else
	#define SIZEOF_SIZE_T 4
#endif

/* The size of `void *', as computed by sizeof. */
#ifdef __x86_64__
	#define SIZEOF_VOID_P 8
#else
	#define SIZEOF_VOID_P 4
#endif

/* The size of `__int64', as computed by sizeof. */
#define SIZEOF___INT64 0

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif

/* Define if we are compiling with debugger. */
/* #undef WANT_DEBUGGER */

/* Define if we are compiling with GBA emulation. */
/* #undef WANT_GBA_EMU */

/* Define if we are compiling with GB emulation. */
/* #undef WANT_GB_EMU */

/* Define if we are compiling with internal CJK fonts. */
/* #undef WANT_INTERNAL_CJK */

/* Define if we are compiling with Lynx emulation. */
/* #undef WANT_LYNX_EMU */

/* Define if we are compiling with Sega Genesis/MegaDrive emulation. */
/* #undef WANT_MD_EMU */

/* Define if we are compiling with NES emulation. */
/* #undef WANT_NES_EMU */

/* Define if we are compiling with NGP emulation. */
/* #undef WANT_NGP_EMU */

/* Define if we are compiling with PCE emulation. */
/* #undef WANT_PCE_EMU */

/* Define if we are compiling with separate fast PCE emulation. */
#define WANT_PCE_FAST_EMU 1

/* Define if we are compiling with PC-FX emulation. */
/* #undef WANT_PCFX_EMU */

/* Define if we are compiling with SMS+GG emulation. */
/* #undef WANT_SMS_EMU */

/* Define if we are compiling with SNES emulation. */
/* #undef WANT_SNES_EMU */

/* Define if we are compiling with Virtual Boy emulation. */
/* #undef WANT_VB_EMU */

/* Define if we are compiling with WonderSwan emulation. */
/* #undef WANT_WSWAN_EMU */

/* Define if we are compiling for Win32. */
/* #undef WIN32 */

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Define to 1 if on MINIX. */
/* #undef _MINIX */

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
/* #undef _POSIX_1_SOURCE */

/* Define to 1 if you need to in order for `stat' and other things to work. */
/* #undef _POSIX_SOURCE */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define as the type of the result of subtracting two pointers, if the system
   doesn't define it. */
/* #undef ptrdiff_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to unsigned long or unsigned long long if <stdint.h> and
   <inttypes.h> don't define. */
/* #undef uintmax_t */


#define __libc_lock_t                   gl_lock_t
#define __libc_lock_define              gl_lock_define
#define __libc_lock_define_initialized  gl_lock_define_initialized
#define __libc_lock_init                gl_lock_init
#define __libc_lock_lock                gl_lock_lock
#define __libc_lock_unlock              gl_lock_unlock
#define __libc_lock_recursive_t                   gl_recursive_lock_t
#define __libc_lock_define_recursive              gl_recursive_lock_define
#define __libc_lock_define_initialized_recursive  gl_recursive_lock_define_initialized
#define __libc_lock_init_recursive                gl_recursive_lock_init
#define __libc_lock_lock_recursive                gl_recursive_lock_lock
#define __libc_lock_unlock_recursive              gl_recursive_lock_unlock
#define glthread_in_use  libintl_thread_in_use
#define glthread_lock_init     libintl_lock_init
#define glthread_lock_lock     libintl_lock_lock
#define glthread_lock_unlock   libintl_lock_unlock
#define glthread_lock_destroy  libintl_lock_destroy
#define glthread_rwlock_init     libintl_rwlock_init
#define glthread_rwlock_rdlock   libintl_rwlock_rdlock
#define glthread_rwlock_wrlock   libintl_rwlock_wrlock
#define glthread_rwlock_unlock   libintl_rwlock_unlock
#define glthread_rwlock_destroy  libintl_rwlock_destroy
#define glthread_recursive_lock_init     libintl_recursive_lock_init
#define glthread_recursive_lock_lock     libintl_recursive_lock_lock
#define glthread_recursive_lock_unlock   libintl_recursive_lock_unlock
#define glthread_recursive_lock_destroy  libintl_recursive_lock_destroy
#define glthread_once                 libintl_once
#define glthread_once_call            libintl_once_call
#define glthread_once_singlethreaded  libintl_once_singlethreaded
