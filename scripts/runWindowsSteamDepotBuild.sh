if [ $# -lt 1 ]
then
	echo "Usage:"
	echo "runWindowsSteamDepotBuild.sh  version_number"
	echo ""
	echo ""
	
	exit 1
fi


cd ~


echo "Removing old steamLatest directory."

rm -rf steamLatest


echo "Extracting steamLatest.tar.gz"
tar xzf steamLatest.tar.gz


steamcmd +login "jasonrohrergames" +run_app_build -desc OneLifeClient_windows_v$1 ~/checkout/OneLifeWorking/build/steam/app_build_windows_ON_SERVER_595690.vdf +quit | tee /tmp/steamBuildLog.txt


echo "About to run AHAP Steam Depot Build script"
echo ""
echo -n "Hit [ENTER] when ready: "
read

checkout/OneLifeWorking/scripts/runWindowsSteamDepotBuildAHAP.sh $1