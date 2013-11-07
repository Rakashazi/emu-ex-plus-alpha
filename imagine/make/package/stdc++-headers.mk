ifndef inc_pkg_stdcxx_headers
inc_pkg_stdcxx_headers := 1

ifeq ($(ENV), android)
 ifeq ($(android_stdcxx), gnu)
  CPPFLAGS += -I$(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/$(gccVersion)/libs/$(android_abi)/include \
   -I$(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/$(gccVersion)/include
 else
  CPPFLAGS += -I$(ANDROID_NDK_PATH)/sources/cxx-stl/stlport/stlport
 endif
else ifeq ($(ENV), ios)
 ifeq ($(SUBARCH), armv6)
  CPPFLAGS += -I$(extraSysroot)/include/c++/v1
 else
  # default Xcode libc++ path
  CPPFLAGS += -I$(XCODE_PATH)/Toolchains/XcodeDefault.xctoolchain/usr/lib/c++/v1
 endif
else ifeq ($(ENV), macosx)
 BASE_CXXFLAGS += -stdlib=libc++
endif

endif