

echo ""
echo "Updating backup server's ssh config"
scp ~/.ssh/config jcr13@backup.onehouronelife.com:.ssh/config


echo ""
echo "Updating backup server's remote server list"
scp ~/www/reflector/remoteServerList.ini jcr13@backup.onehouronelife.com:
