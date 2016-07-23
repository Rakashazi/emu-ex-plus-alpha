ifndef inc_pkg_stdcxx_headers
inc_pkg_stdcxx_headers := 1

ifeq ($(ENV), android)
 STDCXXINC = -isystem $(ANDROID_NDK_PATH)/sources/cxx-stl/llvm-libc++/include \
  -isystem $(ANDROID_NDK_PATH)/sources/android/support/include
else ifneq ($(filter macosx ios, $(ENV)),)
 include $(buildSysPath)/package/stdc++.mk
endif

endif