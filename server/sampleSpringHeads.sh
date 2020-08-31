
if [ ! -f printObjectName ]; then
	./makePrintObjectName
fi


if [ ! -f dumpMapCoord ]; then
	./makeDumpMapCoord
fi



while read -r id
do
	if [[ -f objects/$id.txt ]];
	then
		name=$(cat objects/$id.txt | sed -n 2p );
	elif [[ $id == 0 ]];
	then
		name="Empty space";
	else
		name=`./printObjectName $id | tail -1`
	fi
	echo $name
done < <( ./generateSpringCoords.sh | ./dumpMapCoord - )

