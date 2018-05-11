passwordLineage="secret"

date=`date +"%Y_%b_%d_%a"`


mysqldump -u jcr13_lineageU --password=$passwordLineage jcr13_lineage > ~/backups/lineage_$date.mysql
gzip -f ~/backups/lineage_$date.mysql