makefilesToRun='
	src/libcxx/android-x86_64.mk

	src/libogg/android-x86_64.mk
	
	src/libvorbis/android-x86_64.mk
	
	src/flac/android-x86_64.mk
	
	src/xz/android-x86_64.mk
	
	src/libarchive/android-x86_64.mk
'

source runMakefiles.sh

runMakefiles $@
