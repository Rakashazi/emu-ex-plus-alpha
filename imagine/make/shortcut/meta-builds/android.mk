include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))../../config.mk
include $(IMAGINE_PATH)/make/android-metadata.mk

.PHONY: all
all : android-apk

# SDK level setup

android_minSDK ?= 9

android_hasSDK5 := $(shell expr $(android_minSDK) \>= 5)
android_hasSDK9 := $(shell expr $(android_minSDK) \>= 9)

ifeq ($(android_hasSDK9), 1)
 android_baseModuleSDK := 9
else ifeq ($(android_hasSDK5), 1)
 android_baseModuleSDK := 5
else
 android_baseModuleSDK := 4
endif

ifndef android_targetSDK
 android_targetSDK := $(android_minSDK)
 ifeq ($(android_minSDK), 5)
  android_targetSDK := 8
 endif
 ifeq ($(android_hasSDK9), 1)
  android_targetSDK := 16
 endif
endif

ifeq ($(android_hasSDK9), 1)
 android_soName := main
else
 android_soName := imagine
endif

# Architecture setup

ifndef android_arch
 ifdef android_ouyaBuild
  android_arch := armv7
 else
  ifeq ($(android_minSDK), 4)
   # only build ARM for older Android OS
   android_arch := arm
  else ifeq ($(android_minSDK), 5)
   android_arch := arm armv7
  else
   # also build X86 on Android 2.3+
   android_arch := arm armv7 x86
  endif
 endif
endif

android_arch := $(filter-out $(android_noArch),$(android_arch))

ifdef android_ouyaBuild
 android_minSDK := 16
 android_buildPrefix := android-ouya
 android_imagineLibPathARMv7 ?= $(IMAGINE_PATH)/lib/android-ouya-16-armv7
 android_imagineIncludePathARMv7 ?= $(IMAGINE_PATH)/build/android-ouya-16-armv7/gen
 android_releaseReadySubdir := ouya
else
 android_buildPrefix := android
 android_imagineLibPathARM ?= $(IMAGINE_PATH)/lib/android-$(android_minSDK)-armv6
 android_imagineIncludePathARM ?= $(IMAGINE_PATH)/build/android-$(android_minSDK)-armv6/gen
 android_imagineLibPathARMv7 ?= $(IMAGINE_PATH)/lib/android-$(android_minSDK)-armv7
 android_imagineIncludePathARMv7 ?= $(IMAGINE_PATH)/build/android-$(android_minSDK)-armv7/gen
 android_imagineLibPathX86 ?= $(IMAGINE_PATH)/lib/android-$(android_minSDK)-x86
 android_imagineIncludePathX86 ?= $(IMAGINE_PATH)/build/android-$(android_minSDK)-x86/gen
 android_releaseReadySubdir := android
endif

# append the minimum SDK level
android_buildPrefix := $(android_buildPrefix)-$(android_minSDK)

android_buildName ?= $(baseMakefileName:.mk=)

ifeq ($(filter arm, $(android_arch)),)
 android_noArm := 1
endif
ifeq ($(filter armv7, $(android_arch)),)
 android_noArmv7 := 1
endif
ifeq ($(filter x86, $(android_arch)),)
 android_noX86 := 1
endif

android_targetPath := target/$(android_buildName)

# metadata

android_manifestXml := $(android_targetPath)/AndroidManifest.xml
$(android_manifestXml) : metadata/conf.mk $(metadata_confDeps)
	@mkdir -p $(@D)
	bash $(IMAGINE_PATH)/tools/genAndroidMeta.sh $(android_gen_metadata_args) --min-sdk=$(android_minSDK) $@
.PHONY: android-metadata
android-metadata : $(android_manifestXml)

# project dir

# user-specified ant.properties
ifdef ANDROID_ANT_PROPERTIES

android_antProperties := $(android_targetPath)/ant.properties

$(android_antProperties) :
	@mkdir -p $(@D)
	ln -s $(ANDROID_ANT_PROPERTIES) $@

endif

