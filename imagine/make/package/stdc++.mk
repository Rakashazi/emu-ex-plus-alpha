ifndef inc_pkg_stdcxx
inc_pkg_stdcxx := 1

ifeq ($(ENV), webos) # force a static stdc++ for WebOS since the base 1.4.5 version uses GCC 4.3.5's which is incompatible with newer versions
 LDLIBS += /usr/lib/gcc/arm-none-linux-gnueabi/$(gccVersion)/libstdc++.a
else ifneq ($(filter macosx ios ,$(ENV)),)
 # skip stdc++-headers.mk since the include path is implicit
 inc_pkg_stdcxx_headers := 1
 ifeq ($(SUBARCH), armv6)
  CPPFLAGS += -I$(extraSysroot)/include/c++/v1
  LDLIBS += $(extraSysroot)/lib/libc++.a  $(extraSysroot)/lib/libcxxabi.a
 else
  # Use clang to get include/lib path
  BASE_CXXFLAGS += -stdlib=libc++
  LDLIBS += -stdlib=libc++ -lc++
 endif
else
 ifdef pkg_stdcxxStaticLib
  LDLIBS += $(pkg_stdcxxStaticLib)
 else
  LDLIBS += -lstdc++
 endif
endif

include $(buildSysPath)/package/stdc++-headers.mk

configDefs += CONFIG_STDCXX

endif