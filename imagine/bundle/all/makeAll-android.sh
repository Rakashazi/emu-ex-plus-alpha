makefilesToRun='
	src/libogg/android-armv6.mk
	src/libogg/android-armv7.mk
	src/libogg/android-x86.mk
	
	src/tremor/android-armv6.mk
	src/tremor/android-armv7.mk
	src/libvorbis/android-x86.mk
	
	src/libsndfile/android-armv6.mk
	src/libsndfile/android-armv7.mk
	src/libsndfile/android-x86.mk
	
	src/minizip/android-armv6.mk
	src/minizip/android-armv7.mk
	src/minizip/android-x86.mk
	
	src/boost/android-armv6.mk
	src/boost/android-armv7.mk
	src/boost/android-x86.mk
'

for makefile in $makefilesToRun
do
	oldDir=`pwd`
	cd `dirname $makefile`
	echo "running make on $makefile"
	make -f `basename $makefile` $@
	if [ $? != 0 ]
	then
		exit 1
	fi
	cd $oldDir
done

