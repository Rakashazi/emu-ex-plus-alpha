ifndef inc_pkg_libsndfile
inc_pkg_libsndfile := 1

ifdef package_libsndfile_externalPath
 CPPFLAGS += -I$(package_libsndfile_externalPath)/include
 LDLIBS += $(package_libsndfile_externalPath)/lib/libsndfile.a
else
 pkgConfigDeps += sndfile
endif

endif