cxxExceptions := 1

# ld complains about ___floatdisf if using -dead_strip with LTO, cause unknown but could be linker bug
ios_noDeadStrip := 1