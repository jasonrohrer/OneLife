

flagFile=/home/jcr15/ahapDataUpdateFlag.txt

if [ -f $flagFile ]
then


	flag=$(head -n 1 $flagFile)

	# dummy arg
	wipeArg="-a"

	if [ "$flag" = "2" ]
	then
		flag="1"
		wipeArg="WIPE"
	fi


	if [ "$flag" = "1" ]
	then
		echo ""
		echo "Data update scheduled.  Running it now."

		echo "0" > $flagFile
		
		date=`date +"%Y_%m_%d__%H_%M_%S"`;

		logPath="/tmp/ahapUpdateGateLog.txt"

		if [ -f $logPath ]
		then
			mv $logPath /tmp/ahapUpdateGateLog_backup_$date.txt
		fi

		

		# pass two arguments to indicate automation and skip interactive
		# confirmation
		# second argument is wipe flag
		/home/jcr15/checkout/OneLifeWorking/scripts/generateDataOnlyDiffBundleAHAP.sh -a $wipeArg > $logPath 2>&1
				
	fi
fi