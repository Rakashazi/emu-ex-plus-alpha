include $(IMAGINE_PATH)/make/config.mk
-include $(projectPath)/config.mk
include $(buildSysPath)/android-metadata.mk
include $(buildSysPath)/setAndroidNDKPath.mk

.PHONY: all
all : android-bundle

BUNDLETOOL_PATH ?= $(IMAGINE_PATH)/tools/bundletool-all-1.15.6.jar
BUNDLETOOL := java -jar $(BUNDLETOOL_PATH)

# Code signing parameters used when generating APKs from the app bundle
android_gradlePropertiesPath = $(HOME)/.gradle/gradle.properties
-include $(android_gradlePropertiesPath)
ifdef ANDROID_KEY_STORE
 keySignParams := --ks-key-alias=$(ANDROID_KEY_ALIAS) --key-pass=pass:$(ANDROID_KEY_PASSWORD) --ks=$(ANDROID_KEY_STORE) --ks-pass=pass:$(ANDROID_KEY_STORE_PASSWORD)
endif

ifdef android_deviceSerial
 bundletoolArgs += --device-id=$(android_deviceSerial)
endif

# SDK level setup

android_minSDK ?= 9

android_metadata_soName ?= main
android_makefileOpts += android_metadata_soName=$(android_metadata_soName)

# Architecture setup

ifndef android_arch
 android_arch := armv7 arm64 x86 x86_64
endif

android_arch := $(filter-out $(android_noArch),$(android_arch))

android_buildPrefix := android
android_releaseReadySubdir := android

android_buildName ?= $(firstMakefileName:.mk=)

ifeq ($(filter armv7, $(android_arch)),)
 android_noArmv7 := 1
endif
ifeq ($(filter arm64, $(android_arch)),)
 android_noArm64 := 1
endif
ifeq ($(filter x86, $(android_arch)),)
 android_noX86 := 1
endif
ifeq ($(filter x86_64, $(android_arch)),)
 android_noX86_64 := 1
endif

android_targetPath := target/$(android_buildName)
android_resPath := $(android_targetPath)/src/main/res

ifdef imagineLibExt
 android_makefileOpts += imagineLibExt=$(imagineLibExt)
endif

xmlDecl := <?xml version="1.0" encoding="utf-8"?>

# metadata

android_manifestXml := $(android_targetPath)/src/main/AndroidManifest.xml
$(android_manifestXml) : $(projectPath)/metadata/conf.mk $(metadata_confDeps)
	@mkdir -p $(@D)
	bash $(IMAGINE_PATH)/tools/genAndroidMeta.sh $(android_gen_metadata_args) $@
.PHONY: android-metadata
android-metadata : $(android_manifestXml)

# project dir

ifdef LTO_MODE
 android_makefileOpts += LTO_MODE=$(LTO_MODE)
endif

resPath := $(projectPath)/res
android_resSrcPath := $(resPath)/android

ifneq ($(wildcard $(android_resSrcPath)/assets-$(android_minSDK)),)
 android_assetsSrcPath := $(android_resSrcPath)/assets-$(android_minSDK)
else ifneq ($(wildcard $(android_resSrcPath)/assets),)
 android_assetsSrcPath := $(android_resSrcPath)/assets
endif

ifdef android_assetsSrcPath
android_assetsPath := $(android_targetPath)/src/main/assets
$(android_assetsPath) :
	@mkdir -p $(@D)
	ln -rs $(android_assetsSrcPath) $@
endif

ifneq ($(wildcard $(resPath)/icons/icon-48.png),)
android_drawableMdpiIconPath := $(android_resPath)/mipmap-mdpi/icon.png
$(android_drawableMdpiIconPath) :
	@mkdir -p $(@D)
	ln -rs $(resPath)/icons/icon-48.png $@
endif

ifneq ($(wildcard $(resPath)/icons/icon-72.png),)
android_drawableHdpiIconPath := $(android_resPath)/mipmap-hdpi/icon.png
$(android_drawableHdpiIconPath) :
	@mkdir -p $(@D)
	ln -rs $(resPath)/icons/icon-72.png $@
endif

