echo "About to clear monument logs with 1 or fewer lines"
echo ""
echo -n "Hit [ENTER] when ready: "
read



# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
while read user server port
do
  echo "  Running remoteServerClearShortMonumentLogs on $server"
  ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerClearShortMonumentLogs.sh'
done <  <( grep "" ~/www/reflector/remoteServerList.ini )


echo "" 
echo "Done clearing short monument logs on all servers."
echo ""