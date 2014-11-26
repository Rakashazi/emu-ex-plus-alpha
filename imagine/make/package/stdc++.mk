ifndef inc_pkg_stdcxx
inc_pkg_stdcxx := 1

ifneq ($(filter macosx ios, $(ENV)),)
 # skip stdc++-headers.mk since the include path is implicit
 inc_pkg_stdcxx_headers := 1
 ifeq ($(SUBARCH), armv6)
  CPPFLAGS += -I$(IMAGINE_SDK_PLATFORM_PATH)/include/c++/v1
  STDCXXLIB = $(IMAGINE_SDK_PLATFORM_PATH)/lib/libc++.a $(IMAGINE_SDK_PLATFORM_PATH)/lib/libcxxabi.a
 else
  # Use clang to get include/lib path
  BASE_CXXFLAGS += -stdlib=libc++
  STDCXXLIB = -stdlib=libc++
 endif
else
 ifdef pkg_stdcxxStaticLib
  STDCXXLIB = $(pkg_stdcxxStaticLib)
 endif
endif

include $(buildSysPath)/package/stdc++-headers.mk

configDefs += CONFIG_STDCXX

endif