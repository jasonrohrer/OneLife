# run as a cron job on main server to get MySQL backups
# and push all backups to backup server

# Mysql user passwords MUST be set here for this to work
passowrdTicket="secret"
passowrdForums="secret2"

date=`date +"%Y_%b_%d_%a"`


mysqldump -u jcr15_olTickU --password=$passwordTicket jcr15_olTicket > /home
/jcr15/backups/cd_ticket_$date.mysql
gzip -f ~/backups/ol_ticket_$date.mysql


mysqldump -u jcr15_olForumsU --password=$passowrdForums jcr15_olForums > /home/j
cr15/backups/ol_forums_$date.mysql
gzip -f ~/backups/ol_forums_$date.mysql


# delete backup files older than two weeks
find ~/backups -mtime +14 -delete



rsync -avz -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" --progress ~/backups/* $user@$server:checkout/OneLife/server/backups/main/ 

rsync -avz -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" --progress ~/checkout/OneLife/server/backups/* $user@$server:checkout/OneLife/server/backups/main/ 
