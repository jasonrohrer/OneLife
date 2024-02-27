

echo ""
echo "Updating backup server's ssh config"
scp ~/.ssh/config jcr13@backup.onehouronelife.com:.ssh/config


echo ""
echo "Updating backup server's remote server list"

# combine remote server lists from both reflectors


cat ~/www/reflector/remoteServerList.ini ~/www/ahapReflector/remoteServerList.ini > /tmp/remoteServerList.ini


scp /tmp/remoteServerList.ini jcr13@backup.onehouronelife.com:


rm /tmp/tempRemote