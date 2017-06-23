# script to run on main server to wipe local and remote server maps



# shut them all down


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
echo "Deleting map and biome db files"
echo ""

rm ~/checkout/OneLife/server/biome.db ~/checkout/OneLife/server/map.db



echo "" 
echo "Re-launching local server"
echo ""


echo -n "0" > ~/checkout/OneLife/server/settings/shutdownMode.ini


cd ~/checkout/OneLife/server/

sh ./runHeadlessServerLinux.sh




echo "" 
echo "Triggering remote server map wipes."
echo ""


# contiue with remote server rebuilds
while read user server port
do
  echo "  Starting update on $server"
  ssh $user@$server '~/checkout/OneLife/scripts/remoteServerWipeMap.sh'
done <  <( grep "" ~/www/reflector/remoteServerList.ini )


echo "" 
echo "Done wiping maps on all servers."
echo ""