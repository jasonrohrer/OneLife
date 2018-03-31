if [ -f ../gameSource/testMap.txt ]
then
	rm *.db testMapStale.txt; cp ../gameSource/testMap.txt .; make; ./OneLifeServer
else
	echo "testMap.txt does not exist in ../gameSource"
fi


