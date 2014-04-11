makefilesToRun='
	src/minizip/linux-armv7-pandora.mk
	
	src/boost/linux-armv7-pandora.mk
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

