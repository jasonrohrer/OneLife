
for f in *day.txt; do

	deaths=`grep "D " $f | wc -l`

	murders=`grep ") killer_" $f | wc -l`


	uniqPlayers=`grep "D " $f | sed -e "s/D [0-9]* [0-9]* //" | sed -e "s/ .*//" | sort -n | uniq -c | wc -l`

	uniqVictims=`grep "D " $f | grep "killer_" | sed -e "s/D [0-9]* [0-9]* //" | sed -e "s/ .*//" | sort -n | uniq -c | wc -l`

	uniqKillers=`grep "D " $f | grep "killer_" | sed -e "s/.* killer_[0-9]*_//" | sed -e "s/ .*//" | sort | uniq -c | wc -l`

	if [ $deaths -gt 0 ]
	then

	
		date=`grep "D " $f | head -n 1 | sed -e 's/D //' | sed -e 's/ .*//'`

		echo $f $date $deaths $murders $uniqPlayers $uniqVictims $uniqKillers
	fi
done
