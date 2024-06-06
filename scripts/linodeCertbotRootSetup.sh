# run as root on a web server to get certbot setup for https


apt -y install snapd

snap install --classic certbot


ln -s /snap/bin/certbot /usr/bin/certbot



echo -n "Enter subdomain to use for remote server: "

read subdomain


sudo certbot --nginx -d $subdomain.onehouronelife.com -m jasonrohrer@fastmail.fm --non-interactive --agree-tos



sudo certbot renew --dry-run


ufw allow 443


echo "Please check this URL with your browser:"

echo "   https://$subdomain.onehouronelife.com"

echo ""

echo -n "Press ENTER when done:"

read dummy