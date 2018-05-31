# Script run on main server to run git pull, but NOT rebuild, on all
# remote servers.



echo "About to set serverID to server domain names on all remote servers"
echo ""
echo -n "Hit [ENTER] when ready: "
read



# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
while read user server port
do
  echo "  Setting serverID $server"
  ssh -n $user@$server "echo $server > ~/checkout/OneLife/server/settings/serverID.ini"
done <  <( grep "" ~/www/reflector/remoteServerList.ini )


echo "" 
echo "Done setting serverID on all servers."
echo ""
