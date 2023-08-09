makefilesToRun='
	src/libcxx/android-x86.mk

	src/libogg/android-x86.mk
	
	src/libvorbis/android-x86.mk
	
	src/flac/android-x86.mk
	
	src/xz/android-x86.mk
	
	src/libarchive/android-x86.mk
'

source runMakefiles.sh

runMakefiles $@

