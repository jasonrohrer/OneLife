
for f in *day.txt; do

	deaths=`grep "D " $f | wc -l`

	murders=`grep ") killer_" $f | wc -l`


	if [ $deaths -gt 0 ]
	then

	
		date=`grep "D " $f | head -n 1 | sed -e 's/D //' | sed -e 's/ .*//'`

		echo $f $date $deaths $murders
	fi
done
