passwordLineage="secret"

date=`date +"%Y_%b_%d_%a"`


mysqldump -u jcr13_lineageU --password=$passwordLineage jcr13_lineage > ~/backups/lineage_$date.mysql
gzip -f ~/backups/lineage_$date.mysql



# now all website php stuff, which contains settings files
# that have been modified from git versions

dirName=lineage_webPHP_$date
dirPath=/home/jcr13/backups/$dirName

rm -rf $dirPath
mkdir $dirPath

cd /home/jcr13/public_html
find . -type f -name '*.php' -exec cp --parents -t $dirPath {} +

cd /home/jcr13/backups

tar czf $dirName.tar.gz $dirName

rm -r $dirName




# delete backup files older than two weeks
find ~/backups -mtime +14 -delete
