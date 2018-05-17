
for f in *day.txt; do

	num=`grep "D " $f | wc -l`

	if [ $num -gt 0 ]
	then

		date=`grep "D " $f | head -n 1 | sed -e 's/D //' | sed -e 's/ .*//'`
		av=`grep "D " $f | sed -e "s/.*age=//" | sed -e "s/ .*//" | awk '{s+=$1}END{print "",s/NR}' RS="\n"`
		echo $f $date $num $av
	fi
done
