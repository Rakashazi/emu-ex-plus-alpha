ifndef inc_pkg_imagine
inc_pkg_imagine := 1

#configIncNext := <config.h>
imagineLibPath ?= $(IMAGINE_PATH)/lib/$(buildName)
imagineStaticLib = $(imagineLibPath)/libimagine.a
PKG_CONFIG_PATH := $(PKG_CONFIG_PATH):$(imagineLibPath)
pkgConfigDeps += imagine

endif