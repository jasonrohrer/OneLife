

for ((y=-320;y<=320;y+=40)); do
	for ((x=-320;x<=320;x+=40)); do
		id=`./dumpMapCoord $x $y`
		
		if [[ $id == 0 ]];
		then
			name="Empty Spot"
		else
			name=$(cat objects/$id.txt | sed -n 2p );
		fi
		echo $name
	done
done
