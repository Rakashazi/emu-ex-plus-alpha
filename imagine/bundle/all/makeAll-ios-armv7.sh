makefilesToRun='
	src/btstack/ios-armv7.mk

	src/libogg/ios-armv7.mk
	
	src/libvorbis/ios-armv7.mk
	
	src/flac/ios-armv7.mk
	
	src/xz/ios-armv7.mk
	
	src/libarchive/ios-armv7.mk
	
	src/libcxx/ios-armv7.mk
'

source runMakefiles.sh

runMakefiles $@

