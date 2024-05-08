# put a 1 in this file to stop this
# to block map wipes per-server
flagFile=~/skipMapWipe.txt

skipWipe=0

if [ -f $flagFile ]
then

	flag=$(head -n 1 $flagFile)


	if [ "$flag" = "1" ]
	then
		echo ""
		echo "Skipping map wipe for this server"
		echo ""
		skipWipe=1
	fi
fi




echo "" 
echo "Shutting down server"
echo ""

serverPID=`pgrep OneLifeServer`

if [ -z $serverPID ]
then
	echo "Server not running!"
else

	echo -n "0" > ~/keepServerRunning.txt
	
	echo -n "1" > ~/checkout/OneLife/server/settings/shutdownMode.ini

	echo "" 
	echo "Set server shutdownMode, waiting for server to exit"
	echo ""

        while kill -CONT $serverPID 1>/dev/null 2>&1; do sleep 1; done

	echo "" 
	echo "Server has shutdown"
	echo ""
fi


if [ $skipWipe -ne 1 ]
then
	echo "" 
	echo "Deleting map and biome and mapTime db files and recent placements"
	echo ""
	

	rm ~/checkout/OneLife/server/biome.db 
	rm ~/checkout/OneLife/server/map.db
	rm ~/checkout/OneLife/server/mapTime.db
	rm ~/checkout/OneLife/server/lookTime.db
	rm ~/checkout/OneLife/server/floor.db 
	rm ~/checkout/OneLife/server/floorTime.db
	rm ~/checkout/OneLife/server/eve.db

    # don't delete playerStats.db

	rm ~/checkout/OneLife/server/recentPlacements.txt
	rm ~/checkout/OneLife/server/eveRadius.txt
	rm ~/checkout/OneLife/server/mapDummyRecall.txt
	rm ~/checkout/OneLife/server/lastEveLocation.txt
	rm ~/checkout/OneLife/server/biomeRandSeed.txt
	rm ~/checkout/OneLife/server/landingLocations.txt

	echo "0,0" > ~/checkout/OneLife/server/shutdownLongLineagePos.txt
fi




echo "" 
echo "Re-launching server"
echo ""


echo -n "0" > ~/checkout/OneLife/server/settings/shutdownMode.ini



cd ~/checkout/OneLife/server/

bash -l ./runHeadlessServerLinux.sh

echo -n "1" > ~/keepServerRunning.txt

