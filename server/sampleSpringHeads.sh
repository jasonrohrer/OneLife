

for ((y=-320;y<=320;y+=40)); do
	for ((x=-320;x<=320;x+=40)); do
		./dumpMapCoord $x $y
	done
done
