include metadata/conf.mk

# needed generics
ifndef macosx_metadata_name
 macosx_metadata_name := $(metadata_name)
 ifndef macosx_metadata_name
  $(error Please specify metadata_name or macosx_metadata_name)
 endif
endif
macosx_gen_metadata_args += --name="$(macosx_metadata_name)"

ifndef macosx_metadata_exec
 macosx_metadata_exec := $(metadata_exec)
 ifndef macosx_metadata_exec
  $(error Please specify metadata_exec or macosx_metadata_exec)
 endif
endif
macosx_gen_metadata_args += --exec="$(macosx_metadata_exec)"

ifndef macosx_metadata_id
 macosx_metadata_id := $(metadata_id)
 ifndef macosx_metadata_id
  $(error Please specify metadata_id or macosx_metadata_id)
 endif
endif
macosx_gen_metadata_args += --id=$(macosx_metadata_id)

ifndef macosx_metadata_vendor
 macosx_metadata_vendor := $(metadata_vendor)
 ifndef macosx_metadata_vendor
  $(error Please specify metadata_vendor or macosx_metadata_vendor)
 endif
endif
macosx_gen_metadata_args += --vendor="$(macosx_metadata_vendor)"

ifndef macosx_metadata_version
 macosx_metadata_version := $(metadata_version)
 ifndef macosx_metadata_version
  $(error Please specify metadata_version or macosx_metadata_version)
 endif
endif
macosx_gen_metadata_args += --version=$(macosx_metadata_version)

# needed iOS-specific 
ifndef macosx_metadata_bundleName
 ifndef metadata_pkgName
  $(error Please specify macosx_metadata_bundleName or metadata_pkgName)
 else
  macosx_metadata_bundleName := $(metadata_pkgName)
 endif
endif
macosx_gen_metadata_args += --bundle-name=$(macosx_metadata_bundleName)
