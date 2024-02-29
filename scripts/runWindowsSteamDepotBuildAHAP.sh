if [ $# -lt 1 ]
then
	echo "Usage:"
	echo "runWindowsSteamDepotBuildAHAP.sh  version_number"
	echo ""
	echo ""
	
	exit 1
fi


cd ~

echo "Removing old steamLatestAHAP directory."

rm -rf steamLatestAHAP


echo "Extracting steamLatest.tar.gz separately for AHAP"
mkdir ahapTemp

cp steamLatest.tar.gz ahapTemp

cd ahapTemp

tar xzf steamLatest.tar.gz

mv steamLatest ../steamLatestAHAP

cd ..
rm -rf ahapTemp


echo "Putting 1 in isAHAP.txt for AHAP build"

echo -n "1" > steamLatestAHAP/isAHAP.txt




echo "Running steamcmd to push AHAP Windows build"

steamcmd +login "jasonrohrergames" +run_app_build -desc AnotherPlanetClient_windows_v$1 ~/checkout/OneLifeWorking/build/steam/ahap/app_build_windows_ON_SERVER_2787060.vdf +quit | tee /tmp/steamBuildLogAHAP.txt