ifneq ($(wildcard $(resPath)/icons/icon-96.png),)
android_drawableXhdpiIconPath := $(android_resPath)/mipmap-xhdpi/icon.png
$(android_drawableXhdpiIconPath) :
	@mkdir -p $(@D)
	ln -rs $(resPath)/icons/icon-96.png $@
endif

ifneq ($(wildcard $(resPath)/icons/icon-144.png),)
android_drawableXxhdpiIconPath := $(android_resPath)/mipmap-xxhdpi/icon.png
$(android_drawableXxhdpiIconPath) :
	@mkdir -p $(@D)
	ln -rs $(resPath)/icons/icon-144.png $@

# "iconbig" used by Xperia Play launcher, links to xxhdpi icon
android_drawableMdpiBigIconPath := $(android_resPath)/drawable-mdpi/iconbig.png
$(android_drawableMdpiBigIconPath) :
	@mkdir -p $(@D)
	ln -rs $(resPath)/icons/icon-144.png $@
endif

ifneq ($(wildcard $(resPath)/icons/icon-192.png),)
android_drawableXxxhdpiIconPath := $(android_resPath)/mipmap-xxxhdpi/icon.png
$(android_drawableXxxhdpiIconPath) :
	@mkdir -p $(@D)
	ln -rs $(resPath)/icons/icon-192.png $@
endif

ifdef android_metadata_adaptiveIconIsMonochrome
 androidMonochromeIcon := icon_fg
else
 androidMonochromeIcon := icon_monochrome
endif

ifneq ($(wildcard $(resPath)/icons/adaptive-icon-bg.webp),)
android_drawableIconV26Path := $(android_resPath)/mipmap-anydpi-v26/icon.xml
$(android_drawableIconV26Path) :
	@mkdir -p $(@D)
	printf '$(xmlDecl)\n<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">\n\t<background android:drawable="@mipmap/icon_bg" />\n\t<foreground android:drawable="@mipmap/icon_fg" />\n\t<monochrome android:drawable="@mipmap/$(androidMonochromeIcon)" />\n</adaptive-icon>' > $@

android_drawableIconBgPath := $(android_resPath)/mipmap-xhdpi/icon_bg.webp
$(android_drawableIconBgPath) :
	@mkdir -p $(@D)
	ln -rs $(resPath)/icons/adaptive-icon-bg.webp $@

android_mipmapXhdpiIconFgPath := $(android_resPath)/mipmap-xhdpi/icon_fg.webp
$(android_mipmapXhdpiIconFgPath) :
	@mkdir -p $(@D)
	ln -rs $(resPath)/icons/adaptive-icon-fg-216.webp $@

android_mipmapXxhdpiIconFgPath := $(android_resPath)/mipmap-xxhdpi/icon_fg.webp
$(android_mipmapXxhdpiIconFgPath) :
	@mkdir -p $(@D)
	ln -rs $(resPath)/icons/adaptive-icon-fg-324.webp $@

android_mipmapXxxhdpiIconFgPath := $(android_resPath)/mipmap-xxxhdpi/icon_fg.webp
$(android_mipmapXxxhdpiIconFgPath) :
	@mkdir -p $(@D)
	ln -rs $(resPath)/icons/adaptive-icon-fg-432.webp $@
endif

ifneq ($(wildcard $(resPath)/icons/monochrome-icon-432.webp),)
android_mipmapXhdpiIconMonochromePath := $(android_resPath)/mipmap-xhdpi/icon_monochrome.webp
$(android_mipmapXhdpiIconMonochromePath) :
	@mkdir -p $(@D)
	ln -rs $(resPath)/icons/monochrome-icon-216.webp $@

android_mipmapXxhdpiIconMonochromePath := $(android_resPath)/mipmap-xxhdpi/icon_monochrome.webp
$(android_mipmapXxhdpiIconMonochromePath) :
	@mkdir -p $(@D)
	ln -rs $(resPath)/icons/monochrome-icon-324.webp $@

android_mipmapXxxhdpiIconMonochromePath := $(android_resPath)/mipmap-xxxhdpi/icon_monochrome.webp
$(android_mipmapXxxhdpiIconMonochromePath) :
	@mkdir -p $(@D)
	ln -rs $(resPath)/icons/monochrome-icon-432.webp $@
