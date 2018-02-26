

echo "New remote DOWNLOAD server DNS setup and configuration"

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

sshpass -p "$rootPass" scp -o StrictHostKeychecking=no linodeDownloadServerRootSetup.sh root@$subdomain.onehouronelife.com:


EOSU



echo ""
echo "Connecting to remote host to run setup script there"
echo ""

sshpass -p "$rootPass" ssh -o StrictHostKeychecking=no root@$subdomain.onehouronelife.com "./linodeDownloadServerRootSetup.sh"


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


echo "Telling local diff bundle folder about new server"


cd ~/diffBundles

echo "jcr13 $subdomain.onehouronelife.com" >> remoteServerList.ini




echo "Copying diff bundles to new server's download directory"

scp *.dbz jcr13@$subdomain.onehouronelife.com:downloads/


EOSU2


# cleaner to do this without escaping
echo "Adding to update server's URL mirror lists"

cd /home/jcr15/diffBundles

for file in *.dbz; do
    urlFile=$(echo $file | sed -e 's/.dbz/_urls.txt/g')
    echo "http://$subdomain.onehouronelife.com/downloads/$file" >> $urlFile 
done



su jcr15<<EOSU3
echo "Copying primary download files to new server's download directory"


cd ~/oneLifeDownloads

scp * jcr13@$subdomain.onehouronelife.com:downloads/


echo ""
echo "Telling ticket server about new mirror for main downloads"
echo ""

echo "http://$subdomain.onehouronelife.com/downloads/" >> remoteServerList.ini


EOSU3


echo "Done with new server setup"