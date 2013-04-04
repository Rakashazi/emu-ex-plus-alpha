include $(IMAGINE_PATH)/make/android-metadata.mk

ifdef android_ouyaBuild
 android_minSDK := 16
 android_noArmv6 := 1
 android_noX86 := 1
 targetPrefix := android-ouya
else
 targetPrefix := android
endif

ifndef android_minSDK 
 android_minSDK := 9
endif

ifeq ($(android_minSDK), 4)
 # only build ARMv6 for older Android OS
 android_noArmv7=1
 android_noX86=1
endif

ifeq ($(android_minSDK), 5)
 # only build X86 on Android 2.3+
 android_noX86=1
endif

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

android_targetPath := target/$(targetPrefix)-$(android_minSDK)

# metadata

android_manifestXml := $(android_targetPath)/AndroidManifest.xml
$(android_manifestXml) : metadata/conf.mk $(metadata_confDeps)
	@mkdir -p $(@D)
	bash $(IMAGINE_PATH)/tools/genAndroidMeta.sh $(android_gen_metadata_args) --min-sdk=$(android_minSDK) $@
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
	ln -s ../../$(android_assetsSrcPath) $@
endif

ifneq ($(wildcard res/icons/icon-48.png),)
android_drawableMdpiIconPath := $(android_targetPath)/res/drawable-mdpi/icon.png
$(android_drawableMdpiIconPath) :
	@mkdir -p $(@D)
	ln -s ../../../../res/icons/icon-48.png $@
endif

ifneq ($(wildcard res/icons/icon-72.png),)
android_drawableHdpiIconPath := $(android_targetPath)/res/drawable-hdpi/icon.png
$(android_drawableHdpiIconPath) :
	@mkdir -p $(@D)
	ln -s ../../../../res/icons/icon-72.png $@
endif

ifneq ($(wildcard res/icons/icon-96.png),)
android_drawableXhdpiIconPath := $(android_targetPath)/res/drawable-xhdpi/icon.png
$(android_drawableXhdpiIconPath) :
	@mkdir -p $(@D)
	ln -s ../../../../res/icons/icon-96.png $@
endif

ifneq ($(wildcard res/icons/icon-144.png),)
android_drawableXxhdpiIconPath := $(android_targetPath)/res/drawable-xxhdpi/icon.xml
# "iconbig" used by Xperia Play launcher, links to xxhdpi icon
$(android_drawableXxhdpiIconPath) :
	@mkdir -p $(@D) $(android_targetPath)/res/drawable/
	ln -s ../../../../res/icons/icon-144.png $(android_targetPath)/res/drawable/icon144.png
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
	ln -s ../../../../res/icons/ouya_icon.png $@
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

ifndef android_noArmv6

android_armv6SOPath := $(android_targetPath)/lib-debug/armeabi/lib$(android_soName).so
android-armv6 :
	$(MAKE) -j3 -f $(targetPrefix)-$(android_minSDK)-armv6.mk
$(android_armv6SOPath) : android-armv6

android_armv6ReleaseSOPath := $(android_targetPath)/lib-release/armeabi/lib$(android_soName).so
android-armv6-release :
	$(MAKE) -j3 -f $(targetPrefix)-$(android_minSDK)-armv6-release.mk
$(android_armv6ReleaseSOPath) : android-armv6-release

endif

ifndef android_noArmv7

android_armv7SOPath := $(android_targetPath)/libs-debug/armeabi-v7a/lib$(android_soName).so
android-armv7 :
	$(MAKE) -j3 -f $(targetPrefix)-$(android_minSDK)-armv7.mk
$(android_armv7SOPath) : android-armv7

android_armv7ReleaseSOPath := $(android_targetPath)/libs-release/armeabi-v7a/lib$(android_soName).so
android-armv7-release :
	$(MAKE) -j3 -f $(targetPrefix)-$(android_minSDK)-armv7-release.mk
$(android_armv7ReleaseSOPath) : android-armv7-release

endif

ifndef android_noX86

android_x86SOPath := $(android_targetPath)/libs-debug/x86/lib$(android_soName).so
android-x86 :
	$(MAKE) -j3 -f $(targetPrefix)-$(android_minSDK)-x86.mk
$(android_x86SOPath) : android-x86

android_x86ReleaseSOPath := $(android_targetPath)/libs-release/x86/lib$(android_soName).so
android-x86-release :
	$(MAKE) -j3 -f $(targetPrefix)-$(android_minSDK)-x86-release.mk
$(android_x86ReleaseSOPath) : android-x86-release

endif

android-release : $(android_armv6ReleaseSOPath) $(android_armv7ReleaseSOPath) $(android_x86ReleaseSOPath)

# apks

ifdef V
 antVerbose := -verbose
endif

android_projectDeps := $(android_buildXml) $(android_proguardConfPath)

ifndef android_antTarget
 android_antTarget := release
endif

android_apkPath := $(android_targetPath)/bin-debug/$(android_metadata_project)-$(android_antTarget).apk
android-apk : $(android_projectDeps) $(android_armv7SOPath) $(android_armv6SOPath) $(android_x86SOPath)
	cd $(android_targetPath) && ANT_OPTS=-Dimagine.path=$(IMAGINE_PATH) ant $(antVerbose) -Dout.dir=bin-debug \
-Dnative.libs.absolute.dir=libs-debug -Djar.libs.dir=libs-debug $(android_antTarget)

android-install : android-apk
	adb install -r $(android_apkPath)

android-install-only :
	adb install -r $(android_apkPath)

android_apkReleasePath := $(android_targetPath)/bin-release/$(android_metadata_project)-$(android_antTarget).apk
android-release-apk : $(android_projectDeps) $(android_armv7ReleaseSOPath) $(android_armv6ReleaseSOPath) $(android_x86ReleaseSOPath)
	cd $(android_targetPath) && ANT_OPTS=-Dimagine.path=$(IMAGINE_PATH) ant $(antVerbose) -Dout.dir=bin-release \
-Dnative.libs.absolute.dir=libs-release -Djar.libs.dir=libs-debug $(android_antTarget)

android-release-install : android-release-apk
	adb install -r $(android_apkReleasePath)

android-release-install-only :
	adb install -r $(android_apkReleasePath)

android-release-ready : 
	cp $(android_apkReleasePath) $(IMAGINE_PATH)/../releases-bin/android/$(android_metadata_project)-$(android_minSDK)-$(android_metadata_version).apk

android-check :
	@echo "Checking compiled debug version of $(android_metadata_project) (SDK $(android_minSDK)) $(android_metadata_version)"
	@strings $(android_targetPath)/libs-debug/*/*.so | grep " $(android_metadata_version)"

android-release-check :
	@echo "Checking compiled release version of $(android_metadata_project) (SDK $(android_minSDK)) $(android_metadata_version)"
	@strings $(android_targetPath)/libs-release/*/*.so | grep " $(android_metadata_version)"

android-release-clean:
	rm -f $(android_armv6ReleaseSOPath) $(android_armv7ReleaseSOPath) $(android_x86ReleaseSOPath)
	rm -rf build/$(targetPrefix)-$(android_minSDK)-armv6-release build/$(targetPrefix)-$(android_minSDK)-armv7-release build/$(targetPrefix)-$(android_minSDK)-x86-release

.PHONY: android-metadata  \
android-armv6 android-armv7 android-armv6-release android-armv7-release android-release \
android-x86 android-x86-release \
android-apk android-install android-install-only \
android-release-apk android-release-install android-release-install-only \
android-release-ready \
android-check android-release-check \
android-release-clean
