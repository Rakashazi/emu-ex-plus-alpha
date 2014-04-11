include $(IMAGINE_PATH)/make/config.mk
# use user-built static minizip & libpng for better cross-distro compatibility
staticLibcxx := 1
libpngStatic := 1
minizipStatic := 1
include $(IMAGINE_PATH)/make/shortcut/common-builds/linux-x86_64-release.mk
# compatibilty with Gentoo system zlib when using our static minizip
CPPFLAGS += -DOF\(args\)=args
