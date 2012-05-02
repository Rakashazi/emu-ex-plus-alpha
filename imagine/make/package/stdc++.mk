ifndef inc_pkg_stdcxx
inc_pkg_stdcxx := 1

include $(IMAGINE_PATH)/make/package/stdc++-headers.mk

ifeq ($(ENV), webos) # force a static stdc++ for WebOS since the base 1.4.5 version uses GCC 4.3.5's which is incompatible with newer versions
LDLIBS += /usr/lib/gcc/arm-none-linux-gnueabi/4.6.3/libstdc++.a #/usr/lib/gcc/arm-none-linux-gnueabi/4.3.5/libstdc++.so
else ifeq ($(ENV), android)
LDLIBS += $(android_stdcxxLib)
else
LDLIBS += -lstdc++
endif

configDefs += CONFIG_STDCXX CONFIG_SUPCXX

endif