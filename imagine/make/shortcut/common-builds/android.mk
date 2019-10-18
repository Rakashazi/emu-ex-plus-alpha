include $(IMAGINE_PATH)/make/config.mk

ifndef android_arch
 ifdef android_ouyaBuild
  android_arch := armv7
 else
  android_arch := arm64 armv7 x86 x86_64
 endif
endif

android_buildPrefix := android

ifdef android_ouyaBuild
 android_buildPrefix := android-ouya
endif

ifneq ($(filter armv7, $(android_arch)),)
 armv7Target := $(android_buildPrefix)-armv7$(targetExt)
 targets += $(armv7Target)
endif
ifneq ($(filter arm64, $(android_arch)),)
 arm64Target := $(android_buildPrefix)-arm64$(targetExt)
 targets += $(arm64Target)
endif
ifneq ($(filter x86, $(android_arch)),)
 x86Target := $(android_buildPrefix)-x86$(targetExt)
 targets += $(x86Target)
endif
ifneq ($(filter x86_64, $(android_arch)),)
 x86_64Target := $(android_buildPrefix)-x86_64$(targetExt)
 targets += $(x86_64Target)
endif

makefileOpts += projectPath=$(projectPath)

ifdef imagineLibExt
 makefileOpts += imagineLibExt=$(imagineLibExt)
endif

commonBuildPath := $(buildSysPath)/shortcut/common-builds

.PHONY: all

makeTargets = $(foreach target, $(1),$(MAKE) -f $(commonBuildPath)/$(target).mk $(makefileOpts) $(2);)

all :
	$(PRINT_CMD)+$(call makeTargets,$(targets),)

.DEFAULT:
	$(PRINT_CMD)+$(call makeTargets,$(targets),$@)
