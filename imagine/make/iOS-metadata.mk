include metadata/conf.mk

# needed generics
ifndef iOS_metadata_name
 iOS_metadata_name := $(metadata_name)
 ifndef iOS_metadata_name
  $(error Please specify metadata_name or iOS_metadata_name)
 endif
endif
iOS_gen_metadata_args += --name="$(iOS_metadata_name)"

ifndef iOS_metadata_exec
 iOS_metadata_exec := $(metadata_exec)
 ifndef iOS_metadata_exec
  $(error Please specify metadata_exec or iOS_metadata_exec)
 endif
endif
iOS_gen_metadata_args += --exec="$(iOS_metadata_exec)"

ifndef iOS_metadata_id
 iOS_metadata_id := $(metadata_id)
 ifndef iOS_metadata_id
  $(error Please specify metadata_id or iOS_metadata_id)
 endif
endif
iOS_gen_metadata_args += --id=$(iOS_metadata_id)

ifndef iOS_metadata_vendor
 iOS_metadata_vendor := $(metadata_vendor)
 ifndef iOS_metadata_vendor
  $(error Please specify metadata_vendor or iOS_metadata_vendor)
 endif
endif
iOS_gen_metadata_args += --vendor="$(iOS_metadata_vendor)"

ifndef iOS_metadata_version
 iOS_metadata_version := $(metadata_version)
 ifndef iOS_metadata_version
  $(error Please specify metadata_version or iOS_metadata_version)
 endif
endif
iOS_gen_metadata_args += --version=$(iOS_metadata_version)

# needed iOS-specific 
ifndef iOS_metadata_bundleName
 ifndef metadata_pkgName
  $(error Please specify iOS_metadata_bundleName or metadata_pkgName)
 else
  iOS_metadata_bundleName := $(metadata_pkgName)
 endif
endif
iOS_gen_metadata_args += --bundle-name=$(iOS_metadata_bundleName)

ifdef iOS_metadata_setuid
iOS_gen_metadata_args += --setuid
endif