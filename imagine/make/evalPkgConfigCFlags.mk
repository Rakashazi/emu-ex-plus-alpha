ifdef pkgConfigDeps
 pkgConfigIncludeDeps := 1
endif
ifdef pkgConfigStaticDeps
 pkgConfigIncludeDeps := 1
endif
ifdef pkgConfigIncludeDeps
 pkgConfigCFlagsOutput := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) PKG_CONFIG_SYSTEM_INCLUDE_PATH=$(PKG_CONFIG_SYSTEM_INCLUDE_PATH) pkg-config $(pkgConfigDeps) $(pkgConfigStaticDeps) --cflags $(pkgConfigOpts))
 CPPFLAGS += $(pkgConfigCFlagsOutput)
endif