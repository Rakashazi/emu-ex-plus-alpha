#pragma once

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#ifdef __clang__
	#define CLANG_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#endif

#ifdef __cplusplus
	#define CLINK extern "C"
	#define BEGIN_C_DECLS extern "C" {
	#define END_C_DECLS }
#else
	#define CLINK
	#define BEGIN_C_DECLS
	#define END_C_DECLS
#endif

// Make symbol remain visible after linking
#define LVISIBLE __attribute__((visibility("default"), externally_visible))

// Make symbol remain visible outside compilation unit
#define EVISIBLE __attribute__((externally_visible))

// Shortcut for GCC attributes
#define ATTRS(...) __attribute__((__VA_ARGS__))

#define INITFIRST __attribute__((init_priority(101)))

#define var_isConst(E) __builtin_constant_p(E)
#define likely(E) __builtin_expect(!!(E),1)
#define unlikely(E) __builtin_expect(!!(E),0)

#define PP_STRINGIFY(A) #A
#define PP_STRINGIFY_EXP(A) PP_STRINGIFY(A)
