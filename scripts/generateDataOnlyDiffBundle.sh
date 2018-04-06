
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
git pull --tags



lastTaggedDataVersion=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags | sed -e 's/OneLife_v//'`


echo "" 
echo "Most recent Data git version is:  $lastTaggedDataVersion"
echo ""



numNewChangsets=`git log OneLife_v$lastTaggedDataVersion..HEAD | grep commit | wc -l`


echo "" 
echo "Num git revisions since version:  $numNewChangsets"
echo ""



if [ $numNewChangsets -lt 1 ]
then
	echo "No new revisions to bundle, exiting."
	exit 1
fi



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



# any argument means automation
if [ $# -ne 1 ]
then
	echo ""
	echo ""
	echo "Most recent code version $lastTaggedCodeVersion"
	echo "Most recent data version $lastTaggedDataVersion"
	echo ""
	echo "About to post and tag data with $newVersion"
	echo ""
	echo -n "Hit [ENTER] when ready: "
	read
fi



cd ~/checkout/OneLifeData7Latest


mkdir ~/checkout/diffWorking


echo "" 
echo "Exporting latest data for diffing"
echo ""


git clone . ~/checkout/diffWorking/dataLatest
rm -rf ~/checkout/diffWorking/dataLatest/.git*
rm ~/checkout/diffWorking/dataLatest/.hg*
rm -rf ~/checkout/diffWorking/dataLatest/soundsRaw
rm -r ~/checkout/diffWorking/dataLatest/*.sh ~/checkout/diffWorking/dataLatest/working ~/checkout/diffWorking/dataLatest/overlays
echo -n "$newVersion" > ~/checkout/diffWorking/dataLatest/dataVersionNumber.txt


echo "" 
echo "Exporting last tagged data for diffing"
echo ""

git checkout -q OneLife_v$lastTaggedDataVersion

git clone . ~/checkout/diffWorking/dataLast
rm -rf ~/checkout/diffWorking/dataLast/.git*
rm ~/checkout/diffWorking/dataLast/.hg*
rm -rf ~/checkout/diffWorking/dataLast/soundsRaw
rm -r ~/checkout/diffWorking/dataLast/*.sh ~/checkout/diffWorking/dataLast/working ~/checkout/diffWorking/dataLast/overlays


# restore repository to latest, to avoid confusion later
git checkout -q master


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

  echo "http://$server/downloads/$dbzFileName" >> ~/diffBundles/${newVersion}_inc_all_urls.txt

done <  <( grep "" ~/diffBundles/remoteServerList.ini )


# DO NOT add the main server to the mirror list
# farm this out to conserve bandwidth on the main server

# echo -n "http://onehouronelife.com/updateBundles/$dbzFileName" > ~/diffBundles/${newVersion}_inc_all_urls.txt


# don't post new version requirement to reflector just yet
# need to shutdown all servers first


rm -r ~/checkout/diffWorking
















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
git push origin $newTag



echo "" 
echo "Re-compiling non-running local server code base as a sanity check"
echo ""

cd ~/checkout/OneLife/server
git pull

./configure 1
make



echo "" 
echo "Posting to website Update log."
echo ""

~/checkout/OneLifeWorking/scripts/generateUpdatePosts.sh



echo "" 
echo "Generating object report for website."
echo ""

~/checkout/OneLifeWorking/scripts/generateObjectReport.sh






echo "" 
echo "Shutting down EVEN remote servers, setting server shutdownMode flags."
echo ""



# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
i=1
while read user server port
do
	if [ $((i % 2)) -eq 0 ];
	then
		echo "  Shutting down $server"
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerShutdown.sh'
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )







echo "" 
echo "Triggering EVEN remote server updates."
echo ""


# this will wait for even servers to finish shutting down
i=1
while read user server port
do
	if [ $((i % 2)) -eq 0 ];
	then
		echo "  Starting update on $server"
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerUpdate.sh'
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )






echo "" 
echo "Shutting down ODD remote servers, setting server shutdownMode flags."
echo ""



# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
i=1
while read user server port
do
	if [ $((i % 2)) -eq 1 ];
	then
		echo "  Shutting down $server"
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerShutdown.sh'
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )





echo "" 
echo "Setting required version number to $newVersion for all future connections."
echo ""



# now that servers are no longer accepting new connections, tell reflector
# and update server that an upgrade is available
echo -n "$newVersion" > ~/diffBundles/latest.txt

echo -n "<?php \$version=$newVersion; ?>" > ~/www/reflector/requiredVersion.php




echo "" 
echo "Triggering EVEN remote server startups."
echo ""

i=1
while read user server port
do
	if [ $((i % 2)) -eq 0 ];
	then
		echo "  Triggering server startup on $server"
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerStartup.sh'
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )





echo "" 
echo "Triggering ODD remote server updates."
echo ""


# this will wait for ODD servers to finish shutting down
i=1
while read user server port
do
	if [ $((i % 2)) -eq 1 ];
	then
		echo "  Starting update on $server"
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerUpdate.sh'
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )



echo "" 
echo "Triggering ODD remote server startups."
echo ""

i=1
while read user server port
do
	if [ $((i % 2)) -eq 1 ];
	then
		echo "  Triggering server startup on $server"
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerStartup.sh'
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )




echo "" 
echo "Entire update process is done."
echo ""






