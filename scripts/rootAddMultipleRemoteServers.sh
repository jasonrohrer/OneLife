if [ $# -ne 1 ]
then
	echo "Usage:"
	echo "rootAddMultipleRemoteServers.sh  list_file"
	echo ""
    echo "list_file contains one subdomain name per line, like:"
    echo "download3"
	echo "download4"
	echo "download5"
	echo ""
	echo "Example:"
	echo "rootAddMultipleRemoteServers.sh list.txt"
	echo ""
	
	exit 1
fi



echo -n "Enter root password to use for remote servers: "

read rootPass

echo ""



# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
echo "About to run Remote Server setup for these subdomains:"
echo ""

num=1
while read subdomain
do
  echo "$num.  $subdomain"
  num=$((num + 1))
done <  <( grep "" $1 )


echo ""
	
echo -n "Hit [ENTER] when ready: "
read


while read subdomain
do
    # don't interrupt our reading of input lines if sub-script reads input
	./rootAddNewRemoteServer.sh $subdomain $rootPass </dev/null
done <  <( grep "" $1 )
