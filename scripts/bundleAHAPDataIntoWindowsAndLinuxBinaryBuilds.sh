if [ $# -lt 1 ]
then
	echo "Usage:"
	echo "bundleAHAPDataIntoWindowsAndLinuxBinaryBuilds.sh  version_number"
	echo ""
	echo ""
	echo "If version_number is XYZ, then these files must exist:"
	echo "  ~/ahapDownloads/AnotherPlanet_vXYZ_Windows.zip"
	echo "  and"
	echo "  ~/ahapDownloads/AnotherPlanet_vXYZ_Linux.tar.gz" 
	
	echo ""
	echo "If latest AHAP data has version number ABC,"
	echo "The following new files will be generated:"
	echo "  ~/ahapDownloads/AnotherPlanet_vXYZ_dABC_Windows.zip"
	echo "  and"
	echo "  ~/ahapDownloads/AnotherPlanet_vXYZ_dABC_Linux.tar.gz" 
	echo ""
	
	exit 1
fi


windowsBaseFileName=AnotherPlanet_v${1}_Windows.zip
linuxBaseFileName=AnotherPlanet_v${1}_Linux.tar.gz

windowsBaseFile=~/ahapDownloads/$windowsBaseFileName
linuxBaseFile=~/ahapDownloads/$linuxBaseFileName


if [ ! -f $windowsBaseFile ]
then
	echo ""
	echo "File does not exist:"
	echo "  $windowsBaseFile"
	echo ""

	exit 1
fi

if [ ! -f $linuxBaseFile ]
then
	echo ""
	echo "File does not exist:"
	echo "  $linuxBaseFile"
	echo ""

	exit 1
fi


cd ~

rm -rf ahapTemp

mkdir ahapTemp

cd ahapTemp

cp $windowsBaseFile .
cp $linuxBaseFile .




echo "" 
echo "Preparing to bundle latest AHAP data with AHAP Windows and Linux builds"
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


newWindowsFileName=AnotherPlanet_v${1}_d${baseDataVersion}_Windows.zip
newLinuxFileName=AnotherPlanet_v${1}_d${baseDataVersion}_Linux.tar.gz

echo "About to bundle these new files:"
echo "   $newWindowsFileName"
echo "     and"
echo "   $newLinuxFileName"
echo ""


echo "Press [ENTER] when ready."
echo ""
echo -n "Ready? "
read goWord


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



echo "" 
echo "Generating caches for content"
echo ""

cd ~/checkout/ahapDataTemp/

cp ~/checkout/OneLifeWorking/gameSource/reverbImpulseResponse.aiff .
~/checkout/OneLifeWorking/gameSource/regenerateCaches
rm reverbImpulseResponse.aiff
# don't include bin_cache files in downloads, because they change
# with each update, and are too big
# cache.fcz files are full of compressed text files, so they're much smaller
# and fine to included when they change
rm */bin_*cache.fcz




echo "" 
echo "Extracting existing Windows v$1 build"
echo ""

cd ~/ahapTemp
unzip $windowsBaseFile

baseDirName=AnotherPlanet_v$1

echo "" 
echo "Copying v$baseDataVersion data into Windows build"
echo ""

cp -r ~/checkout/ahapDataTemp/* $baseDirName/

echo "" 
echo "Bundling Windows build"
echo ""

zip -r $newWindowsFileName $baseDirName

rm -rf $baseDirName


echo "" 
echo "Extracting existing Linux v$1 build"
echo ""


tar xf $linuxBaseFile


echo "" 
echo "Copying v$baseDataVersion data into Linux build"
echo ""

cp -r ~/checkout/ahapDataTemp/* $baseDirName/


echo "" 
echo "Bundling Windows build"
echo ""

tar czf $newLinuxFileName $baseDirName

rm -rf $baseDirName


echo "" 
echo "Copying new builds into ~/ahapDownloads directory"
echo ""


cp $newWindowsFileName ~/ahapDownloads
cp $newLinuxFileName ~/ahapDownloads



echo "" 
echo "Cleaning up..."
echo ""

rm -rf ~/checkout/ahapDataTemp


rm -rf ahapTemp


cd ~


echo "" 
echo "Done."
echo ""
