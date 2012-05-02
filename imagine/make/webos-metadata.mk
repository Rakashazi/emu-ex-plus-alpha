include metadata/conf.mk

# needed generics
ifndef webOS_metadata_name
 webOS_metadata_name := $(metadata_name)
 ifndef webOS_metadata_name
  $(error Please specify metadata_name or webOS_metadata_name)
 endif
endif
webOS_gen_metadata_args += "--name=$(webOS_metadata_name)"

ifndef webOS_metadata_exec
 webOS_metadata_exec := $(metadata_exec)
 ifndef webOS_metadata_exec
  $(error Please specify metadata_exec or webOS_metadata_exec)
 endif
endif
webOS_gen_metadata_args += "--exec=$(webOS_metadata_exec)"

ifndef webOS_metadata_id
 webOS_metadata_id := $(metadata_id)
 ifndef webOS_metadata_id
  $(error Please specify metadata_id or webOS_metadata_id)
 endif
endif
webOS_gen_metadata_args += --id=$(webOS_metadata_id)

ifndef webOS_metadata_vendor
 webOS_metadata_vendor := $(metadata_vendor)
 ifndef webOS_metadata_vendor
  $(error Please specify metadata_vendor or webOS_metadata_vendor)
 endif
endif
webOS_gen_metadata_args += "--vendor=$(webOS_metadata_vendor)"

# needed WebOS-specific 
ifndef webOS_metadata_version
 webOS_metadata_version := $(metadata_version)
 ifndef webOS_metadata_version
  $(error Please specify metadata_version or webOS_metadata_version)
 endif
endif
webOS_gen_metadata_args += --version=$(webOS_metadata_version)

# optional WebOS-specific
ifdef webOS_metadata_requiredMemory
 webOS_gen_metadata_args += --required-memory=$(webOS_metadata_requiredMemory)
endif
