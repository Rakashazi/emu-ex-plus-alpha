include $(IMAGINE_PATH)/make/config.mk
-include $(projectPath)/config.mk
include $(IMAGINE_PATH)/make/android-metadata.mk

.PHONY: all
all : android-apk

# SDK level setup

android_minSDK ?= 9
android_baseModuleSDK := 9
android_targetSDK ?= 23

android_soName := main

# Architecture setup

ifndef android_arch
 ifdef android_ouyaBuild
  android_arch := armv7
 else
  android_arch := arm64 armv7 x86
 endif
endif

android_arch := $(filter-out $(android_noArch),$(android_arch))

ifdef android_ouyaBuild
 android_minSDK := 16
 android_buildPrefix := android-ouya
 android_releaseReadySubdir := ouya
else
 android_buildPrefix := android
 android_releaseReadySubdir := android
endif

android_buildName ?= $(firstMakefileName:.mk=)

ifeq ($(filter arm, $(android_arch)),)
 android_noArm := 1
endif
ifeq ($(filter armv7, $(android_arch)),)
 android_noArmv7 := 1
endif
ifeq ($(filter arm64, $(android_arch)),)
 android_noArm64 := 1
endif
ifeq ($(filter x86, $(android_arch)),)
 android_noX86 := 1
endif

android_targetPath := target/$(android_buildName)

ifdef imagineLibExt
 android_makefileOpts += imagineLibExt=$(imagineLibExt)
endif

android_sdkToolsPath := $(shell dirname `which android`)

