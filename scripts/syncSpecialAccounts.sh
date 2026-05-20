
# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
while read user server port
do

  echo "  Sending to $server"
  scp /home/jcr15/specialAccounts.ini $user@$server:checkout/OneLife/server/settings/

done <  <( grep "" ~/www/reflector/remoteServerList.ini )


echo "" 
echo "Done updating special accounts on all servers."
echo ""
