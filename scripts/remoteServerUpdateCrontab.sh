# Script run on main server to update crontabs on all remote servers



echo "About to git pull and update crontabs from latest git on all remote servers (without rebuilding them)"
echo ""
echo -n "Hit [ENTER] when ready: "
read


~/checkout/OneLifeWorking/scripts/remoteServerGitPull.sh





# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
while read user server port
do
  echo "   Updating crontab on $server"
  ssh -n $user@$server 'crontab /home/jcr13/checkout/OneLife/scripts/remoteServerCrontabSource'
done <  <( grep "" ~/www/reflector/remoteServerList.ini )


echo "" 
echo "Done updating crontab on all servers."
echo ""
