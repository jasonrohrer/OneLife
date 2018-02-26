

echo "New remote BACKUP server DNS setup and configuration"

echo ""

echo "You must run this as root from onehouronelife.com"

echo ""


echo -n "Enter subdomain to use for remote server: "

read subdomain


echo ""

echo "Manually configure Linode DNS A record for $subdomain.onehouronelife.com to pointing to the new server's IP address"

echo -n "Hit [ENTER] when ready: "
read





su jcr15<<EOSU

echo ""
echo "Copying remote setup script onto new server"
echo ""

cd ~/checkout/OneLifeWorking
git pull
cd scripts

scp -o StrictHostKeychecking=no linodeBackupServerRootSetup.sh root@$subdomain.onehouronelife.com:


EOSU



echo ""
echo "Connecting to remote host to run setup script there"
echo ""

ssh -o StrictHostKeychecking=no root@$subdomain.onehouronelife.com "./linodeBackupServerRootSetup.sh"


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