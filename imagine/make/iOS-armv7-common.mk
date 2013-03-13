# ARMv7 with LTO needs at least iOS 4.3, libc++ needs 5.0
minIOSVer := 5.0
BASE_CXXFLAGS += -stdlib=libc++
ifdef ios_armv7State # default is -mthumb by compiler if not defined
 IOS_FLAGS += $(ios_armv7State)
endif
CPPFLAGS += -I$(system_externalSysroot)/include
LDLIBS += -L$(system_externalSysroot)/lib -stdlib=libc++