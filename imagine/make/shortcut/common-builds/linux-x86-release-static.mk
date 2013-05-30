# use user-built static minizip & libpng for better cross-distro compatibility
extraSysroot := $(IMAGINE_PATH)/bundle/linux-x86
staticLibcxx := 1
include $(IMAGINE_PATH)/make/shortcut/common-builds/linux-x86-release.mk
# compatibilty with Gentoo system zlib when using our static minizip
CPPFLAGS += -DOF\(args\)=args
