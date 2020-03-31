if [ $# -ne 2 ]
then
	echo "Usage:"
	echo "./printObjectCreationLocations.sh mapChangeLogFile object_id"
	exit
fi


startTime=`echo -n "$1" | sed -e "s/time_mapLog.txt//" | sed "s/.*\///" `

echo "startTime = $startTime"


while read time coord id user
do
	timestamp=`echo "$startTime + $time" | bc`
	
	date=`date -d @$timestamp`

	echo "$coord  || $date ||  $id  "
done < <( cat $1 | sed -e "s/\([0-9.]* \)\([0-9\-]*\) \([0-9\-]*\) /\1(\2,\3) id=/" | grep "id=$2" )