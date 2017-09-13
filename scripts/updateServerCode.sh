# script run on main server to shutdown all servers and update them
# to the latest binary server code.  Does NOT update game data for the servers.


# shut them all down


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



serverPID=`pgrep OneLifeServer`

if [ -z $serverPID ]
then
	echo "Local server not running!"
else

	echo "" 
	echo "Waiting for local server to exit"
	echo ""

        while kill -CONT $serverPID 1>/dev/null 2>&1; do sleep 1; done

	echo "" 
	echo "Local server has shutdown"
	echo ""
fi




echo "" 
echo "Re-compiling local server"
echo ""

cd ~/checkout/minorGems
hg pull
hg update

cd ~/checkout/OneLife/server
hg pull
hg update

./configure 1
make



echo "" 
echo "Re-launching local server"
echo ""


echo -n "0" > ~/checkout/OneLife/server/settings/shutdownMode.ini


cd ~/checkout/OneLife/server/

sh ./runHeadlessServerLinux.sh


echo -n "1" > ~/keepServerRunning.txt



echo "" 
echo "Triggering remote server rebuilds."
echo ""


# contiue with remote server rebuilds
while read user server port
do
  echo "  Starting update on $server"
  ssh $user@$server '~/checkout/OneLife/scripts/remoteServerCodeUpdate.sh'
done <  <( grep "" ~/www/reflector/remoteServerList.ini )


echo "" 
echo "Done updating code on all servers."
echo ""
