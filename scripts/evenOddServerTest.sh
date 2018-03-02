# Script to test even/odd looping.
# Does nothing.





echo "" 
echo "Looping over EVEN servers."
echo ""

# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
i=1
while read user server port
do
	if [ $((i % 2)) -eq 0 ];
	then
		echo "  Hit $server"
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )



echo "" 
echo "Looping over ODD servers."
echo ""

# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
i=1
while read user server port
do
	if [ $((i % 2)) -eq 1 ];
	then
		echo "  Hit $server"
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )



echo "" 
echo "Done looping over all servers."
echo ""