endif

android_drawableIconPaths := $(android_drawableMdpiIconPath) $(android_drawableHdpiIconPath) \
 $(android_drawableXhdpiIconPath) $(android_drawableXxhdpiIconPath) $(android_drawableXxxhdpiIconPath) \
 $(android_drawableIconV26Path) $(android_drawableIconBgPath) $(android_mipmapXhdpiIconFgPath) \
 $(android_mipmapXxhdpiIconFgPath) $(android_mipmapXxxhdpiIconFgPath) $(android_drawableMdpiBigIconPath) \
 $(android_mipmapXhdpiIconMonochromePath) $(android_mipmapXxhdpiIconMonochromePath) $(android_mipmapXxxhdpiIconMonochromePath)

ifneq ($(wildcard $(resPath)/icons/ouya_icon.png),)
 android_drawableXhdpiOuyaIconPath := $(android_resPath)/drawable-xhdpi/ouya_icon.png
 android_drawableIconPaths += $(android_drawableXhdpiOuyaIconPath)
 android_gen_metadata_args += --ouya
endif

ifdef android_drawableXhdpiOuyaIconPath
# Ouya icon needs an explicit xml reference to prevent build system from stripping it since it's not used in AndroidManifest.xml
$(android_drawableXhdpiOuyaIconPath) :
	@mkdir -p $(@D)
	ln -rs $(resPath)/icons/ouya_icon.png $@
	printf '$(xmlDecl)\n<resources xmlns:tools="http://schemas.android.com/tools" tools:keep="@drawable/ouya_icon"/>\n' > $(@D)/ouya.xml
endif

ifneq ($(wildcard $(resPath)/icons/tv-banner.webp),)
 android_drawableTVBannerPath := $(android_resPath)/drawable-v21/banner.webp
 android_drawableIconPaths += $(android_drawableTVBannerPath)
 android_gen_metadata_args += --tv
endif

ifdef android_drawableTVBannerPath
$(android_drawableTVBannerPath) :
	@mkdir -p $(@D)
	ln -rs $(resPath)/icons/tv-banner.webp $@
endif

android_imagineLib9SrcPath := $(IMAGINE_SDK_PATH)/android-java/imagine-v9.aar
android_imagineLib9 := $(android_targetPath)/libs/imagine-v9.aar

$(android_imagineLib9) : | $(android_imagineLib9SrcPath)
	@mkdir -p $(@D)
	ln -s $| $@
	mkdir -p $(android_targetPath)/src

ifeq ($(wildcard $(android_imagineLib9SrcPath)),)
 $(error couldn't find $(android_imagineLib9SrcPath), make sure you've built and installed it with android-java.mk) 
endif

android_styles21Xml := $(android_resPath)/values-v21/styles.xml

android_MaterialTheme := android:Theme.Material.NoActionBar.TranslucentDecor
android_HoloTheme := android:Theme.Holo.NoActionBar.TranslucentDecor
android_LegacyHoloTheme := android:Theme.Holo.NoActionBar
android_LegacyTheme := android:Theme.NoTitleBar
android_ShortEdgesItem := <item name="android:windowLayoutInDisplayCutoutMode">shortEdges</item>

$(android_styles21Xml) :
	@mkdir -p $(@D)
	printf '$(xmlDecl)\n<resources>\n\t<style name="AppTheme" parent="$(android_MaterialTheme)">\n\t\t$(android_ShortEdgesItem)\n\t</style>\n</resources>\n' > $@

android_stylesXmlFiles += $(android_styles21Xml)

ifeq ($(shell expr $(android_minSDK) \< 21), 1)

android_styles19Xml := $(android_resPath)/values-v19/styles.xml

$(android_styles19Xml) :
	@mkdir -p $(@D)
	printf '$(xmlDecl)\n<resources>\n\t<style name="AppTheme" parent="$(android_HoloTheme)"/>\n</resources>\n' > $@

android_stylesXmlFiles += $(android_styles19Xml)

endif

ifeq ($(shell expr $(android_minSDK) \< 19), 1)

android_styles11Xml := $(android_resPath)/values-v11/styles.xml

