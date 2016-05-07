ifndef inc_pkg_stdcxx_headers
inc_pkg_stdcxx_headers := 1

ifeq ($(ENV), android)
 ifeq ($(android_stdcxx), gnu)
  STDCXXINC = -I$(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/$(ANDROID_GCC_VERSION)/libs/$(android_abi)/include \
   -I$(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/$(ANDROID_GCC_VERSION)/include
 else
  STDCXXINC = -I$(ANDROID_NDK_PATH)/sources/cxx-stl/llvm-libc++/libcxx/include \
   -I$(ANDROID_NDK_PATH)/sources/android/support/include
 endif
else ifneq ($(filter macosx ios, $(ENV)),)
 include $(buildSysPath)/package/stdc++.mk
endif

endif