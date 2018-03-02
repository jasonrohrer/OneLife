# script run on main server to shutdown all servers and update them
# to the latest binary server code.  Does NOT update game data for the servers.


# shuts down each server and rebuilds them, one by one, allowing
# players to continue playing on the other servers in the mean time





echo "" 
echo "Re-compiling non-running local server code base as a sanity check"
echo ""

cd ~/checkout/minorGems
git pull

cd ~/checkout/OneLife/server
git pull

./configure 1
make





echo "" 
echo "Triggering EVEN remote server shutdowns."
echo ""

# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
i=1
while read user server port
do
	if [ $((i % 2)) -eq 0 ];
	then
		echo "  Shutting down $server"
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerShutdown.sh'
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )



echo "" 
echo "Triggering EVEN remote server code updates and restart."
echo ""

# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
i=1
while read user server port
do
	if [ $((i % 2)) -eq 0 ];
	then
		echo "  Starting update on $server"
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerCodeUpdate.sh'
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )






echo "" 
echo "Triggering ODD remote server shutdowns."
echo ""

# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
i=1
while read user server port
do
	if [ $((i % 2)) -eq 1 ];
	then
		echo "  Shutting down $server"
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerShutdown.sh'
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )



echo "" 
echo "Triggering ODD remote server code updates and restart."
echo ""

# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
i=1
while read user server port
do
	if [ $((i % 2)) -eq 1 ];
	then
		echo "  Starting update on $server"
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerCodeUpdate.sh'
	fi
	i=$((i + 1))  
done <  <( grep "" ~/www/reflector/remoteServerList.ini )







echo "" 
echo "Done updating code on all servers."
echo ""
