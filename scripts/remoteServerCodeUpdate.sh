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
echo "Re-compiling server"
echo ""

cd ~/checkout/minorGems
git pull --tags


cd ~/checkout/OneLife/server
git pull --tags

./configure 1
make


git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags | sed -e 's/OneLife_v//' > serverCodeVersionNumber.txt


~/checkout/OneLife/scripts/remoteServerCodeUpdateCustomPost.sh


