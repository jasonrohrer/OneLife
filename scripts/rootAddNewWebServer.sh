

echo "New remote WEB server DNS setup and configuration"

echo ""

echo "You must run this as root from onehouronelife.com"

echo ""


if [ $# -ne 2 ]
then
	echo -n "Enter subdomain to use for remote server: "

	read subdomain

	echo ""

	echo -n "Enter root password to use for remote server: "

	read rootPass

	echo ""


	echo "Manually configure Linode DNS A record for $subdomain.onehouronelife.com to pointing to the new server's IP address."

	echo ""
	
	echo -n "Hit [ENTER] when ready: "
	read

else
	subdomain=$1
	rootPass=$2
fi



su jcr15<<EOSU

echo ""
echo "Copying remote setup script onto new server"
echo ""

cd ~/checkout/OneLifeWorking
git pull
cd scripts

sshpass -p "$rootPass" scp -o StrictHostKeychecking=no linodeWebServerRootSetup.sh root@$subdomain.onehouronelife.com:


EOSU



echo ""
echo "Connecting to remote host to run setup script there"
echo ""

sshpass -p "$rootPass" ssh -o StrictHostKeychecking=no root@$subdomain.onehouronelife.com "./linodeWebServerRootSetup.sh"


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



~/checkout/OneLifeWorking/scripts/tellBackupServerAboutGameServerChanges.sh


EOSU2







echo "Done with new Web server setup"