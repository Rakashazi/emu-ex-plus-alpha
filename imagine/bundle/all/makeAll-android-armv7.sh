makefilesToRun='
	src/libcxx/android-armv7.mk

	src/libogg/android-armv7.mk
	
	src/libvorbis/android-armv7.mk
	
	src/flac/android-armv7.mk
	
	src/xz/android-armv7.mk
	
	src/libarchive/android-armv7.mk
'

source runMakefiles.sh

runMakefiles $@

