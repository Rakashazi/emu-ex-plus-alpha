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
#if GCC_VERSION <= 40101
	// hack to prevent PS3 GCC from crashing
	#define EVISIBLE
#else
	#define EVISIBLE __attribute__((externally_visible))
#endif

// Shortcut for GCC attributes
#define ATTRS(...) __attribute__((__VA_ARGS__))

#define var_isConst(E) __builtin_constant_p(E)
#define likely(E) __builtin_expect((E),1)
#define unlikely(E) __builtin_expect((E),0)

#ifdef __cplusplus

#if GCC_VERSION < 40700 // GCC < 4.7 doesn't properly define __cplusplus,
						// neither does Clang 3.0 which ids as GCC 4.2.1
	#ifdef __GXX_EXPERIMENTAL_CXX0X__
		#define CONFIG_CXX11
	#endif
#else
	#if __cplusplus >= 201103L
		#define CONFIG_CXX11
	#endif
#endif

#ifndef CONFIG_CXX11

//#warning "DEBUG: not using C++11"

#define constexpr

static const class nullptr_t
{
  public:
    template<class T>
    inline operator T*() const // convertible to any type of null non-member pointer...
    { return 0; }

    template<class C, class T>
    inline operator T C::*() const   // or any type of null member pointer...
    { return 0; }

  private:
    void operator&() const;  // Can't take address of nullptr

} nullptr = { };

#else

//#warning "DEBUG: using C++11"

// Initializer list syntax not yet supported in Clang 3.0
// TODO: remove once Clang 3.1+ can compile everything
#if __clang_major__ == 3 && __clang_minor__ == 0
	#define CXX11_INIT_LIST(...) (__VA_ARGS__)
#else
	#define CXX11_INIT_LIST(...) __VA_ARGS__
#endif

#endif

#endif // __cplusplus
