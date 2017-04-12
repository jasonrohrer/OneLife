

echo "New remote DOWNLOAD server DNS setup and configuration"

echo ""

echo "You must run this as root from server.thecastledoctrine.net"

echo ""


echo -n "Enter IP address of remote server: "

read address


echo -n "Enter subdomain to use for remote server: "

read subdomain


echo ""

echo "Configuring DNS for $subdomain.onehouronelife.com to point to $address"

echo -n "Hit [ENTER] when ready: "
read

timestamp="$(date +"%s")"


cp /var/named/onehouronelife.com.db /var/named/onehouronelife.com.db_backup_$timestamp

echo "$subdomain	14400	IN	A	$address" >> /var/named/onehouronelife.com.db


echo "Triggering DNS reload"


rndc reload onehouronelife.com in internal

rndc reload onehouronelife.com in external




su jcr15<<EOSU

echo "Running remote setup script on new server"

cd ~/checkout/OneLifeWorking
hg pull
hg update
cd scripts

scp -o StrictHostKeychecking=no linodeDownloadServerRootSetup.sh root@$address:


EOSU


echo ""
echo "You now must login as root on the new server and run "
echo "  ./linodeDownloadServerRootSetup.sh"
echo "This has to be done interactively.  exit when you're done"
echo ""

ssh -o StrictHostKeychecking=no root@$address



su jcr15<<EOSU2

echo "Setting up local ssh private key for new server"

cd ~/.ssh

echo "" >> config
echo "Host           $subdomain" >> config
echo "HostName       $subdomain.onehouronelife.com" >> config
echo "IdentityFile   ~/.ssh/remoteServers_id_rsa" >> config
echo "User           jcr13" >> config


echo "Telling local diff bundle folder about new server"


cd ~/diffBundles

echo "jcr13 $subdomain.onehouronelife.com" >> remoteServerList.ini


echo "Copying diff bundles to new server's download directory"

scp *.dbz jcr13@$address:downloads/

cd ~/oneLifeDownloads

scp * jcr13@$address:downloads/

EOSU2


echo "Done with new server setup"