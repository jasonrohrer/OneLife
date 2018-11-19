# Script run on main server to set crontab on all remote servers



echo "About to set crontab on all remote servers (without rebuilding them)"
echo ""
echo -n "Hit [ENTER] when ready: "
read



# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
while read user server port
do
  echo "  Setting crontab on $server"
  ssh -n $user@$server 'cd ~/checkout/OneLife/scripts; crontab ./remoteServerCrontabSource'
done <  <( grep "" ~/www/reflector/remoteServerList.ini )


echo "" 
echo "Done setting crontab on all servers."
echo ""
