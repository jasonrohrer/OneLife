

flagFile=~/ahapDataUpdateFlag.txt

if [ -f $flagFile ]
then


	flag=$(head -n 1 $flagFile)


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
		~/checkout/OneLifeWorking/scripts/generateDataOnlyDiffBundleAHAP.sh -a -a > $logPath 2>&1
				
	fi
fi