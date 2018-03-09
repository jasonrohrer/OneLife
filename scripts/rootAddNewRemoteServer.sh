

echo "New remote GAME server DNS setup and configuration"

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

sshpass -p "$rootPass" scp -o StrictHostKeychecking=no linodeGameServerRootSetup.sh root@$subdomain.onehouronelife.com:


EOSU



echo ""
echo "Connecting to remote host to run setup script there"
echo ""

sshpass -p "$rootPass" ssh -o StrictHostKeychecking=no root@$subdomain.onehouronelife.com "./linodeGameServerRootSetup.sh"


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


echo "Telling local reflector about new server"


cd ~/www/reflector

echo "jcr13 $subdomain.onehouronelife.com 8005" >> remoteServerList.ini




~/checkout/OneLifeWorking/scripts/tellBackupServerAboutGameServerChanges.sh


scp -o StrictHostKeychecking=no ~/postmarkToken.txt jcr13@$subdomain.onehouronelife.com:

echo -n "$subdomain.onehouronelife.com" > /tmp/serverName.txt

scp -o StrictHostKeychecking=no /tmp/serverName.txt jcr13@$subdomain.onehouronelife.com:

rm /tmp/serverName.txt



echo "Sending stats server settings to remote server and restarting it."


scp -o StrictHostKeychecking=no ~/checkout/OneLife/server/settings/statsServerSharedSecret.ini ~/checkout/OneLife/server/settings/statsServerURL.ini ~/checkout/OneLife/server/settings/useStatsServer.ini jcr13@$subdomain.onehouronelife.com:checkout/OneLife/server/settings/

ssh -o StrictHostKeychecking=no jcr13@$subdomain.onehouronelife.com '~/checkout/OneLife/scripts/remoteServerCodeUpdate.sh'

ssh -o StrictHostKeychecking=no jcr13@$subdomain.onehouronelife.com '~/checkout/OneLife/scripts/remoteServerStartup.sh'

EOSU2


echo "Done with new server setup"