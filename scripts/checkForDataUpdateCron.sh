

flagFile=~/dataUpdateFlag.txt

if [ -f $flagFile ]
then


	flag=$(head -n 1 $flagFile)


	if [ "$flag" = "1" ]
	then
		echo ""
		echo "Data update scheduled.  Running it now."
		
		~/checkout/OneLifeWorking/scripts/generateDataOnlyDiffBundle.sh
		
		
		echo "0" > $flagFile
	fi
fi