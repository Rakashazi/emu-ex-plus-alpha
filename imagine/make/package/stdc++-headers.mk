ifndef inc_pkg_stdcxx_headers
inc_pkg_stdcxx_headers := 1

ifeq ($(ENV), android)
 ifeq ($(android_stdcxx), gnu)
  CPPFLAGS += -I$(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/$(gccVersion)/libs/$(android_abi)/include \
   -I$(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/$(gccVersion)/include
 else
  # TODO: libc++
 endif
else ifneq ($(filter macosx ios, $(ENV)),)
 include $(buildSysPath)/package/stdc++.mk
endif

endif