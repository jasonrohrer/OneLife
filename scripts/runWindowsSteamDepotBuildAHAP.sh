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


echo "Setting steamGateServerURL.txt for AHAP build to:"
echo "http://onehouronelife.com/ahapSteamGate/server.php"

echo -n "http://onehouronelife.com/ahapSteamGate/server.php" > steamLatestAHAP/steamGateServerURL.txt




echo "" 
echo "Preparing to bundle latest AHAP data with AHAP Windows Steam Build"
echo ""



echo "" 
echo "Updating OneLifeWorking"
echo ""

cd ~/checkout/OneLifeWorking
git pull --tags


echo "" 
echo "Building headless cache generation tool"
echo ""

cd gameSource
sh ./makeRegenerateCaches




echo "" 
echo "Updating AnotherPlanetDataLatest"
echo ""

cd ~/checkout/AnotherPlanetDataLatest
git pull --tags



lastTaggedDataVersion=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags/AnotherPlanet_v* | sed -e 's/AnotherPlanet_v//'`


echo "" 
echo "Most recent Data git version is:  $lastTaggedDataVersion"
echo ""

baseDataVersion=$lastTaggedDataVersion


rm -rf ~/checkout/ahapDataTemp


echo "" 
echo "Exporting base data for bundling"
echo ""

git checkout -q AnotherPlanet_v$baseDataVersion

git clone . ~/checkout/ahapDataTemp
rm -rf ~/checkout/ahapDataTemp/.git*
rm ~/checkout/ahapDataTemp/.hg*
rm -rf ~/checkout/ahapDataTemp/soundsRaw
rm -rf ~/checkout/ahapDataTemp/faces
rm -rf ~/checkout/ahapDataTemp/scenes
rm -r ~/checkout/ahapDataTemp/*.sh ~/checkout/ahapDataTemp/working ~/checkout/ahapDataTemp/overlays


# restore repository to latest, to avoid confusion later
git checkout -q master


cp -r ~/checkout/ahapDataTemp/* ~/steamLatestAHAP

rm -rf ~/checkout/ahapDataTemp



cd ~/steamLatestAHAP

echo "" 
echo "Generating caches for content"
echo ""


cp ~/checkout/OneLifeWorking/gameSource/reverbImpulseResponse.aiff .
~/checkout/OneLifeWorking/gameSource/regenerateCaches
rm reverbImpulseResponse.aiff
# don't include bin_cache files in downloads, because they change
# with each update, and are too big
# cache.fcz files are full of compressed text files, so they're much smaller
# and fine to included when they change
rm */bin_*cache.fcz




echo "Running steamcmd to push AHAP Windows v$1 build, including v$baseDataVersion content."

steamcmd +login "jasonrohrergames" +run_app_build -desc AnotherPlanetClient_windows__Bin_v$1__Content_v$baseDataVersion ~/checkout/OneLifeWorking/build/steam/ahap/app_build_windows_ON_SERVER_2787060.vdf +quit | tee /tmp/steamBuildLogAHAP.txt