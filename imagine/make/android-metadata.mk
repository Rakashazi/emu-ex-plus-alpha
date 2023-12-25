include $(projectPath)/metadata/conf.mk

# needed generics
ifndef android_metadata_name
 android_metadata_name := $(metadata_name)
 ifndef android_metadata_name
  $(error Please specify metadata_name or android_metadata_name)
 endif
endif
android_gen_metadata_args += --name="$(android_metadata_name)"

ifndef android_metadata_id
 android_metadata_id := $(metadata_id)
 ifndef android_metadata_id
  $(error Please specify metadata_id or android_metadata_id)
 endif
endif

ifndef android_metadata_vendor
 android_metadata_vendor := $(metadata_vendor)
 ifndef android_metadata_vendor
  $(error Please specify metadata_vendor or android_metadata_vendor)
 endif
endif
android_gen_metadata_args += --vendor="$(android_metadata_vendor)"

ifndef android_metadata_version
 android_metadata_version := $(metadata_version)
 ifndef android_metadata_version
  $(error Please specify metadata_version or android_metadata_version)
 endif
endif
android_gen_metadata_args += --version=$(android_metadata_version)

ifndef android_metadata_noIcon
 android_metadata_noIcon := $(metadata_noIcon)
 ifdef android_metadata_noIcon
  android_gen_metadata_args += --no-icon
 endif
endif

# optional generics
ifdef metadata_supportedMIMETypes
 android_gen_metadata_args += --intent-mimetypes="$(metadata_supportedMIMETypes)"
endif

ifdef metadata_supportedFileExtensions
 android_gen_metadata_args += --intent-file-extensions="$(metadata_supportedFileExtensions)"
endif

# needed android-specific 
ifndef android_metadata_project
 ifndef metadata_pkgName
  $(error Please specify android_metadata_project or metadata_pkgName)
 else
  android_metadata_project := $(metadata_pkgName)
 endif
endif

# optional android-specific
ifdef android_metadata_versionCode
 android_gen_metadata_args += --version-code=$(android_metadata_versionCode)
endif

ifdef android_metadata_versionCodeExtra
 android_gen_metadata_args += --version-code-extra=$(android_metadata_versionCodeExtra)
endif

ifdef android_metadata_writeExtStore
 android_gen_metadata_args += --permission-write-ext
endif

ifdef android_metadata_bluetooth
 android_gen_metadata_args += --permission-bluetooth
endif

ifndef android_metadata_target_sdk
 android_metadata_target_sdk := 34
endif

ifdef android_metadata_vibrate
 android_gen_metadata_args += --permission-vibrate
endif
 
ifdef android_metadata_installShortcut
 android_gen_metadata_args += --permission-install-shortcut
endif

ifndef metadata_noIcon
 ifdef android_metadata_xperiaPlayOptimized
  android_gen_metadata_args += --xperia-play-optimized
 endif
endif

ifdef android_metadata_legacyStorage
 android_gen_metadata_args += --legacy-storage
endif

ifdef android_metadata_appExtStorage
 android_gen_metadata_args += --app-ext-storage
endif

ifeq ($(findstring O_RELEASE=1,$(android_makefileOpts)),)
 android_gen_metadata_args += --debug
endif
