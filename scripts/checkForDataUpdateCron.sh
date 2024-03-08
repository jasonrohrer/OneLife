

flagFile=~/dataUpdateFlag.txt

if [ -f $flagFile ]
then


	flag=$(head -n 1 $flagFile)


	if [ "$flag" = "1" ]
	then
		echo ""
		echo "Data update scheduled.  Running it now."
				
		echo "0" > $flagFile
		
		# pass two arguments to indicate automation and skip interactive
		# confirmation
		~/checkout/OneLifeWorking/scripts/generateDataOnlyDiffBundle.sh -a -a
	fi
fi