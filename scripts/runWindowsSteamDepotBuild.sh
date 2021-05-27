if [ $# -lt 1 ]
then
	echo "Usage:"
	echo "runWindowsSteamDepotBuild.sh  version_number"
	echo ""
	echo ""
	
	exit 1
fi


steamcmd +login "jasonrohrergames" +run_app_build -desc OneLifeClient_windows_v$1 ~/checkout/OneLifeWorking/build/steam/app_build_content_ON_SERVER_595690.vdf +quit | tee /tmp/steamBuildLog.txt