ifneq ($(wildcard res/android/assets-$(android_minSDK)),)
 android_assetsSrcPath := res/android/assets-$(android_minSDK)
else ifneq ($(wildcard res/android/assets),)
 android_assetsSrcPath := res/android/assets
endif

ifdef android_assetsSrcPath
android_assetsPath := $(android_targetPath)/assets
$(android_assetsPath) :
	@mkdir -p $(@D)
	ln -rs $(android_assetsSrcPath) $@
endif

ifneq ($(wildcard res/icons/icon-48.png),)
android_drawableMdpiIconPath := $(android_targetPath)/res/drawable-mdpi/icon.png
$(android_drawableMdpiIconPath) :
	@mkdir -p $(@D)
	ln -rs res/icons/icon-48.png $@
endif

ifneq ($(wildcard res/icons/icon-72.png),)
android_drawableHdpiIconPath := $(android_targetPath)/res/drawable-hdpi/icon.png
$(android_drawableHdpiIconPath) :
	@mkdir -p $(@D)
	ln -rs res/icons/icon-72.png $@
endif

ifneq ($(wildcard res/icons/icon-96.png),)
android_drawableXhdpiIconPath := $(android_targetPath)/res/drawable-xhdpi/icon.png
$(android_drawableXhdpiIconPath) :
	@mkdir -p $(@D)
	ln -rs res/icons/icon-96.png $@
endif

ifneq ($(wildcard res/icons/icon-144.png),)
android_drawableXxhdpiIconPath := $(android_targetPath)/res/drawable-xxhdpi/icon.xml
# "iconbig" used by Xperia Play launcher, links to xxhdpi icon
$(android_drawableXxhdpiIconPath) :
	@mkdir -p $(@D) $(android_targetPath)/res/drawable/
	ln -rs res/icons/icon-144.png $(android_targetPath)/res/drawable/icon144.png
	echo -e "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<bitmap xmlns:android=\"http://schemas.android.com/apk/res/android\" android:src=\"@drawable/icon144\" />" > $@
	cp $@ $(android_targetPath)/res/drawable/iconbig.xml
endif

android_drawableIconPaths := $(android_drawableMdpiIconPath) $(android_drawableHdpiIconPath)
ifeq ($(android_hasSDK9), 1)
 android_drawableIconPaths += $(android_drawableXhdpiIconPath) $(android_drawableXxhdpiIconPath)
endif

ifdef android_ouyaBuild
 android_drawableXhdpiOuyaIconPath := $(android_targetPath)/res/drawable-xhdpi/ouya_icon.png
 $(android_drawableXhdpiOuyaIconPath) :
	@mkdir -p $(@D)
	ln -rs res/icons/ouya_icon.png $@
 android_drawableIconPaths := $(android_drawableXhdpiIconPath) $(android_drawableXhdpiOuyaIconPath)
endif

android_imagineJavaSrcPath := $(android_targetPath)/src/com/imagine

$(android_imagineJavaSrcPath) :
	@mkdir -p $(@D)
	ln -s $(IMAGINE_PATH)/src/base/android/java/sdk-$(android_baseModuleSDK)/imagine $@

android_stringsXml := $(android_targetPath)/res/values/strings.xml

$(android_stringsXml) : metadata/conf.mk
	@mkdir -p $(@D)
	echo -e "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<resources>\n\t<string name=\"app_name\">$(android_metadata_name)</string>\n</resources>" > $@

android_buildXml := $(android_targetPath)/build.xml

$(android_buildXml) : | $(android_manifestXml) $(android_stringsXml) $(android_imagineJavaSrcPath) \
$(android_drawableIconPaths) $(android_assetsPath) $(android_antProperties)
	android update project -p $(@D) -n $(android_metadata_project) -t android-$(android_targetSDK)
	rm $(@D)/proguard-project.txt

ifneq ($(wildcard res/android/proguard.cfg),)
android_proguardConfPath := $(android_targetPath)/proguard.cfg
$(android_proguardConfPath) : | $(android_buildXml)
	@mkdir -p $(@D)
	ln -s ../../res/android/proguard.cfg $@
