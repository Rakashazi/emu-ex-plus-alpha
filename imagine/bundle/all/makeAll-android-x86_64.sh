makefilesToRun='
	src/libogg/android-x86_64.mk
	
	src/libvorbis/android-x86_64.mk
	
	src/libsndfile/android-x86_64.mk
	
	src/xz/android-x86_64.mk
	
	src/libarchive/android-x86_64.mk
	
	src/boost/android-x86_64.mk
'

source runMakefiles.sh

runMakefiles $@