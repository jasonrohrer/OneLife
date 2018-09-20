# Script run on main server to run git pull, but NOT rebuild, on all
# remote servers.



echo "About to push various custom settings to all remote servers"
echo ""
echo -n "Hit [ENTER] when ready: "
read



# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
while read user server port
do
  echo "  Running pushing settings to $server"
  scp ~/checkout/OneLife/server/settings/statsServerSharedSecret.ini ~/checkout/OneLife/server/settings/statsServerURL.ini ~/checkout/OneLife/server/settings/useStatsServer.ini ~/checkout/OneLife/server/settings/lineageServerSharedSecret.ini ~/checkout/OneLife/server/settings/lineageServerURL.ini ~/checkout/OneLife/server/settings/useLineageServer.ini  ~/checkout/OneLife/server/settings/curseServerSharedSecret.ini ~/checkout/OneLife/server/settings/curseServerURL.ini ~/checkout/OneLife/server/settings/useCurseServer.ini ~/checkout/OneLife/server/settings/reflectorSharedSecret.ini $user@$server:checkout/OneLife/server/settings/

done <  <( grep "" ~/www/reflector/remoteServerList.ini )


echo "" 
echo "Done pushing settings to all servers."
echo ""
