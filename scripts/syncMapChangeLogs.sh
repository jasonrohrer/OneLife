

# copy from game server to here
rsync -avz -e ssh --progress bigserver2.onehouronelife.com:checkout/OneLife/server/mapChangeLogs/*.txt ~/mapChangeLogs/

# copy from here to public data server
# send all but latest file, which is not public knowledge yet (still being
# updated with new data, seed still a secret)
cd ~/mapChangeLogs/
rsync -avz -e ssh --progress `ls -1tr | head -n -1` publicdata.onehouronelife.com:public_html/publicMapChangeData/bigserver2.onehouronelife.com/
