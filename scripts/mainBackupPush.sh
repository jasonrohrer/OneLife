# run as a cron job on main server to get MySQL backups
# and push all backups to backup server

# Mysql user passwords MUST be set here for this to work
passwordTicket="secret1"
passwordForums="secret2"

date=`date +"%Y_%b_%d_%a"`


mysqldump -u jcr15_olTickUser --password=$passwordTicket jcr15_olTicket > ~/backups/ol_ticket_$date.mysql
gzip -f ~/backups/ol_ticket_$date.mysql


mysqldump -u jcr15_olForumU --password=$passwordForums jcr15_olForums > ~/backups/ol_forums_$date.mysql
gzip -f ~/backups/ol_forums_$date.mysql


# delete backup files older than two weeks
find ~/backups -mtime +14 -delete




rsync -avz -e ssh --progress ~/backups/* jcr13@backup.onehouronelife.com:backups/main/ 

rsync -avz -e ssh --progress ~/checkout/OneLife/server/backups/* jcr13@backup.onehouronelife.com:backups/main/ 