endif

# native libs

ifndef android_noArm

android_armMakefile ?= $(IMAGINE_PATH)/make/shortcut/common-builds/$(android_buildPrefix)-armv6.mk
android_armSODir := $(android_targetPath)/libs/armeabi
android_armSO := $(android_armSODir)/lib$(android_soName).so
.PHONY: android-arm
android-arm :
	@echo "Building ARM Shared Object"
	$(PRINT_CMD)$(MAKE) -f $(android_armMakefile) $(android_makefileOpts) targetDir=$(android_armSODir) buildName=$(android_buildName)-armv6 \
	imagineLibPath=$(android_imagineLibPathARM) imagineIncludePath=$(android_imagineIncludePathARM)
$(android_armSO) : android-arm

endif

ifndef android_noArmv7

android_armv7Makefile ?= $(IMAGINE_PATH)/make/shortcut/common-builds/$(android_buildPrefix)-armv7.mk
android_armv7SODir := $(android_targetPath)/libs/armeabi-v7a
android_armv7SO := $(android_armv7SODir)/lib$(android_soName).so
.PHONY: android-armv7
android-armv7 :
	@echo "Building ARMv7 Shared Object"
	$(PRINT_CMD)$(MAKE) -f $(android_armv7Makefile) $(android_makefileOpts) targetDir=$(android_armv7SODir) buildName=$(android_buildName)-armv7 \
	imagineLibPath=$(android_imagineLibPathARMv7) imagineIncludePath=$(android_imagineIncludePathARMv7)
$(android_armv7SO) : android-armv7

endif

ifndef android_noX86

android_x86Makefile ?= $(IMAGINE_PATH)/make/shortcut/common-builds/$(android_buildPrefix)-x86.mk
android_x86SODir := $(android_targetPath)/libs/x86
android_x86SO := $(android_x86SODir)/lib$(android_soName).so
.PHONY: android-x86
android-x86 :
	@echo "Building X86 Shared Object"
	$(PRINT_CMD)$(MAKE) -f $(android_x86Makefile) $(android_makefileOpts) targetDir=$(android_x86SODir) buildName=$(android_buildName)-x86 \
	imagineLibPath=$(android_imagineLibPathX86) imagineIncludePath=$(android_imagineIncludePathX86)
$(android_x86SO) : android-x86

endif

.PHONY: android-build
android-build : $(android_armSO) $(android_armv7SO) $(android_x86SO)

# apks

ifdef V
 antVerbose := -verbose
endif

android_projectDeps := $(android_buildXml) $(android_proguardConfPath)

ifndef android_antTarget
 android_antTarget := release
endif

android_apkPath := $(android_targetPath)/bin/$(android_metadata_project)-$(android_antTarget).apk
.PHONY: android-apk
android-apk : $(android_projectDeps) $(android_armSO) $(android_armv7SO) $(android_x86SO)
	cd $(android_targetPath) && ANT_OPTS=-Dimagine.path=$(IMAGINE_PATH) ant $(antVerbose) $(android_antTarget)
$(android_apkPath) : android-apk

.PHONY: android-install
android-install : android-apk
	adb install -r $(android_apkPath)

.PHONY: android-install-only
android-install-only :
	adb install -r $(android_apkPath)

.PHONY: android-ready
android-ready : 
	cp $(android_apkPath) $(IMAGINE_PATH)/../releases-bin/$(android_releaseReadySubdir)/$(android_metadata_project)-$(android_minSDK)-$(android_metadata_version).apk

.PHONY: android-check
android-check :
	@echo "Checking compiled version of $(android_metadata_project) (SDK $(android_minSDK)) $(android_metadata_version)"
	@strings $(android_targetPath)/libs/*/*.so | grep " $(android_metadata_version)"

.PHONY: android-clean
android-clean:
	rm -f $(android_armSO) $(android_armv7SO) $(android_x86SO)
	rm -rf build/$(android_buildName)-armv6 build/$(android_buildName)-armv7 build/$(android_buildName)-x86
