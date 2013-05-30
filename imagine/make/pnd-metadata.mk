include metadata/conf.mk

# needed generics
ifndef pnd_metadata_name
 pnd_metadata_name := $(metadata_name)
 ifndef pnd_metadata_name
  $(error Please specify metadata_name or pnd_metadata_name)
 endif
endif
pnd_gen_metadata_args += --name="$(pnd_metadata_name)"

ifndef pnd_metadata_exec
 pnd_metadata_exec := $(metadata_exec)
 ifndef pnd_metadata_exec
  $(error Please specify metadata_exec or pnd_metadata_exec)
 endif
endif
pnd_gen_metadata_args += --exec="$(pnd_metadata_exec)"

ifndef pnd_metadata_id
 pnd_metadata_id := $(metadata_id)
 ifndef pnd_metadata_id
  $(error Please specify metadata_id or pnd_metadata_id)
 endif
endif
pnd_gen_metadata_args += --id=$(pnd_metadata_id)

ifndef pnd_metadata_vendor
 pnd_metadata_vendor := $(metadata_vendor)
 ifndef pnd_metadata_vendor
  $(error Please specify metadata_vendor or pnd_metadata_vendor)
 endif
endif
pnd_gen_metadata_args += --vendor="$(pnd_metadata_vendor)"

ifndef pnd_metadata_version
 pnd_metadata_version := $(metadata_version)
 ifndef pnd_metadata_version
  $(error Please specify metadata_version or pnd_metadata_version)
 endif
endif
pnd_gen_metadata_args += --version=$(pnd_metadata_version)

# needed PND-specific 
ifndef pnd_metadata_pndName
 ifndef metadata_pkgName
  $(error Please specify pnd_metadata_pndName or metadata_pkgName)
 else
  pnd_metadata_pndName := $(metadata_pkgName)
 endif
endif
iOS_gen_metadata_args += --bundle-name=$(pnd_metadata_pndName)

# optional PND-specific
ifdef pnd_metadata_website
 pnd_gen_metadata_args += --website="$(pnd_metadata_website)"
endif

ifdef pnd_metadata_description
 pnd_gen_metadata_args += --description="$(pnd_metadata_description)"
endif

ifdef pnd_metadata_license
 pnd_gen_metadata_args += --license="$(pnd_metadata_license)"
endif

ifdef pnd_metadata_licenseURL
 pnd_gen_metadata_args += --license-url="$(pnd_metadata_licenseURL)"
endif

ifdef pnd_metadata_sourceCodeURL
 pnd_gen_metadata_args += --source-code-url="$(pnd_metadata_sourceCodeURL)"
endif

ifdef pnd_metadata_subcategory
 pnd_gen_metadata_args += --subcategory="$(pnd_metadata_subcategory)"
endif