# Script run on main server to run git pull, but NOT rebuild, on all
# remote servers.



echo "About to run git pull on all remote servers (without rebuilding them)"
echo ""
echo -n "Hit [ENTER] when ready: "
read



# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
while read user server port
do
  echo "  Running git pull on $server"
  ssh -n $user@$server 'cd ~/checkout/OneLife; git pull'
done <  <( grep "" ~/www/reflector/remoteServerList.ini )


echo "" 
echo "Done running git pull on all servers."
echo ""
