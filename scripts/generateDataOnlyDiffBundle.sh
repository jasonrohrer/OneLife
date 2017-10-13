
echo "" 
echo "Updating minorGems"
echo ""

cd ~/checkout/minorGems
git pull



echo "" 
echo "Building diffBundle tool"
echo ""

cd game/diffBundle
./diffBundleCompile



echo "" 
echo "Updating OneLifeData7Latest"
echo ""

cd ~/checkout/OneLifeData7Latest
git pull



lastTaggedDataVersion=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags | sed -e 's/OneLife_v//'`


echo "" 
echo "Most recent Data git version is:  $lastTaggedDataVersion"
echo ""



numNewChangsets=`git log OneLife_v$lastTaggedDataVersion..HEAD | grep commit | wc -l`


echo "" 
echo "Num git revisions since version:  $numNewChangsets"
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
git pull


echo "" 
echo "Building headless cache generation tool"
echo ""

cd gameSource
sh ./makeRegenerateCaches

cd ~/checkout/OneLifeWorking



lastTaggedCodeVersion=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags | sed -e 's/OneLife_v//'`


echo "" 
echo "Most recent Code git version is:  $lastTaggedCodeVersion"
echo ""


lastTaggedVersion=$lastTaggedDataVersion

if [ $lastTaggedCodeVersion -gt $lastTaggedDataVersion ]
then
	lastTaggedVersion=$lastTaggedCodeVersion
fi


echo "" 
echo "Most recent Data or Code git version is:  $lastTaggedVersion"
echo ""



newVersion=$((lastTaggedVersion + 1))

echo "" 
echo "Using new version number:  $newVersion"
echo ""



cd ~/checkout/OneLifeData7Latest


mkdir ~/checkout/diffWorking


echo "" 
echo "Exporting latest data for diffing"
echo ""


git clone . ~/checkout/diffWorking/dataLatest
rm -rf ~/checkout/diffWorking/dataLatest/.git*
rm ~/checkout/diffWorking/dataLatest/.hg*
rm -r ~/checkout/diffWorking/dataLatest/*.sh ~/checkout/diffWorking/dataLatest/working ~/checkout/diffWorking/dataLatest/overlays
echo -n "$newVersion" > ~/checkout/diffWorking/dataLatest/dataVersionNumber.txt


echo "" 
echo "Exporting last tagged data for diffing"
echo ""

git checkout OneLife_v$lastTaggedDataVersion

git clone . ~/checkout/diffWorking/dataLast
rm -rf ~/checkout/diffWorking/dataLast/.git*
rm ~/checkout/diffWorking/dataLast/.hg*
rm -r ~/checkout/diffWorking/dataLast/*.sh ~/checkout/diffWorking/dataLast/working ~/checkout/diffWorking/dataLast/overlays


# restore repository to latest, to avoid confusion later
git checkout master


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


# start with an empty URL list
echo -n "" > ~/diffBundles/${newVersion}_inc_all_urls.txt


# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline

# send this new .dbz to all the download servers
while read user server
do
  echo ""
  echo "Sending $dbzFileName to $server"
  scp ~/diffBundles/$dbzFileName $user@$server:downloads/

  echo "Adding url for $server to mirror list for this .dbz"

  echo "http://$server/downloads/$dbzFileName" > ~/diffBundles/${newVersion}_inc_all_urls.txt

done <  <( grep "" ~/diffBundles/remoteServerList.ini )


# DO NOT add the main server to the mirror list
# farm this out to conserve bandwidth on the main server

# echo -n "http://onehouronelife.com/updateBundles/$dbzFileName" > ~/diffBundles/${newVersion}_inc_all_urls.txt


# don't post new version requirement to reflector just yet
# need to shutdown all servers first


rm -r ~/checkout/diffWorking


echo "" 
echo "Shutting down local server, set server shutdownMode flag."
echo ""


echo -n "0" > ~/keepServerRunning.txt
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


cd ~/checkout/OneLifeData7
git pull
rm */cache.fcz



newTag=OneLife_v$newVersion

echo "" 
echo "Updating live server data dataVersionNumber.txt to $newVersion"
echo ""

echo -n "$newVersion" > dataVersionNumber.txt

git add dataVersionNumber.txt
git commit -m "Updatated dataVersionNumber to $newVersion"


echo "" 
echo "Tagging live server data with new git tag $newTag"
echo ""

git tag -a $newTag -m "Tag automatically generated by data-only diffBundle script."


echo "" 
echo "Pushing tag change to central git server"
echo ""

git push
git push $newTag



echo "" 
echo "Re-compiling server"
echo ""

cd ~/checkout/OneLife/server
git pull

./configure 1
make



echo "" 
echo "Re-launching server"
echo ""


echo -n "0" > ~/checkout/OneLife/server/settings/shutdownMode.ini



cd ~/checkout/OneLife/server/

sh ./runHeadlessServerLinux.sh


echo -n "1" > ~/keepServerRunning.txt




echo "" 
echo "Triggering remote server updates and restarts."
echo ""


# contiue with remote server restarts
while read user server port
do
  echo "  Starting update on $server"
  ssh $user@$server '~/checkout/OneLife/scripts/remoteServerUpdate.sh'
done <  <( grep "" ~/www/reflector/remoteServerList.ini )




echo "" 
echo "Posting to website Update log."
echo ""

~/checkout/OneLifeWorking/scripts/generateUpdatePosts.sh



echo "" 
echo "Generating object report for website."
echo ""

~/checkout/OneLifeWorking/scripts/generateObjectReport.sh