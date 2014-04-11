pkgConfigEnv := PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) PKG_CONFIG_SYSTEM_LIBRARY_PATH=$(PKG_CONFIG_SYSTEM_LIBRARY_PATH)

ifdef pkgConfigDeps
 pkgConfigLibsOutput := $(shell $(pkgConfigEnv) pkg-config $(pkgConfigDeps) --libs $(pkgConfigOpts))
 LDLIBS += $(pkgConfigLibsOutput)
 pkgConfigLibsOnlyOutput := $(shell $(pkgConfigEnv) pkg-config $(pkgConfigDeps) --libs-only-l $(pkgConfigOpts))
 linkerLibs += $(pkgConfigLibsOnlyOutput)
 pkgConfigLibPathOnlyOutput := $(shell $(pkgConfigEnv) pkg-config $(pkgConfigDeps) --libs-only-L $(pkgConfigOpts))
 ifneq ($(pkgConfigLibPathOnlyOutput),)
  VPATH += $(pkgConfigLibPathOnlyOutput:-L%=%)
 endif
endif

ifdef pkgConfigStaticDeps
 pkgConfigStaticLibsOutput := $(shell $(pkgConfigEnv) pkg-config $(pkgConfigStaticDeps) --libs --static $(pkgConfigOpts))
 LDLIBS += $(pkgConfigStaticLibsOutput)
 pkgConfigStaticLibsOnlyOutput := $(shell $(pkgConfigEnv) pkg-config $(pkgConfigStaticDeps) --libs-only-l --static $(pkgConfigOpts))
 linkerLibs += $(pkgConfigStaticLibsOnlyOutput)
 pkgConfigStaticLibPathOnlyOutput := $(shell $(pkgConfigEnv) pkg-config $(pkgConfigStaticDeps) --libs-only-L --static $(pkgConfigOpts))
 ifneq ($(pkgConfigStaticLibPathOnlyOutput),)
  VPATH += $(pkgConfigStaticLibPathOnlyOutput:-L%=%)
 endif
endif

ifeq ($(android_hardFP),1)
 # make sure soft-float libm isn't linked
 LDLIBS := $(LDLIBS:-lm=-lm_hard)
 linkerLibs := $(linkerLibs:-lm=-lm_hard)
endif

ifdef PKG_CONFIG_SYSTEM_LIBRARY_PATH
 VPATH += $(PKG_CONFIG_SYSTEM_LIBRARY_PATH)
endif
