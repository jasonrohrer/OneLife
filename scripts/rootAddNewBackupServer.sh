

echo "New remote BACKUP server DNS setup and configuration"

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

echo ""
echo "Copying remote setup script onto new server"
echo ""

cd ~/checkout/OneLifeWorking
hg pull
hg update
cd scripts

scp -o StrictHostKeychecking=no linodeBackupServerRootSetup.sh root@$address:


EOSU



echo ""
echo "Connecting to remote host to run setup script there"
echo ""

ssh -o StrictHostKeychecking=no root@$address "./linodeBackupServerRootSetup.sh"


echo ""
echo "Disconnected from remote host"
echo ""



su jcr15<<EOSU2

echo "Setting up local ssh private key for new server"

cd ~/.ssh

echo "" >> config
echo "Host           $subdomain.onehouronelife.com" >> config
echo "HostName       $subdomain.onehouronelife.com" >> config
echo "IdentityFile   ~/.ssh/remoteServers_id_rsa" >> config
echo "User           jcr13" >> config


echo ""
echo "Sending ssh config to backup server"


scp ~/.ssh/config jcr13@$subdomain.onehouronelife.com:.ssh/config
scp ~/.ssh/remoteServers_id_rsa jcr13@$subdomain.onehouronelife.com:.ssh/


echo ""
echo "Updating backup server's remote server list"
scp ~/www/reflector/remoteServerList.ini jcr13@$subdomain.onehouronelife.com:



EOSU2



echo "Done with new server setup"