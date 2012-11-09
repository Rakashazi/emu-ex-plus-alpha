cxxExceptions := 1

# ld complains about ___divdi3 if using -dead_strip with LTO, cause unknown but could be linker bug
ios_noDeadStrip := 1