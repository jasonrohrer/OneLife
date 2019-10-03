pushd ~/checkout/OneLife/documentation/devProcess/names/

g++ -g -o processNameYear processNameYear.cpp

popd

processApp=~/checkout/OneLife/documentation/devProcess/names/processNameYear

for f in yob*.txt
do
	echo "Processing $f"
	dos2unix $f
	cat $f | sort --field-separator=',' -k1,1 -k2,2  > sorted_$f
	$processApp sorted_$f male_$f female_$f 0.20
done

cat male_* | sort | uniq | tr '[a-z]' '[A-Z]' > maleNames.txt
cat female_* | sort | uniq | tr '[a-z]' '[A-Z]' > femaleNames.txt