$(android_styles11Xml) :
	@mkdir -p $(@D)
	printf '$(xmlDecl)\n<resources>\n\t<style name="AppTheme" parent="$(android_LegacyHoloTheme)"/>\n</resources>\n' > $@

android_stylesXmlFiles += $(android_styles11Xml)

endif

ifeq ($(shell expr $(android_minSDK) \< 11), 1)

android_stylesXml := $(android_resPath)/values/styles.xml

$(android_stylesXml) :
	@mkdir -p $(@D)
	printf '$(xmlDecl)\n<resources>\n\t<style name="AppTheme" parent="$(android_LegacyTheme)"/>\n</resources>\n' > $@

android_stylesXmlFiles += $(android_stylesXml)

endif

android_stringsXml := $(android_resPath)/values/strings.xml

$(android_stringsXml) : $(projectPath)/metadata/conf.mk
	@mkdir -p $(@D)
	printf '$(xmlDecl)\n<resources>\n\t<string name="app_name">$(android_metadata_name)</string>\n</resources>\n' > $@

gradleSrcPath = $(IMAGINE_PATH)/make/gradle

android_buildGradle := $(android_targetPath)/build.gradle

$(android_buildGradle) : | $(android_manifestXml) $(android_stringsXml) $(android_imagineLib9) \
$(android_drawableIconPaths) $(android_assetsPath) $(android_stylesXmlFiles)
	cp $(gradleSrcPath)/gradlew $(gradleSrcPath)/app/build.gradle $(gradleSrcPath)/app/gradle.properties $(@D)
	cp -r $(gradleSrcPath)/gradle $(@D)
	echo METADATA_PROJECT=$(android_metadata_project) >> $(@D)/gradle.properties
	echo METADATA_NAMESPACE=$(android_metadata_id) >> $(@D)/gradle.properties
	echo METADATA_MIN_SDK=$(android_minSDK) >> $(@D)/gradle.properties
	echo METADATA_TARGET_SDK=$(android_metadata_target_sdk) >> $(@D)/gradle.properties

ifneq ($(wildcard $(android_resSrcPath)/proguard.cfg),)
android_proguardConfSrcPath = $(android_resSrcPath)/proguard.cfg
else
android_proguardConfSrcPath = $(IMAGINE_PATH)/src/base/android/proguard/imagine.cfg
endif

android_proguardConfPath := $(android_targetPath)/proguard.cfg
$(android_proguardConfPath) : | $(android_buildGradle)
	@mkdir -p $(@D)
	ln -rs $(android_proguardConfSrcPath) $@

# native libs

ifndef android_noArmv7

android_armv7Makefile ?= $(IMAGINE_PATH)/make/shortcut/common-builds/$(android_buildPrefix)-armv7.mk
android_armv7SODir := $(android_targetPath)/src/main/jniLibs/armeabi-v7a
android_armv7SO := $(android_armv7SODir)/lib$(android_metadata_soName).so
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
android_arm64SODir := $(android_targetPath)/src/main/jniLibs/arm64-v8a
android_arm64SO := $(android_arm64SODir)/lib$(android_metadata_soName).so
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
android_x86SODir := $(android_targetPath)/src/main/jniLibs/x86
android_x86SO := $(android_x86SODir)/lib$(android_metadata_soName).so
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

ifndef android_noX86_64

android_x86_64Makefile ?= $(IMAGINE_PATH)/make/shortcut/common-builds/$(android_buildPrefix)-x86_64.mk
android_x86_64SODir := $(android_targetPath)/src/main/jniLibs/x86_64
android_x86_64SO := $(android_x86_64SODir)/lib$(android_metadata_soName).so
android_x86_64MakeArgs = -f $(android_x86_64Makefile) $(android_makefileOpts) \
 targetDir=$(android_x86_64SODir) buildName=$(android_buildName)-x86_64 \
 projectPath=$(projectPath)
.PHONY: android-x86_64
android-x86_64 :
	@echo "Building X86_64 Shared Object"
	$(PRINT_CMD)$(MAKE) $(android_x86_64MakeArgs)
$(android_x86_64SO) : android-x86_64

