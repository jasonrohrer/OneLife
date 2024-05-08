if [ -f ../gameSource/testMap.txt ]
then
	rm *.db curseSave.txt mapDummyRecall.txt testMapStale.txt shutdownLongLineagePos.txt lastEveLocation.txt landingLocations.txt; cp ../gameSource/testMap.txt .; make; ./OneLifeServer
else
	echo "testMap.txt does not exist in ../gameSource"
fi


