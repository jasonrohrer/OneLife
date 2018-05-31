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


cd ~/checkout/OneLife/server/
# this bundles local lifelog and lifeLog_serverX folders together
tar czf ~/backups/lifeLog_$date.tar.gz lifeLog lifeLog_*

# this bundles local foodLog and foodLog_serverX folders together
tar czf ~/backups/foodLog_$date.tar.gz foodLog foodLog_*

# this bundles local failureLog and failureLog_serverX folders together
tar czf ~/backups/failureLog_$date.tar.gz failureLog failureLog_*

# this bundles local failureLog and failureLog_serverX folders together
tar czf ~/backups/monumentLogs_$date.tar.gz failureLogs failureLogs_*


cd ~/www/reflector/
tar czf ~/backups/apocalypseLog_$date.tar.gz apocalypseLog.txt


cd ~

# delete backup files older than two weeks
find ~/backups -mtime +14 -delete




rsync -avz -e ssh --progress ~/backups/* jcr13@backup.onehouronelife.com:backups/main/ 

rsync -avz -e ssh --progress ~/checkout/OneLife/server/backups/* jcr13@backup.onehouronelife.com:backups/main/ 


rsync -avz -e ssh --progress ~/www/artPosts/ jcr13@backup.onehouronelife.com:backups/main/artPosts/ --delete

rsync -avz -e ssh --progress ~/www/artPostsBig/ jcr13@backup.onehouronelife.com:backups/main/artPostsBig/ --delete
