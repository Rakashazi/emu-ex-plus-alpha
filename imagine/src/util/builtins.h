#pragma once

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#ifdef __cplusplus
	#define CLINK extern "C"
#else
	#define CLINK
#endif

// Make symbol remain visible after linking
#define LVISIBLE __attribute__((visibility("default"), externally_visible))

// Make symbol remain visible outside compilation unit
#define EVISIBLE __attribute__((externally_visible))

// Shortcut for GCC attributes
#define ATTRS(...) __attribute__((__VA_ARGS__))

#define var_isConst(E) __builtin_constant_p(E)
#define likely(E) __builtin_expect((E),1)
#define unlikely(E) __builtin_expect((E),0)
