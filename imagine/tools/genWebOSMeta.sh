for arg in "$@"
do
	case $arg in
		*=*) optarg=`expr "X$arg" : '[^=]*=\(.*\)'` ;;
	esac
	
	#echo $arg
	
	case "$arg" in
		--name=*)
			name=$optarg
		;;
		--id=*)
			id=$optarg
		;;
		--version=*)
			version=$optarg
		;;
		--vendor=*)
			vendor=$optarg
		;;
		--exec=*)
			exec=$optarg
		;;
		--required-memory=*)
			requiredMemory=$optarg
		;;
		# special actions
		-v | --verbose)
			verbose=1
		;;
		-*)
			echo "warning: unknown parameter $arg"
		;;
		*)
			outPath=$arg
		;;
	esac
done

if [ ! "$name" ]
then
	echo "error: no name specified"
	exit 1
fi

if [ ! $id ]
then
	echo "error: no id specified"
	exit 1
fi

if [ ! $version ]
then
	echo "error: no version specified"
	exit 1
fi

if [ ! "$vendor" ]
then
	echo "error: no vendor specified"
	exit 1
fi

if [ ! $exec ]
then
	echo "error: no executable name specified"
	exit 1
fi

if [ ! $requiredMemory ]
then
	requiredMemory=32
fi

if [ ! "$outPath" ]
then
	echo "error: no output file specified"
	exit 1
fi

echo "{
	\"title\": \"$name\",
	\"type\": \"pdk\",
	\"main\": \"$exec\",
	\"id\": \"$id\",
	\"version\": \"$version\",
	\"vendor\": \"$vendor\",
	\"requiredMemory\": $requiredMemory
}" > $outPath
