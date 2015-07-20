ifndef inc_pkg_stdcxx_headers
inc_pkg_stdcxx_headers := 1

ifeq ($(ENV), android)
 ifeq ($(android_stdcxx), gnu)
  CPPFLAGS += -I$(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/$(gccVersion)/libs/$(android_abi)/include \
   -I$(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/$(gccVersion)/include
 else
  CPPFLAGS += -I$(ANDROID_NDK_PATH)/sources/cxx-stl/llvm-libc++/libcxx/include \
   -I$(ANDROID_NDK_PATH)/sources/android/support/include
 endif
else ifneq ($(filter macosx ios, $(ENV)),)
 include $(buildSysPath)/package/stdc++.mk
endif

endif