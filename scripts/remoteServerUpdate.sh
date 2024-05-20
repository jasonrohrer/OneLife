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
echo "Updating live server data"
echo ""


cd ~/checkout/OneLifeData7
git checkout master
git pull --tags --force
git reset origin/master --hard
rm */cache.fcz
rm */bin_*cache.fcz


echo "" 
echo "Re-compiling server"
echo ""

cd ~/checkout/minorGems
git checkout master
git pull --tags --force
git reset origin/master --hard


~/checkout/OneLife/scripts/serverPull.sh

cd ~/checkout/OneLife/server


./configure 1
make


rm -f dataVersionNumber.txt

ln -s ~/checkout/OneLifeData7/dataVersionNumber.txt .


rm -f isAHAP.txt

ln -s ~/checkout/OneLifeData7/isAHAP.txt .


git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags/OneLife_v* | sed -e 's/OneLife_v//' > serverCodeVersionNumber.txt
