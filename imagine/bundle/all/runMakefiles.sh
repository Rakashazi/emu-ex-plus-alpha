runMakefiles ()
{
	makeParams=install
	if [[ "$@" ]]
	then
		echo "Make parameters: $@"
		makeParams=$@
	fi
	for makefile in $makefilesToRun
	do
		oldDir=`pwd`
		cd `dirname $makefile`
		echo "running make on $makefile"
		make -f `basename $makefile` $makeParams
		if [ $? != 0 ]
		then
			exit 1
		fi
		cd $oldDir
	done
}

