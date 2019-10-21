

for ((y=-320;y<=320;y+=40)); do
	for ((x=-320;x<=320;x+=40)); do
		id=`./dumpMapCoord $x $y`
		
		if [[ -f objects/$id.txt ]];
		then
			name=$(cat objects/$id.txt | sed -n 2p );
		else
			name="ID: $id"
		fi
		echo $name
	done
done
