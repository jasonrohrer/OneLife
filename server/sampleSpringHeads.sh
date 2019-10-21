

while read -r id
do
	if [[ -f objects/$id.txt ]];
	then
		name=$(cat objects/$id.txt | sed -n 2p );
	else
		name="ID: $id"
	fi
	echo $name
done < <( ./generateSpringCoords.sh | ./dumpMapCoord - )

