ifndef inc_pkg_stdcxx_headers
inc_pkg_stdcxx_headers := 1

ifeq ($(ENV), android)
CPPFLAGS += -I$(ANDROID_NDK_PATH)/sources/cxx-stl/stlport/stlport
endif

endif