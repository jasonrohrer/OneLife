

if [ $# -ne 1 ]
then
	echo "Usage:"
	echo "removeDownloadServerFromURLLists.sh  server_subdomain"
	echo ""
	echo "Example:"
	echo "removeDownloadServerFromURLLists.sh download2"
	echo ""
	
	exit 1
fi


subdomain=$1

sed -i "/${subdomain}.onehouronelife.com/d" ~/diffBundles/*_urls.txt