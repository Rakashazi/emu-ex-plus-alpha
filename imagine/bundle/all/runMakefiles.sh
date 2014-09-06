runMakefiles ()
{
	if [[ "$@" ]]
	then
		echo "Make parameters: $@"
	fi
	for makefile in $makefilesToRun
	do
		oldDir=`pwd`
		cd `dirname $makefile`
		echo "running make on $makefile"
		make -f `basename $makefile` install $@
		if [ $? != 0 ]
		then
			exit 1
		fi
		cd $oldDir
	done
}

