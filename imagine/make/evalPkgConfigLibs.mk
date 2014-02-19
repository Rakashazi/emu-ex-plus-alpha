ifdef pkgConfigDeps
 pkgConfigLibsOutput := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) PKG_CONFIG_SYSTEM_LIBRARY_PATH=$(PKG_CONFIG_SYSTEM_LIBRARY_PATH) pkg-config $(pkgConfigDeps) --libs $(pkgConfigOpts))
 LDLIBS += $(pkgConfigLibsOutput)
endif

ifdef pkgConfigStaticDeps
 pkgConfigStaticLibsOutput := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) PKG_CONFIG_SYSTEM_LIBRARY_PATH=$(PKG_CONFIG_SYSTEM_LIBRARY_PATH) pkg-config $(pkgConfigStaticDeps) --libs --static $(pkgConfigOpts))
 LDLIBS += $(pkgConfigStaticLibsOutput)
endif

ifeq ($(android_hardFP),1)
 # make sure soft-float libm isn't linked
 LDLIBS := $(LDLIBS:-lm=-lm_hard)
endif