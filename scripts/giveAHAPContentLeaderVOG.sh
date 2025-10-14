

while read user server
do
  echo ""
  echo "Updating vogAllowAccounts.ini on $server"
  scp ~/ahapContentLeaderEmail.txt $user@$server:checkout/OneLife/server/settings/vogAllowAccounts.ini

done <  <( grep "" ~/www/ahapReflector/remoteServerList.ini )