ifeq ($(android_sdkToolsPath),)
 $(error couldn't find Android SDK Tools path, make sure the "android" executable is in your PATH)
endif

# metadata

android_manifestXml := $(android_targetPath)/AndroidManifest.xml
$(android_manifestXml) : $(projectPath)/metadata/conf.mk $(metadata_confDeps)
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

resPath := $(projectPath)/res
android_resSrcPath := $(resPath)/android

ifneq ($(wildcard $(android_resSrcPath)/assets-$(android_minSDK)),)
 android_assetsSrcPath := $(android_resSrcPath)/assets-$(android_minSDK)
else ifneq ($(wildcard $(android_resSrcPath)/assets),)
 android_assetsSrcPath := $(android_resSrcPath)/assets
endif

ifdef android_ouyaBuild
 ifneq ($(wildcard $(android_resSrcPath)/assets-ouya),)
  android_assetsSrcPath := $(android_resSrcPath)/assets-ouya
 endif
endif

ifdef android_assetsSrcPath
android_assetsPath := $(android_targetPath)/assets
$(android_assetsPath) :
	@mkdir -p $(@D)
	ln -rs $(android_assetsSrcPath) $@
endif

ifneq ($(wildcard $(resPath)/icons/icon-48.png),)
android_drawableMdpiIconPath := $(android_targetPath)/res/drawable-mdpi/icon.png
$(android_drawableMdpiIconPath) :
	@mkdir -p $(@D)
	ln -rs $(resPath)/icons/icon-48.png $@
endif

ifneq ($(wildcard $(resPath)/icons/icon-72.png),)
android_drawableHdpiIconPath := $(android_targetPath)/res/drawable-hdpi/icon.png
$(android_drawableHdpiIconPath) :
	@mkdir -p $(@D)
	ln -rs $(resPath)/icons/icon-72.png $@
endif

ifneq ($(wildcard $(resPath)/icons/icon-96.png),)
android_drawableXhdpiIconPath := $(android_targetPath)/res/drawable-xhdpi/icon.png
$(android_drawableXhdpiIconPath) :
	@mkdir -p $(@D)
	ln -rs $(resPath)/icons/icon-96.png $@
endif

ifneq ($(wildcard $(resPath)/icons/icon-144.png),)
android_drawableXxhdpiIconPath := $(android_targetPath)/res/drawable-xxhdpi/icon.xml
# "iconbig" used by Xperia Play launcher, links to xxhdpi icon
$(android_drawableXxhdpiIconPath) :
	@mkdir -p $(@D) $(android_targetPath)/res/drawable-mdpi/
	ln -rs $(resPath)/icons/icon-144.png $(android_targetPath)/res/drawable-mdpi/icon144.png
	printf '<?xml version="1.0" encoding="utf-8"?>\n<bitmap xmlns:android="http://schemas.android.com/apk/res/android" android:src="@drawable/icon144" />\n' > $@
	cp $@ $(android_targetPath)/res/drawable-mdpi/iconbig.xml
endif

ifneq ($(wildcard $(resPath)/icons/icon-192.png),)
android_drawableXxxhdpiIconPath := $(android_targetPath)/res/drawable-xxxhdpi/icon.png
$(android_drawableXxxhdpiIconPath) :
	@mkdir -p $(@D)
	ln -rs $(resPath)/icons/icon-192.png $@
endif

android_drawableIconPaths := $(android_drawableMdpiIconPath) $(android_drawableHdpiIconPath) \
 $(android_drawableXhdpiIconPath) $(android_drawableXxhdpiIconPath) $(android_drawableXxxhdpiIconPath)

ifdef android_ouyaBuild
 android_drawableXhdpiOuyaIconPath := $(android_targetPath)/res/drawable-xhdpi/ouya_icon.png
 $(android_drawableXhdpiOuyaIconPath) :
	@mkdir -p $(@D)
	ln -rs $(resPath)/icons/ouya_icon.png $@
 android_drawableIconPaths := $(android_drawableXhdpiIconPath) $(android_drawableXhdpiOuyaIconPath)
else
 ifneq ($(wildcard $(resPath)/icons/tv-banner.png),)
  android_drawableTVBannerPath := $(android_targetPath)/res/drawable-xhdpi/banner.png
  android_drawableIconPaths += $(android_drawableTVBannerPath)
  android_gen_metadata_args += --tv
 endif
endif

ifdef android_drawableTVBannerPath
$(android_drawableTVBannerPath) :
	@mkdir -p $(@D)
	ln -rs $(resPath)/icons/tv-banner.png $@
endif

android_imagineLib9SrcPath := $(IMAGINE_SDK_PATH)/android-java/imagine-v9.jar
android_imagineLib9 := $(android_targetPath)/libs/imagine-v9.jar

$(android_imagineLib9) : | $(android_imagineLib9SrcPath)
	@mkdir -p $(@D)
	ln -s $| $@
	mkdir -p $(android_targetPath)/src

ifeq ($(wildcard $(android_imagineLib9SrcPath)),)
 $(error couldn't find $(android_imagineLib9SrcPath), make sure you've built and installed it with android-java.mk) 
endif

android_supportLib4SrcPath := $(android_sdkToolsPath)/../extras/android/support/v4/android-support-v4.jar
android_supportLib4 := $(android_targetPath)/libs/android-support-v4.jar

$(android_supportLib4) : | $(android_supportLib4SrcPath)
	@mkdir -p $(@D)
	ln -s $| $@

android_styles21Xml := $(android_targetPath)/res/values-v21/styles.xml

$(android_styles21Xml) :
	@mkdir -p $(@D)
	printf '<?xml version="1.0" encoding="utf-8"?>\n<resources>\n\t<style name="AppTheme" parent="android:Theme.Material.NoActionBar.TranslucentDecor"/>\n</resources>\n' > $@

android_stylesXmlFiles += $(android_styles21Xml)

ifeq ($(shell expr $(android_minSDK) \< 21), 1)

android_styles19Xml := $(android_targetPath)/res/values-v19/styles.xml

$(android_styles19Xml) :
	@mkdir -p $(@D)
	printf '<?xml version="1.0" encoding="utf-8"?>\n<resources>\n\t<style name="AppTheme" parent="android:Theme.Holo.NoActionBar.TranslucentDecor"/>\n</resources>\n' > $@

android_stylesXmlFiles += $(android_styles19Xml)

endif

ifeq ($(shell expr $(android_minSDK) \< 19), 1)

android_styles11Xml := $(android_targetPath)/res/values-v11/styles.xml

$(android_styles11Xml) :
	@mkdir -p $(@D)
	printf '<?xml version="1.0" encoding="utf-8"?>\n<resources>\n\t<style name="AppTheme" parent="android:Theme.Holo.NoActionBar"/>\n</resources>\n' > $@

android_stylesXmlFiles += $(android_styles11Xml)

endif

ifeq ($(shell expr $(android_minSDK) \< 11), 1)

android_stylesXml := $(android_targetPath)/res/values/styles.xml

$(android_stylesXml) :
	@mkdir -p $(@D)
	printf '<?xml version="1.0" encoding="utf-8"?>\n<resources>\n\t<style name="AppTheme" parent="android:Theme.NoTitleBar"/>\n</resources>\n' > $@

android_stylesXmlFiles += $(android_stylesXml)

endif

android_stringsXml := $(android_targetPath)/res/values/strings.xml

$(android_stringsXml) : $(projectPath)/metadata/conf.mk
	@mkdir -p $(@D)
	printf '<?xml version="1.0" encoding="utf-8"?>\n<resources>\n\t<string name="app_name">$(android_metadata_name)</string>\n</resources>\n' > $@

android_buildXml := $(android_targetPath)/build.xml

$(android_buildXml) : | $(android_manifestXml) $(android_stringsXml) $(android_imagineLib9) \
$(android_drawableIconPaths) $(android_assetsPath) $(android_antProperties) $(android_stylesXmlFiles) $(android_supportLib4)
	android update project -p $(@D) -n $(android_metadata_project) -t android-$(android_targetSDK)
	rm $(@D)/proguard-project.txt

ifneq ($(wildcard $(android_resSrcPath)/proguard.cfg),)
android_proguardConfPath := $(android_targetPath)/proguard.cfg
$(android_proguardConfPath) : | $(android_buildXml)
	@mkdir -p $(@D)
	ln -rs $(android_resSrcPath)/proguard.cfg $@
endif

# native libs

ifndef android_noArm

android_armMakefile ?= $(IMAGINE_PATH)/make/shortcut/common-builds/$(android_buildPrefix)-arm.mk
android_armSODir := $(android_targetPath)/libs/armeabi
android_armSO := $(android_armSODir)/lib$(android_soName).so
android_armMakeArgs = -f $(android_armMakefile) $(android_makefileOpts) \
 targetDir=$(android_armSODir) buildName=$(android_buildName)-arm \
 projectPath=$(projectPath)
.PHONY: android-arm
android-arm :
	@echo "Building ARM Shared Object"
	$(PRINT_CMD)$(MAKE) $(android_armMakeArgs)
$(android_armSO) : android-arm

.PHONY: android-arm-clean
android-arm-clean :
	@echo "Cleaning ARM Build"
	$(PRINT_CMD)$(MAKE) $(android_armMakeArgs) clean
android_cleanTargets += android-arm-clean
android_soFiles += $(android_armSO)

endif

ifndef android_noArmv7

android_armv7Makefile ?= $(IMAGINE_PATH)/make/shortcut/common-builds/$(android_buildPrefix)-armv7.mk
android_armv7SODir := $(android_targetPath)/libs/armeabi-v7a
android_armv7SO := $(android_armv7SODir)/lib$(android_soName).so
android_armv7MakeArgs = -f $(android_armv7Makefile) $(android_makefileOpts) \
 targetDir=$(android_armv7SODir) buildName=$(android_buildName)-armv7 \
 projectPath=$(projectPath)
.PHONY: android-armv7
android-armv7 :
	@echo "Building ARMv7 Shared Object"
	$(PRINT_CMD)$(MAKE) $(android_armv7MakeArgs)
$(android_armv7SO) : android-armv7

.PHONY: android-armv7-clean
android-armv7-clean :
	@echo "Cleaning ARMv7 Build"
	$(PRINT_CMD)$(MAKE) $(android_armv7MakeArgs) clean
android_cleanTargets += android-armv7-clean
android_soFiles += $(android_armv7SO)

endif

ifndef android_noArm64

android_arm64Makefile ?= $(IMAGINE_PATH)/make/shortcut/common-builds/$(android_buildPrefix)-arm64.mk
android_arm64SODir := $(android_targetPath)/libs/arm64-v8a
android_arm64SO := $(android_arm64SODir)/lib$(android_soName).so
android_arm64MakeArgs = -f $(android_arm64Makefile) $(android_makefileOpts) \
 targetDir=$(android_arm64SODir) buildName=$(android_buildName)-arm64 \
 projectPath=$(projectPath)
.PHONY: android-arm64
android-arm64 :
	@echo "Building ARM64 Shared Object"
	$(PRINT_CMD)$(MAKE) $(android_arm64MakeArgs)
$(android_arm64SO) : android-arm64

.PHONY: android-arm64-clean
android-arm64-clean :
	@echo "Cleaning ARM64 Build"
	$(PRINT_CMD)$(MAKE) $(android_arm64MakeArgs) clean
android_cleanTargets += android-arm64-clean
android_soFiles += $(android_arm64SO)

endif

ifndef android_noX86

android_x86Makefile ?= $(IMAGINE_PATH)/make/shortcut/common-builds/$(android_buildPrefix)-x86.mk
android_x86SODir := $(android_targetPath)/libs/x86
android_x86SO := $(android_x86SODir)/lib$(android_soName).so
android_x86MakeArgs = -f $(android_x86Makefile) $(android_makefileOpts) \
 targetDir=$(android_x86SODir) buildName=$(android_buildName)-x86 \
 projectPath=$(projectPath)
.PHONY: android-x86
android-x86 :
	@echo "Building X86 Shared Object"
	$(PRINT_CMD)$(MAKE) $(android_x86MakeArgs)
$(android_x86SO) : android-x86

.PHONY: android-x86-clean
android-x86-clean :
	@echo "Cleaning X86 Build"
	$(PRINT_CMD)$(MAKE) $(android_x86MakeArgs) clean
android_cleanTargets += android-x86-clean
android_soFiles += $(android_x86SO)

endif

.PHONY: android-build
android-build : $(android_soFiles)

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
android-apk : $(android_projectDeps) $(android_soFiles)
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

.PHONY: android-clean-project
android-clean-project:
	rm -rf $(android_targetPath)
android_cleanTargets += android-clean-project

.PHONY: android-clean
android-clean: $(android_cleanTargets)
