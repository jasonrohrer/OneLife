# Script run on main server to clear old log files on all remote servers



echo "About to clear old log files on all remote servers (without rebuilding them)"
echo ""
echo -n "Hit [ENTER] when ready: "
read



# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
while read user server port
do
  echo "   Clearing old bug files  on $server"
  ssh -n $user@$server 'echo Before:; df /;  /home/jcr13/checkout/OneLife/scripts/clearOldLogFiles.sh; echo After:; df /'
done <  <( grep "" ~/www/reflector/remoteServerList.ini )


echo "" 
echo "Done clearing old log files on all servers."
echo ""
