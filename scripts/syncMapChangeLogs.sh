

# copy from game server to here
rsync -avz -e ssh --progress bigserver2.onehouronelife.com:checkout/OneLife/server/mapChangeLogs/*.txt ~/mapChangeLogs/

# copy from here to public data server
rsync -avz -e ssh --progress ~/mapChangeLogs/ publicdata.onehouronelife.com:public_html/publicMapChangeData/bigserver2.onehouronelife.com/
