
# exit on first error
set -e


cd ~/checkout/OneLife/server/

cp -r monumentLogs monumentLogs.bak

cd monumentLogs

# clear those with less than two lines
# method found here:  
# https://unix.stackexchange.com/questions/327371/fast-way-to-delete-files-with-less-than-x-lines

find . -type f -exec awk -v x=2 'NR==x{exit 1}' {} \; -exec rm -f {} \;