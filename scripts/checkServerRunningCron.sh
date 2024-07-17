
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

		# make sure it's not a read-only filesystem
		ro=`grep ' / ' /proc/mounts | grep --count ' ro,'`
		
		if [ $ro -eq 1 ]
		then
			echo "File system read-only"
			
			# send email report
			serverT=`date`
			pdt=`TZ=":America/Los_Angeles" date`
			
			curl "https://api.postmarkapp.com/email" \
				-X POST \
				-H "Accept: application/json" \
				-H "Content-Type: application/json" \
				-H "X-Postmark-Server-Token: $postmarkToken" \
				-d "{\"From\": \"jason@thecastledoctrine.net\", \"To\": \"jasonrohrer@fastmail.fm\", \"Subject\": \"OneLifeServer on $serverName has read-only file system\", \"TextBody\": \"File system mounted at / flagged as ro.\"}"				
			exit 1
		fi 


		if pgrep -x "OneLifeServer" > /dev/null
		then
            # running
			echo "Server is running"

			
			# make sure there's enough free disk space
			# at least one GB
			minSpace=1000000
			remainSpace=$(df / | tail -n 1 | sed -e "s#/dev/root[ ]*[0-9]*[ ]*[0-9]*[ ]*##" | sed -e "s/ [ ]*[0-9%]*.*//");


			echo "Server has $remainSpace KB disk space."
			echo "min sufficient is $minSpace KB"
			
			if [ $remainSpace -lt $minSpace ]
			then
				echo "Server has insufficient disk space"

				# send email report
				serverT=`date`
				pdt=`TZ=":America/Los_Angeles" date`
				
				curl "https://api.postmarkapp.com/email" \
					-X POST \
					-H "Accept: application/json" \
					-H "Content-Type: application/json" \
					-H "X-Postmark-Server-Token: $postmarkToken" \
					-d "{\"From\": \"jason@thecastledoctrine.net\", \"To\": \"jasonrohrer@fastmail.fm\", \"Subject\": \"OneLifeServer on $serverName has low disk space\", \"TextBody\": \"$remainSpace KB remain.  Server shut down as precaution at time: $serverT\nPDT: $pdt\"}"				

				
				echo "Shutting server down to be safe."

				echo "1" > ~/checkout/OneLife/server/settings/shutdownMode.ini
			else
				echo "Server has sufficient disk space."
			fi

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
			bash -l ./runHeadlessServerLinux.sh

			serverT=`date`
			pdt=`TZ=":America/Los_Angeles" date`
			
			# send email report
			
			curl "https://api.postmarkapp.com/email" \
				-X POST \
				-H "Accept: application/json" \
				-H "Content-Type: application/json" \
				-H "X-Postmark-Server-Token: $postmarkToken" \
				-d "{\"From\": \"jason@thecastledoctrine.net\", \"To\": \"jasonrohrer@fastmail.fm\", \"Subject\": \"OneLifeServer on $serverName restarted\", \"TextBody\": \"Server time: $serverT\nPDT: $pdt\"}"
			exit 1
		fi
	fi
fi

echo "~/keepServerRunning.txt not set to 1"
