# ARMv7 with LTO needs at least iOS 4.3, libc++ needs 5.0
minIOSVer = 4.3
ifdef ios_armv7State # default is -mthumb by compiler if not defined
 IOS_FLAGS += $(ios_armv7State)
endif
