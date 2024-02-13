NEW=
for def in $2; do
	NEW="${NEW}#define ${def/=/ } 1
"
done

for inc in $3; do
	NEW="${NEW}#define ${def/=/ } 0
"
done

for inc in $4; do
	NEW="${NEW}#include $inc
"
done

# check for empty config
if [ -z "${NEW}" ]; then
	exit 0
fi

if [ -f $1 ]; then
	OLD=`cat $1`"
"
	#echo -n "$NEW"
	#echo -n "$OLD"
	if [[ "$NEW" == "$OLD" ]]; then
		exit 0
	fi
fi

echo "Generating Config $1"
printf "$NEW" > $1
