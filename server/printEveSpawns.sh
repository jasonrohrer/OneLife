
if [ $# -ne 1 ]
then
	echo "Usage:"
	echo "./printEveSpawns.sh lifeLogFile"
	exit
fi




while read b time coord
do
	
	date=`date -d @$time`

	echo "$date ||  $coord  "
done < <( grep noParent $1 | grep -v ",-20" | grep -v ",20" | grep " F " | sed -e "s/\(B [0-9]* \).* F /\1/" | sed "s/ noParent.*//" )