.PHONY: android-x86_64-clean
android-x86_64-clean :
	@echo "Cleaning X86_64 Build"
	$(PRINT_CMD)$(MAKE) $(android_x86_64MakeArgs) clean
android_cleanTargets += android-x86_64-clean
android_soFiles += $(android_x86_64SO)

endif

.PHONY: android-build
android-build : $(android_soFiles)

# apks

ifdef V
 antVerbose := -verbose
endif

android_projectDeps := $(android_buildGradle) $(android_proguardConfPath)

ifeq ($(android_buildTarget),debug)
 android_buildTask := assembleDebug
 android_bundleTask := bundleDebug
 android_installTask := installDebug
else
 android_buildTarget := release
 android_buildTask := assembleRelease
 android_bundleTask := bundleRelease
 android_installTask := installRelease
endif

android_bundlePath := $(android_targetPath)/build/outputs/bundle/$(android_buildTarget)/$(android_metadata_project)-$(android_buildTarget).aab
.PHONY: android-bundle
$(android_bundlePath) : $(android_projectDeps) android-build
	cd $(android_targetPath) && ./gradlew -Dimagine.path=$(IMAGINE_PATH) $(android_bundleTask)
android-bundle : $(android_bundlePath)

android_bundleApksPath := $(android_targetPath)/build/outputs/bundle/$(android_buildTarget)/$(android_metadata_project).apks
.PHONY: android-bundle-apks
$(android_bundleApksPath) : $(android_bundlePath)
	$(BUNDLETOOL) build-apks --bundle="$(android_bundlePath)" --output=$(android_bundleApksPath) --overwrite $(keySignParams)
android-bundle-apks : $(android_bundleApksPath)

.PHONY: android-bundle-install
android-bundle-install : $(android_bundleApksPath)
	$(BUNDLETOOL) install-apks --apks=$(android_bundleApksPath) --allow-downgrade $(bundletoolArgs)

.PHONY: android-bundle-ready
android-bundle-ready :
	cp $(android_bundlePath) $(IMAGINE_PATH)/../releases-bin/$(android_releaseReadySubdir)/$(android_metadata_project)-$(android_metadata_version).aab

android_apkPath := $(android_targetPath)/build/outputs/apk/$(android_buildTarget)/$(android_metadata_project)-$(android_buildTarget).apk
.PHONY: android-apk
android-apk : $(android_projectDeps) $(android_soFiles)
	cd $(android_targetPath) && ./gradlew -Dimagine.path=$(IMAGINE_PATH) $(android_buildTask)
$(android_apkPath) : android-apk

.PHONY: android-install
android-install : $(android_projectDeps) $(android_soFiles)
	cd $(android_targetPath) && ./gradlew -Dimagine.path=$(IMAGINE_PATH) $(android_installTask)

.PHONY: android-install-only
android-install-only : $(android_projectDeps)
	cd $(android_targetPath) && ./gradlew -Dimagine.path=$(IMAGINE_PATH) $(android_installTask)

.PHONY: android-ready
android-ready : 
	cp $(android_apkPath) $(IMAGINE_PATH)/../releases-bin/$(android_releaseReadySubdir)/$(android_metadata_project)-$(android_minSDK)-$(android_metadata_version).apk

.PHONY: android-check
android-check :
	@echo "Checking compiled version of $(android_metadata_project) (SDK $(android_minSDK)) $(android_metadata_version)"
	@strings $(android_targetPath)/src/main/jniLibs/*/*.so | grep " $(android_metadata_version)"

.PHONY: android-clean-project
android-clean-project:
	rm -rf $(android_targetPath)
android_cleanTargets += android-clean-project

.PHONY: clean
clean: $(android_cleanTargets)

.DEFAULT:
ifndef android_noArmv7
	$(PRINT_CMD)$(MAKE) $(android_armv7MakeArgs) $@
endif
ifndef android_noArm64
	$(PRINT_CMD)$(MAKE) $(android_arm64MakeArgs) $@
endif
ifndef android_noX86
	$(PRINT_CMD)$(MAKE) $(android_x86MakeArgs) $@
endif
ifndef android_noX86_64
	$(PRINT_CMD)$(MAKE) $(android_x86_64MakeArgs) $@
endif
