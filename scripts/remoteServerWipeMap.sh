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



echo "" 
echo "Re-launching server"
echo ""


echo -n "0" > ~/checkout/OneLife/server/settings/shutdownMode.ini



cd ~/checkout/OneLife/server/

sh ./runHeadlessServerLinux.sh

echo -n "1" > ~/keepServerRunning.txt

