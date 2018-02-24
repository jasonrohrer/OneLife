# script to run on main server to wipe local and remote server maps



# shut them all down




echo "" 
echo "Shutting down remote servers, setting server shutdownMode flags."
echo ""

# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
while read user server port
do
  echo "  Shutting down $server"
  ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerShutdown.sh'
done <  <( grep "" ~/www/reflector/remoteServerList.ini )






echo "" 
echo "Triggering remote server map wipes."
echo ""


# contiue with remote server rebuilds
while read user server port
do
  echo "  Starting update on $server"
  ssh -n $user@$server 'cd ~/checkout/OneLife; git pull'
  ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerWipeMap.sh'
done <  <( grep "" ~/www/reflector/remoteServerList.ini )


echo "" 
echo "Done wiping maps on all servers."
echo ""