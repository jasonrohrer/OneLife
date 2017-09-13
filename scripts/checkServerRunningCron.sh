
postmarkFile=~/postmarkToken.txt

postmarkToken="0"


if [ -f $postmarkFile ]
then
	postmarkToken=$(head -n 1 $postmarkFile)
fi


serverNameFile=~/serverName.txt

serverName="unknown.onehouronelife.com"


if [ -f $serverNameFile ]
then
	serverName=$(head -n 1 $serverNameFile)
fi




# put a 0 in this file to stop this
# cron jobfrom auto-restarting the server
# or a 1 in here to keep it running
flagFile=~/keepServerRunning.txt

if [ -f $flagFile ]
then


	flag=$(head -n 1 $flagFile)


	if [ "$flag" = "1" ]
	then



		if pgrep -x "OneLifeServer" > /dev/null
		then
            # running
			echo "Server is running"
			exit 1
		else
            # stopped
			echo "Server is not running"
	        
            # make sure it's not in shutdown mode
			cd ~/checkout/OneLife/server/
			
			settingsFile=settings/shutdownMode.ini
			if [ -f $settingsFile ]
			then
				setting=$(head -n 1 $settingsFile)


				if [ "$setting" = "1" ]
				then
					echo "Shutdown flag set, not re-launching server"
					exit 1
				fi
			fi
			
			echo "Relaunching server"

			# launch it
			./runHeadlessServerLinux.sh

			serverT=`date`
			pdt=`TZ=":America/Los_Angeles" date`
			
			# send email report
			
			curl "https://api.postmarkapp.com/email" \
				-X POST \
				-H "Accept: application/json" \
				-H "Content-Type: application/json" \
				-H "X-Postmark-Server-Token: $postmarkToken" \
				-d "{From: 'jason@thecastledoctrine.net', To: 'jasonrohrer@fastmail.fm', Subject: 'OneLifeServer on $serverName restarted', TextBody: 'Server time: $serverT\nPDT: $pdt'}"
			exit 1
		fi
	fi
fi

echo "~/keepServerRunning.txt not set to 1"
