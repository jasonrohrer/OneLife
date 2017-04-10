
echo "" 
echo "Updating minorGems"
echo ""

cd ~/checkout/minorGems
hg pull
hg update



echo "" 
echo "Building diffBundle tool"
echo ""

cd game/diffBundle
./diffBundleCompile



echo "" 
echo "Updating OneLifeData6Latest"
echo ""

cd ~/checkout/OneLifeData6Latest
hg pull
hg update



lastTaggedDataVersion=`hg tags | grep "OneLife" -m 1 | awk '{print $1}' | sed -e 's/OneLife_v//'`


echo "" 
echo "Most recent Data hg version is:  $lastTaggedDataVersion"
echo ""



numNewChangsets=`hg log -P OneLife_v$lastTaggedDataVersion | grep changeset: | wc -l`


echo "" 
echo "Num hg revisions since version:  $numNewChangsets"
echo ""



if [ $numNewChangsets -lt 2 ]
then
	echo "Too few revisions to bundle (only tag revision), exiting."
	exit 1
fi



echo "" 
echo "Updating OneLifeWorking"
echo ""

cd ~/checkout/OneLifeWorking
hg pull
hg update


echo "" 
echo "Building headless cache generation tool"
echo ""

cd gameSource
sh ./makeRegenerateCaches

cd ~/checkout/OneLifeWorking



lastTaggedCodeVersion=`hg tags | grep "OneLife" -m 1 | awk '{print $1}' | sed -e 's/OneLife_v//'`


echo "" 
echo "Most recent Code hg version is:  $lastTaggedCodeVersion"
echo ""


lastTaggedVersion=$lastTaggedDataVersion

if [ $lastTaggedCodeVersion -gt $lastTaggedDataVersion ]
then
	lastTaggedVersion=$lastTaggedCodeVersion
fi


echo "" 
echo "Most recent Data or Code hg version is:  $lastTaggedVersion"
echo ""



newVersion=$((lastTaggedVersion + 1))

echo "" 
echo "Using new version number:  $newVersion"
echo ""



cd ~/checkout/OneLifeData6Latest


mkdir ~/checkout/diffWorking


echo "" 
echo "Exporing latest data for diffing"
echo ""


hg archive ~/checkout/diffWorking/dataLatest
rm ~/checkout/diffWorking/dataLatest/.hg*
echo -n "$newVersion" > ~/checkout/diffWorking/dataLatest/dataVersionNumber.txt


echo "" 
echo "Exporing last tagged data for diffing"
echo ""

hg update -r OneLife_v$lastTaggedDataVersion

hg archive ~/checkout/diffWorking/dataLast
rm ~/checkout/diffWorking/dataLast/.hg*


# restore repository to latest, to avoid confusion later
hg update


echo "" 
echo "Copying last bundle's reverb cache to save work"
echo ""


cp -r ~/checkout/reverbCacheLastBundle ~/checkout/diffWorking/dataLast/reverbCache
cp -r ~/checkout/reverbCacheLastBundle ~/checkout/diffWorking/dataLatest/reverbCache



echo "" 
echo "Generating caches"
echo ""


cd ~/checkout/diffWorking/dataLast
cp ~/checkout/OneLifeWorking/gameSource/reverbImpulseResponse.aiff .
~/checkout/OneLifeWorking/gameSource/regenerateCaches
rm reverbImpulseResponse.aiff

cd ~/checkout/diffWorking/dataLatest
cp ~/checkout/OneLifeWorking/gameSource/reverbImpulseResponse.aiff .
~/checkout/OneLifeWorking/gameSource/regenerateCaches
rm reverbImpulseResponse.aiff


echo "" 
echo "Saving latest reverb cache for next time"
echo ""


rm -r ~/checkout/reverbCacheLastBundle
cp -r ~/checkout/diffWorking/dataLatest/reverbCache ~/checkout/reverbCacheLastBundle


echo "" 
echo "Generating diff bundle"
echo ""

cd ~/checkout/diffWorking

dbzFileName=${newVersion}_inc_all.dbz

~/checkout/minorGems/game/diffBundle/diffBundle dataLast dataLatest $dbzFileName

cp $dbzFileName ~/diffBundles/
mv $dbzFileName ~/www/updateBundles/

echo -n "http://onehouronelife.com/updateBundles/$dbzFileName" > ~/diffBundles/${newVersion}_inc_all_urls.txt


# don't post new version requirement to reflector just yet
# need to shutdown all servers first


rm -r ~/checkout/diffWorking


echo "" 
echo "Shutting down local server, set server shutdownMode flag."
echo ""


echo -n "1" > ~/checkout/OneLife/server/settings/shutdownMode.ini


echo "" 
echo "Shutting down remote servers, setting server shutdownMode flags."
echo ""



# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
while read user server port
do
  echo "  Shutting down $server"
  ssh $user@$server '~/checkout/OneLife/scripts/remoteServerShutdown.sh'
done <  <( grep "" ~/www/reflector/remoteServerList.ini )




# now that servers are no longer accepting new connections, tell reflector
# that an upgrade is available
echo -n "<?php \$version=$newVersion; ?>" > ~/www/reflector/requiredVersion.php




serverPID=`pgrep OneLifeServer`

if [ -z $serverPID ]
then
	echo "Server not running!"
else

	echo "" 
	echo "Waiting for server to exit"
	echo ""

        while kill -CONT $serverPID 1>/dev/null 2>&1; do sleep 1; done

	echo "" 
	echo "Server has shutdown"
	echo ""
fi





echo "" 
echo "Updating live server data"
echo ""


cd ~/checkout/OneLifeData6
hg pull
hg update



newTag=OneLife_v$newVersion

echo "" 
echo "Updating live server data dataVersionNumber.txt to $newVersion"
echo ""

echo -n "$newVersion" > ~/checkout/OneLifeData6/dataVersionNumber.txt

hg commit -m "Updatated dataVersionNumber to $newVersion"

echo "" 
echo "Tagging live server data with new hg tag $newTag"
echo ""

hg tag $newTag


echo "" 
echo "Pushing tag change to central hg server"
echo ""

hg push



echo "" 
echo "Re-compiling server"
echo ""

cd ~/checkout/OneLife/server
hg pull
hg update

./configure 1
make



echo "" 
echo "Re-launching server"
echo ""


echo -n "0" > ~/checkout/OneLife/server/settings/shutdownMode.ini



cd ~/checkout/OneLife/server/

sh ./runHeadlessServerLinux.sh




echo "" 
echo "Triggering remote server updates and restarts."
echo ""


# contiue with remote server restarts
while read user server port
do
  echo "  Starting update on $server"
  ssh $user@$server '~/checkout/OneLife/scripts/remoteServerUpdate.sh'
done <  <( grep "" ~/www/reflector/remoteServerList.ini )
