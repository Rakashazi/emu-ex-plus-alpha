makefilesToRun='
	src/btstack/ios-armv6.mk
	src/btstack/ios-armv7.mk

	src/libogg/ios-armv6.mk
	src/libogg/ios-armv7.mk
	
	src/tremor/ios-armv6.mk
	src/tremor/ios-armv7.mk
	
	src/libsndfile/ios-armv6.mk
	src/libsndfile/ios-armv7.mk
	
	src/minizip/ios-armv6.mk
	src/minizip/ios-armv7.mk
	
	src/boost/ios-armv6.mk
	src/boost/ios-armv7.mk
	
	src/libcxxabi/ios-armv6.mk
	src/libcxx/ios-armv6.mk
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

